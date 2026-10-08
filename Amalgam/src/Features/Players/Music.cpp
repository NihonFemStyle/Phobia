#include "Music.h"

#include "../../SDK/SDK.h"
#include "../Configs/Configs.h"
#include "../ImGui/Menu/Menu.h"
#include "../ImGui/Menu/Components.h"
#include "../ImGui/Menu/neverlose/blur.hpp"

#include <roapi.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <windows.foundation.h>
#include <windows.media.control.h>
#include <windows.storage.streams.h>
#include <robuffer.h>

#include <d3d9.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

#include <algorithm>
#include <chrono>
#include <format>

// Global System Media Transport Controls "now playing" overlay.
// The SMTC plumbing lives on a background worker thread (MTA); the frame thread
// renders the cached info and only decodes artwork into a D3D texture on the render loop.

namespace
{
	using namespace Microsoft::WRL;
	namespace ABI_F = ABI::Windows::Foundation;
	namespace ABI_MC = ABI::Windows::Media::Control;
	namespace ABI_SS = ABI::Windows::Storage::Streams;

	// ---- combase.dll dynamic loading ----
	typedef HRESULT(__stdcall* tRoGetActivationFactory)(HSTRING, REFIID, void**);
	typedef HRESULT(__stdcall* tWindowsCreateString)(PCWSTR, UINT32, HSTRING*);
	typedef HRESULT(__stdcall* tWindowsDeleteString)(HSTRING);
	typedef PCWSTR(__stdcall* tWindowsGetStringRawBuffer)(HSTRING, UINT32*);

	tRoGetActivationFactory pRoGetActivationFactory = nullptr;
	tWindowsCreateString pWindowsCreateString = nullptr;
	tWindowsDeleteString pWindowsDeleteString = nullptr;
	tWindowsGetStringRawBuffer pWindowsGetStringRawBuffer = nullptr;
	bool bComBaseLoaded = false;

	bool LoadComBase()
	{
		if (bComBaseLoaded)
			return pRoGetActivationFactory && pWindowsCreateString && pWindowsDeleteString && pWindowsGetStringRawBuffer;

		HMODULE hModule = LoadLibraryW(L"combase.dll");
		if (hModule)
		{
			pRoGetActivationFactory = (tRoGetActivationFactory)GetProcAddress(hModule, "RoGetActivationFactory");
			pWindowsCreateString = (tWindowsCreateString)GetProcAddress(hModule, "WindowsCreateString");
			pWindowsDeleteString = (tWindowsDeleteString)GetProcAddress(hModule, "WindowsDeleteString");
			pWindowsGetStringRawBuffer = (tWindowsGetStringRawBuffer)GetProcAddress(hModule, "WindowsGetStringRawBuffer");
		}
		bComBaseLoaded = true;
		return pRoGetActivationFactory && pWindowsCreateString && pWindowsDeleteString && pWindowsGetStringRawBuffer;
	}

	HRESULT Activate(const WCHAR* szClass, const IID& iid, void** ppv)
	{
		HSTRING hs = nullptr;
		HRESULT hr = pWindowsCreateString(szClass, (UINT32)wcslen(szClass), &hs);
		if (FAILED(hr))
			return hr;
		hr = pRoGetActivationFactory(hs, iid, ppv);
		pWindowsDeleteString(hs);
		return hr;
	}

	std::wstring ReadHString(HSTRING hs)
	{
		if (!hs)
			return L"";
		UINT32 uLength = 0;
		PCWSTR pBuf = pWindowsGetStringRawBuffer(hs, &uLength);
		return pBuf ? std::wstring(pBuf, uLength) : L"";
	}
	void FreeHString(HSTRING hs)
	{
		if (hs)
			pWindowsDeleteString(hs);
	}

	std::string Narrow(const std::wstring& sWide)
	{
		if (sWide.empty())
			return {};

		const int iLen = WideCharToMultiByte(CP_UTF8, 0, sWide.c_str(), (int)sWide.size(), nullptr, 0, nullptr, nullptr);
		if (iLen <= 0)
			return {};

		std::string sOut(size_t(iLen), '\0');
		WideCharToMultiByte(CP_UTF8, 0, sWide.c_str(), (int)sWide.size(), sOut.data(), iLen, nullptr, nullptr);
		return sOut;
	}

	double NowMonotonic()
	{
		using namespace std::chrono;
		return double(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count()) * 1e-9;
	}

	ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSessionManager> g_Manager;

	// ---- blocking SMTC helpers (worker thread only) ----

	bool AcquireManager(DWORD dwTimeout)
	{
		if (g_Manager)
			return true;
		if (!LoadComBase())
			return false;

		ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSessionManagerStatics> pStatics;
		if (FAILED(Activate(L"Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager", IID_PPV_ARGS(pStatics.ReleaseAndGetAddressOf()))))
			return false;

		ComPtr<ABI_F::IAsyncOperation<ABI_MC::GlobalSystemMediaTransportControlsSessionManager*>> pOp;
		if (FAILED(pStatics->RequestAsync(pOp.ReleaseAndGetAddressOf())))
			return false;

		HANDLE hDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!hDone)
			return false;

		pOp->put_Completed(Callback<ABI_F::IAsyncOperationCompletedHandler<ABI_MC::GlobalSystemMediaTransportControlsSessionManager*>>(
			[hDone, pOp](ABI_F::IAsyncOperation<ABI_MC::GlobalSystemMediaTransportControlsSessionManager*>* op, ABI_F::AsyncStatus status) -> HRESULT
			{
				if (status == ABI_F::AsyncStatus::Completed)
					op->GetResults(g_Manager.ReleaseAndGetAddressOf());
				SetEvent(hDone);
				return S_OK;
			}).Get());

		const bool bOk = WaitForSingleObject(hDone, dwTimeout) == WAIT_OBJECT_0;
		CloseHandle(hDone);
		return bOk && !!g_Manager;
	}

	bool GetMediaPropsBlocking(ABI_MC::IGlobalSystemMediaTransportControlsSession* pSession,
	                           ABI_MC::IGlobalSystemMediaTransportControlsSessionMediaProperties** ppProps)
	{
		if (!pSession || !ppProps)
			return false;

		ComPtr<ABI_F::IAsyncOperation<ABI_MC::GlobalSystemMediaTransportControlsSessionMediaProperties*>> pOp;
		if (FAILED(pSession->TryGetMediaPropertiesAsync(pOp.ReleaseAndGetAddressOf())))
			return false;

		HANDLE hDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!hDone)
			return false;

		bool bGot = false;
		pOp->put_Completed(Callback<ABI_F::IAsyncOperationCompletedHandler<ABI_MC::GlobalSystemMediaTransportControlsSessionMediaProperties*>>(
			[hDone, &bGot, ppProps, pOp](ABI_F::IAsyncOperation<ABI_MC::GlobalSystemMediaTransportControlsSessionMediaProperties*>* op, ABI_F::AsyncStatus status) -> HRESULT
			{
				if (status == ABI_F::AsyncStatus::Completed && SUCCEEDED(op->GetResults(ppProps)) && *ppProps)
					bGot = true;
				SetEvent(hDone);
				return S_OK;
			}).Get());

		const bool bOk = WaitForSingleObject(hDone, 1500) == WAIT_OBJECT_0;
		CloseHandle(hDone);
		return bOk && bGot;
	}

	bool ReadArtworkBlocking(ABI_MC::IGlobalSystemMediaTransportControlsSessionMediaProperties* pProps, std::vector<uint8_t>& vOut)
	{
		ComPtr<ABI_SS::IRandomAccessStreamReference> pRef;
		if (FAILED(pProps->get_Thumbnail(pRef.ReleaseAndGetAddressOf())) || !pRef)
			return false;

		struct Ctx
		{
			ComPtr<ABI_SS::IRandomAccessStreamWithContentType> pStream;
			std::vector<uint8_t> vData;
			bool bGot = false;
		};

		// stage 1: open the stream; chain into a buffer ReadAsync
		auto fOpenDone = [](ABI_F::IAsyncOperation<ABI_SS::IRandomAccessStreamWithContentType*>* pOp1, ABI_F::AsyncStatus st, Ctx* pCtx, HANDLE hDone) -> HRESULT
		{
			if (st != ABI_F::AsyncStatus::Completed)
			{
				SetEvent(hDone);
				return S_OK;
			}
			if (FAILED(pOp1->GetResults(pCtx->pStream.ReleaseAndGetAddressOf())) || !pCtx->pStream)
			{
				SetEvent(hDone);
				return S_OK;
			}

			ComPtr<ABI_SS::IRandomAccessStream> pRas;
			if (FAILED(pCtx->pStream.As(&pRas)) || !pRas)
			{
				SetEvent(hDone);
				return S_OK;
			}

			UINT64 uSize = 0;
			if (FAILED(pRas->get_Size(&uSize)) || !uSize || uSize > 16 * 1024 * 1024)
			{
				SetEvent(hDone);
				return S_OK;
			}

			ComPtr<ABI_SS::IInputStream> pStreamIn;
			if (FAILED(pRas.As(&pStreamIn)) || !pStreamIn)
			{
				SetEvent(hDone);
				return S_OK;
			}

			ComPtr<ABI_SS::IBufferFactory> pBufFactory;
			if (FAILED(Activate(L"Windows.Storage.Streams.Buffer", IID_PPV_ARGS(pBufFactory.ReleaseAndGetAddressOf()))))
			{
				SetEvent(hDone);
				return S_OK;
			}

			ComPtr<ABI_SS::IBuffer> pBuffer;
			if (FAILED(pBufFactory->Create((UINT32)uSize, pBuffer.GetAddressOf())))
			{
				SetEvent(hDone);
				return S_OK;
			}

			ComPtr<ABI_F::IAsyncOperationWithProgress<ABI_SS::IBuffer*, UINT32>> pReadOp;
			if (FAILED(pStreamIn->ReadAsync(pBuffer.Get(), (UINT32)uSize, ABI_SS::InputStreamOptions_None, pReadOp.ReleaseAndGetAddressOf())))
			{
				SetEvent(hDone);
				return S_OK;
			}

			auto fReadDone = [hDone, pCtx](ABI_F::IAsyncOperationWithProgress<ABI_SS::IBuffer*, UINT32>* pOp2, ABI_F::AsyncStatus st2) -> HRESULT
			{
				ComPtr<ABI_SS::IBuffer> pResult;
				if (st2 == ABI_F::AsyncStatus::Completed && SUCCEEDED(pOp2->GetResults(pResult.GetAddressOf())) && pResult)
				{
					UINT32 uLength = 0;
					if (SUCCEEDED(pResult->get_Length(&uLength)) && uLength)
					{
						ComPtr<Windows::Storage::Streams::IBufferByteAccess> pBytes;
						if (SUCCEEDED(pResult.As(&pBytes)) && pBytes)
						{
							BYTE* pData = nullptr;
							if (SUCCEEDED(pBytes->Buffer(&pData)) && pData)
							{
								pCtx->vData.assign(pData, pData + uLength);
								pCtx->bGot = true;
							}
						}
					}
				}
				SetEvent(hDone);
				return S_OK;
			};

			auto hRead = Callback<ABI_F::IAsyncOperationWithProgressCompletedHandler<ABI_SS::IBuffer*, UINT32>>(fReadDone);
			if (!hRead)
			{
				SetEvent(hDone);
				return S_OK;
			}
			return pReadOp->put_Completed(hRead.Get());
		};

		ComPtr<ABI_F::IAsyncOperation<ABI_SS::IRandomAccessStreamWithContentType*>> pOpenOp;
		if (FAILED(pRef->OpenReadAsync(pOpenOp.ReleaseAndGetAddressOf())))
			return false;

		auto* pCtx = new Ctx;
		HANDLE hDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!hDone)
		{
			delete pCtx;
			return false;
		}

		auto hOpen = Callback<ABI_F::IAsyncOperationCompletedHandler<ABI_SS::IRandomAccessStreamWithContentType*>>(
			[fOpenDone, pOpenOp, pCtx, hDone](ABI_F::IAsyncOperation<ABI_SS::IRandomAccessStreamWithContentType*>* op, ABI_F::AsyncStatus status) -> HRESULT
			{
				return fOpenDone(op, status, pCtx, hDone);
			});
		if (!hOpen)
		{
			delete pCtx;
			CloseHandle(hDone);
			return false;
		}

		const HRESULT hr = pOpenOp->put_Completed(hOpen.Get());
		if (FAILED(hr))
		{
			delete pCtx;
			CloseHandle(hDone);
			return false;
		}

		const bool bOk = WaitForSingleObject(hDone, 3000) == WAIT_OBJECT_0;
		CloseHandle(hDone);

		if (bOk && pCtx->bGot)
			vOut.swap(pCtx->vData);
		const bool bGot = pCtx->bGot;
		delete pCtx;
		return bGot;
	}

	// ---- GDI+ lifetime ----
	struct CGdiPlusInit
	{
		CGdiPlusInit()
		{
			Gdiplus::GdiplusStartupInput tInput;
			Gdiplus::GdiplusStartup(&uToken, &tInput, nullptr);
		}

		ULONG_PTR uToken = 0;
	};

	CGdiPlusInit g_ArtGdiPlus;
}

void CMusic::Start()
{
	if (m_bRunning)
		return;

	m_bUnload = false;
	m_bRunning = true;

	m_Worker = std::thread([this] { WorkerMain(); });
}

void CMusic::Unload()
{
	m_bUnload = true;

	if (m_Worker.joinable())
	{
		if (m_Worker.get_id() == std::this_thread::get_id())
			m_Worker.detach();
		else
			m_Worker.join();
	}

	m_bRunning = false;

	if (m_pArtworkTex)
	{
		m_pArtworkTex->Release();
		m_pArtworkTex = nullptr;
	}
	m_uArtworkW = 0;
	m_uArtworkH = 0;
}

void CMusic::ResetBox()
{
	Vars::Menu::Music::Box.Value = WindowBox_t{};
}

void CMusic::WorkerMain()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	while (!m_bUnload)
	{
		PollOnce();
		Sleep(500);
	}

	CoUninitialize();
	m_bRunning = false;
}

void CMusic::PollOnce()
{
	if (!AcquireManager(2000))
		return;

	ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSession> pSession;
	if (FAILED(g_Manager->GetCurrentSession(pSession.ReleaseAndGetAddressOf())) || !pSession)
	{
		std::lock_guard lock(m_Mutex);
		m_bHasSession = false;
		return;
	}
	{
		std::lock_guard lock(m_Mutex);
		m_bHasSession = true;
	}

	ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSessionMediaProperties> pProps;
	if (!GetMediaPropsBlocking(pSession.Get(), pProps.GetAddressOf()))
		return;

	MusicInfo_t tInfo;

	ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSessionTimelineProperties> pTimeline;
	if (SUCCEEDED(pSession->GetTimelineProperties(pTimeline.ReleaseAndGetAddressOf())) && pTimeline)
	{
		ABI_F::TimeSpan tEnd = {};
		pTimeline->get_EndTime(&tEnd);
		tInfo.m_flDuration = tEnd.Duration > 0 ? double(tEnd.Duration) / 1e7 : 0.0; // 100ns units
		if (tInfo.m_flDuration > 0.0)
		{
			ABI_F::TimeSpan tPos = {};
			pTimeline->get_Position(&tPos);
			tInfo.m_flPosition = double(tPos.Duration) / 1e7;
		}
	}

	ComPtr<ABI_MC::IGlobalSystemMediaTransportControlsSessionPlaybackInfo> pPlayback;
	if (SUCCEEDED(pSession->GetPlaybackInfo(pPlayback.ReleaseAndGetAddressOf())) && pPlayback)
	{
		ABI_MC::GlobalSystemMediaTransportControlsSessionPlaybackStatus eStatus = ABI_MC::GlobalSystemMediaTransportControlsSessionPlaybackStatus_Closed;
		pPlayback->get_PlaybackStatus(&eStatus);
		tInfo.m_iStatus = (int)eStatus;
	}

	HSTRING hs = nullptr;
	if (SUCCEEDED(pProps->get_Title(&hs)))
	{
		tInfo.m_sTitle = ReadHString(hs);
		FreeHString(hs);
		hs = nullptr;
	}
	if (SUCCEEDED(pProps->get_Artist(&hs)))
	{
		tInfo.m_sArtist = ReadHString(hs);
		FreeHString(hs);
		hs = nullptr;
	}
	if (SUCCEEDED(pProps->get_AlbumTitle(&hs)))
	{
		tInfo.m_sAlbum = ReadHString(hs);
		FreeHString(hs);
		hs = nullptr;
	}

	tInfo.m_flFetchedAt = NowMonotonic();

	// artwork: refetch when the track metadata changed, or retry a previous failure after a short delay
	const std::string sKey = Narrow(tInfo.m_sTitle) + "\n" + Narrow(tInfo.m_sArtist) + "\n" + Narrow(tInfo.m_sAlbum);
	const double flNow = NowMonotonic();
	if (sKey != m_sLastArtworkKey || (m_bArtworkFailed && flNow >= m_flNextArtworkRetry))
	{
		std::vector<uint8_t> vArt;
		if (ReadArtworkBlocking(pProps.Get(), vArt))
		{
			m_sLastArtworkKey = sKey;
			m_bArtworkFailed = false;
			{
				std::lock_guard lock(m_Mutex);
				m_vArtworkBytes.swap(vArt);
			}
		}
		else
		{
			m_bArtworkFailed = true;
			m_flNextArtworkRetry = flNow + 10.0;
		}
	}

	{
		std::lock_guard lock(m_Mutex);
		m_tInfo = tInfo;
	}
}

void CMusic::UpdateArtwork(const std::vector<uint8_t>& vBytes)
{
	if (m_pArtworkTex)
	{
		m_pArtworkTex->Release();
		m_pArtworkTex = nullptr;
	}
	m_uArtworkW = 0;
	m_uArtworkH = 0;

	if (vBytes.empty() || !blur::device)
		return;

	const HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, vBytes.size());
	if (!hGlobal)
		return;

	if (void* pMem = GlobalLock(hGlobal); pMem)
	{
		memcpy(pMem, vBytes.data(), vBytes.size());
		GlobalUnlock(hGlobal);

		IStream* pStream = nullptr;
		if (SUCCEEDED(CreateStreamOnHGlobal(hGlobal, FALSE, &pStream)) && pStream)
		{
			auto pBitmap = std::make_unique<Gdiplus::Bitmap>(pStream);
			pStream->Release();
			if (pBitmap->GetLastStatus() == Gdiplus::Ok)
			{
				const UINT nWidth = pBitmap->GetWidth(), nHeight = pBitmap->GetHeight();
				if (nWidth && nHeight)
				{
					std::vector<uint8_t> vPixels(size_t(nWidth) * nHeight * 4);
					Gdiplus::BitmapData tData = {};
					Gdiplus::Rect tRect(0, 0, (INT)nWidth, (INT)nHeight);
					if (pBitmap->LockBits(&tRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &tData) == Gdiplus::Ok)
					{
						const auto* pScan0 = static_cast<const uint8_t*>(tData.Scan0);
						for (UINT y = 0; y < nHeight; y++)
							memcpy(vPixels.data() + size_t(y) * nWidth * 4, pScan0 + size_t(y) * tData.Stride, size_t(nWidth) * 4);
						pBitmap->UnlockBits(&tData);

						IDirect3DTexture9* pTexture = nullptr;
						HRESULT hr = blur::device->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTexture, nullptr);
						if (SUCCEEDED(hr) && pTexture)
						{
							D3DLOCKED_RECT tLockedRect = {};
							if (SUCCEEDED(pTexture->LockRect(0, &tLockedRect, nullptr, 0)))
							{
								for (UINT y = 0; y < nHeight; y++)
									memcpy(reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(tLockedRect.pBits) + uintptr_t(y) * tLockedRect.Pitch),
										vPixels.data() + size_t(y) * nWidth * 4, size_t(nWidth) * 4);
								pTexture->UnlockRect(0);

								m_pArtworkTex = pTexture;
								m_uArtworkW = nWidth;
								m_uArtworkH = nHeight;
							}
							else
								pTexture->Release();
						}
					}
				}
			}
		}
	}
	GlobalFree(hGlobal);
}

void CMusic::Draw()
{
	using namespace ImGui;

	if (!Vars::Menu::Music::Enabled.Value)
		return;

	MusicInfo_t tInfo;
	std::vector<uint8_t> vArt;
	bool bHasSession = false;
	{
		std::lock_guard lock(m_Mutex);
		tInfo = m_tInfo;
		vArt.swap(m_vArtworkBytes);
		bHasSession = m_bHasSession;
	}

	if (!vArt.empty())
		UpdateArtwork(vArt);

	if (!bHasSession || tInfo.m_sTitle.empty())
		return;

	// anchored playback-position tracking — the SMTC timeline position is refreshed
	// only coarsely by many players (often every few seconds), so re-anchoring every
	// frame to the same stale sample makes the bar jitter.
	double flPosition = tInfo.m_flPosition;
	if (tInfo.m_iStatus == 4 && tInfo.m_flFetchedAt > 0.0)
	{
		const std::string sKey = Narrow(tInfo.m_sTitle) + "\n" + Narrow(tInfo.m_sArtist) + "\n" + Narrow(tInfo.m_sAlbum);
		if (sKey != m_sAnchorKey)
		{
			m_flAnchorPos = tInfo.m_flPosition;
			m_flAnchorAt = tInfo.m_flFetchedAt;
			m_sAnchorKey = sKey;
		}
		else if (tInfo.m_flFetchedAt != m_flLastSampleAt)
		{
			const double flPredicted = m_flAnchorPos + (tInfo.m_flFetchedAt - m_flAnchorAt);
			if (tInfo.m_flPosition >= flPredicted - 0.25 || tInfo.m_flPosition < flPredicted - 10.0)
			{
				m_flAnchorPos = tInfo.m_flPosition;
				m_flAnchorAt = tInfo.m_flFetchedAt;
			}
		}
		m_flLastSampleAt = tInfo.m_flFetchedAt;
		flPosition = m_flAnchorPos + (NowMonotonic() - m_flAnchorAt);
	}
	else
	{
		m_sAnchorKey.clear();
		m_flAnchorPos = tInfo.m_flPosition;
		m_flAnchorAt = NowMonotonic();
		m_flLastSampleAt = tInfo.m_flFetchedAt;
	}
	if (flPosition < 0.0)
		flPosition = 0.0;
	if (tInfo.m_flDuration > 0.0 && flPosition > tInfo.m_flDuration)
		flPosition = tInfo.m_flDuration;

	const float flProgress = tInfo.m_flDuration > 0.0 ? float(flPosition / tInfo.m_flDuration) : 0.f;

	// draggable / resizable window, position & size persisted in Vars::Menu::Music::Box
	auto& tBoxVar = Vars::Menu::Music::Box;
	auto tWindowBox = FGet(tBoxVar, true);

	struct BoxStorage
	{
		WindowBox_t m_tBox;
		float m_flScale;
	};
	static BoxStorage s_tStorage = {};
	static bool s_bStorageSet = false;

	SetNextWindowSizeConstraints(ImVec2(H::Draw.Scale(180), H::Draw.Scale(64)), ImVec2(H::Draw.Scale(800), H::Draw.Scale(600)));
	if (!s_bStorageSet || tWindowBox != s_tStorage.m_tBox || H::Draw.Scale() != s_tStorage.m_flScale)
	{
		SetNextWindowPos(ImVec2(float(tWindowBox.x - tWindowBox.w / 2), float(tWindowBox.y)), ImGuiCond_Always);
		SetNextWindowSize(ImVec2(float(tWindowBox.w), float(tWindowBox.h)), ImGuiCond_Always);
	}

	const float flOpacity = std::clamp(Vars::Menu::Music::Opacity.Value, 0.05f, 1.f);

	PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(H::Draw.Scale(6), H::Draw.Scale(6)));

if (Begin("##MusicPlayer", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings))
		{
			const ImVec2 vWindowPos = GetWindowPos();
			const ImVec2 vWinSize = GetWindowSize();

			// same chrome as the binds / indicator overlays: soft accent shadow ->
			// dark body -> black / accent / black triple border (square)
			auto& pDL = *GetWindowDrawList();
			const ImVec2 vP3 = ImVec2(H::Draw.Scale(3.f), H::Draw.Scale(3.f));
			const ImVec2 vP2 = ImVec2(H::Draw.Scale(2.f), H::Draw.Scale(2.f));
			const ImVec2 vP1 = ImVec2(H::Draw.Scale(1.f), H::Draw.Scale(1.f));

			ImVec4 vAcc = F::Render.Accent;
			ImVec4 vSh = vAcc; vSh.w = (22.f / 255.f) * flOpacity;
			pDL.AddRectFilled(vWindowPos + vP3, vWindowPos + vWinSize + vP3, ImColor(vSh));
			vSh.w = (18.f / 255.f) * flOpacity;
			pDL.AddRectFilled(vWindowPos + vP2, vWindowPos + vWinSize + vP2, ImColor(vSh));

			pDL.AddRectFilled(vWindowPos, vWindowPos + vWinSize, ImColor(24.f / 255.f, 24.f / 255.f, 24.f / 255.f, (204.f / 255.f) * flOpacity));

			const ImU32 uBlack = IM_COL32(0, 0, 0, int(255 * flOpacity));
			vAcc.w = flOpacity;
			pDL.AddRect(vWindowPos - vP1, vWindowPos + vWinSize + vP1, uBlack);
			pDL.AddRect(vWindowPos, vWindowPos + vWinSize, ImColor(vAcc));
			pDL.AddRect(vWindowPos + vP1, vWindowPos + vWinSize - vP1, uBlack);

			tWindowBox.w = (int)vWinSize.x, tWindowBox.h = (int)vWinSize.y;
			tWindowBox.x = (int)(vWindowPos.x + tWindowBox.w / 2), tWindowBox.y = (int)vWindowPos.y;
			s_tStorage = { tWindowBox, H::Draw.Scale() };
			s_bStorageSet = true;
			FSet(tBoxVar, tWindowBox);

			const float flPad = GetStyle().WindowPadding.x;
			const ImVec2 vContentMin = GetWindowPos() + GetStyle().WindowPadding;
			const ImVec2 vContentSize = GetWindowSize() - GetStyle().WindowPadding * 2.f;

			// artwork (square on the left) or placeholder
			const float flArtSide = ImMin(vContentSize.x, vContentSize.y - H::Draw.Scale(22) - flPad);
			if (Vars::Menu::Music::ShowTitle.Value || Vars::Menu::Music::ShowArtist.Value)
			{
				if (m_pArtworkTex && m_uArtworkW && m_uArtworkH)
				{
					GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)m_pArtworkTex,
						vContentMin, vContentMin + ImVec2(flArtSide, flArtSide));
				}
				else
				{
					auto& pDLArt = *GetWindowDrawList();
					const ImVec2 vArtMax = vContentMin + ImVec2(flArtSide, flArtSide);
					pDLArt.AddRectFilled(vContentMin, vArtMax, IM_COL32(18, 18, 18, int(255 * flOpacity)));
					ImVec4 vAccArt = F::Render.Accent;
					vAccArt.w = flOpacity;
					pDLArt.AddRect(vContentMin, vArtMax, ImColor(vAccArt));
					if (F::Render.IconFont)
					{
						const float flIcon = F::Render.IconFont->LegacySize;
pDLArt.AddText(F::Render.IconFont, flIcon,
						ImVec2(vContentMin.x + (flArtSide - flIcon) / 2.f, vContentMin.y + (flArtSide - flIcon) / 2.f),
						ImColor(vAccArt), reinterpret_cast<const char*>(ICON_FA_MUSIC));
					}
				}

				// text column: rendered as window-relative items so the content is
				// always anchored to the window and clipped by it (no detached text)
				PushFont(F::Render.FontBold);
				PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, flOpacity));
				{
					const float flLineH = FCalcTextSize("A").y + H::Draw.Scale(2);

					const float flTextX = vContentMin.x + flArtSide + flPad;
					float flY = vContentMin.y;

					if (Vars::Menu::Music::ShowTitle.Value && !tInfo.m_sTitle.empty())
					{
						SetCursorScreenPos({ flTextX, flY });
						TextUnformatted(Narrow(tInfo.m_sTitle).c_str());
						flY += flLineH;
					}
					if (Vars::Menu::Music::ShowArtist.Value && !tInfo.m_sArtist.empty())
					{
						SetCursorScreenPos({ flTextX, flY });
						TextUnformatted(Narrow(tInfo.m_sArtist).c_str());
						flY += flLineH;
					}

					const char* szStatus = "Idle";
					if (tInfo.m_iStatus == 4)
						szStatus = "Playing";
					else if (tInfo.m_iStatus == 5)
						szStatus = "Paused";
					else if (tInfo.m_iStatus == 1 || tInfo.m_iStatus == 2)
						szStatus = "Buffering";

					auto fFormatTime = [](double flSec) -> std::string
					{
						if (flSec < 0.0)
							flSec = 0.0;
						const int iTotal = (int)flSec;
						const int iHours = iTotal / 3600;
						const int iMin = (iTotal % 3600) / 60;
						const int iSec = iTotal % 60;
						if (iHours)
							return std::format("{}:{:02d}:{:02d}", iHours, iMin, iSec);
						return std::format("{}:{:02d}", iMin, iSec);
					};

					// status word in accent (like the panel title), times in white
					ImVec4 vAccText = F::Render.Accent;
					vAccText.w = flOpacity;
					SetCursorScreenPos({ flTextX, flY });
					PushStyleColor(ImGuiCol_Text, vAccText);
					TextUnformatted(szStatus);
					PopStyleColor();
					if (tInfo.m_flDuration > 0.0)
					{
						const std::string sPos = fFormatTime(flPosition);
						const std::string sDur = fFormatTime(tInfo.m_flDuration);
						const float flStatusW = CalcTextSize(szStatus).x;
						SetCursorScreenPos({ flTextX + flStatusW + H::Draw.Scale(6), flY });
						TextUnformatted(std::format("{} / {}", sPos, sDur).c_str());
					}
				}
				PopFont();
				PopStyleColor();
			}

			// progress bar along the bottom (accent fill on a black track, both fading with the panel)
			if (Vars::Menu::Music::ShowProgress.Value)
			{
				const float flBarH = H::Draw.Scale(3);
				const ImVec2 vBarMin(vContentMin.x, vContentMin.y + vContentSize.y - flBarH);
				const ImVec2 vBarMax(GetWindowPos().x + vWinSize.x - GetStyle().WindowPadding.x, vBarMin.y + flBarH);
				auto& pDLBar = *GetWindowDrawList();
				pDLBar.AddRectFilled(vBarMin, vBarMax, IM_COL32(0, 0, 0, int(160 * flOpacity)));
				if (flProgress > 0.f)
				{
					const ImVec2 vFillMax(vBarMin.x + (vBarMax.x - vBarMin.x) * std::clamp(flProgress, 0.f, 1.f), vBarMax.y);
					ImVec4 vAccBar = F::Render.Accent;
					vAccBar.w = flOpacity;
					pDLBar.AddRectFilled(vBarMin, vFillMax, ImColor(vAccBar));
				}
			}

			End();
		}
	PopStyleVar(3);
}