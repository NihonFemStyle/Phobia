#include "PhobiaAssets.h"
#include "bytearray.h"

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <memory>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")

namespace Phobia
{
	const unsigned char* PoppinsFont = poppin_font;
	std::size_t PoppinsFontSize = sizeof poppin_font;
	const unsigned char* IconFont = icon_font;
	std::size_t IconFontSize = sizeof icon_font;

	static Textures tTextures = {};

	Textures& tex()
	{
		return tTextures;
	}

	// decode a PNG from memory into a premultiplied A8R8G8B8 D3D9 texture.
	// optional pTintARGB (0xAARRGGBB, nullptr = keep source colours): every pixel's
	// RGB is replaced by the tint while its alpha is preserved (used to rebake the
	// black logo_one wordmark into a brighter steel grey).
	static bool CreateTextureFromPNG(IDirect3DDevice9* pDevice, const unsigned char* pPNG, std::size_t nSize, IDirect3DTexture9** ppOut, const unsigned int* pTintARGB = nullptr)
	{
		if (!pDevice || !pPNG || !ppOut || nSize == 0)
			return false;

		static ULONG_PTR g_gdiplusToken = 0;
		static bool bStarted = false;
		if (!bStarted)
		{
			Gdiplus::GdiplusStartupInput tInput = {};
			Gdiplus::GdiplusStartup(&g_gdiplusToken, &tInput, nullptr);
			bStarted = true;
		}

		if (const HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize); hGlobal)
		{
			if (void* pMem = GlobalLock(hGlobal); pMem)
			{
				memcpy(pMem, pPNG, nSize);
				GlobalUnlock(hGlobal);

				IStream* pStream = nullptr;
				if (SUCCEEDED(CreateStreamOnHGlobal(hGlobal, FALSE, &pStream)) && pStream)
				{
					std::unique_ptr<Gdiplus::Bitmap> pBitmap = std::make_unique<Gdiplus::Bitmap>(pStream);
					pStream->Release();
					GlobalFree(hGlobal);
					if (pBitmap->GetLastStatus() == Gdiplus::Ok)
					{
						const UINT nWidth = pBitmap->GetWidth(), nHeight = pBitmap->GetHeight();
						if (!nWidth || !nHeight)
							return false;

						std::vector<unsigned char> vBuffer(std::size_t(nWidth) * nHeight * 4);
						Gdiplus::BitmapData tData = {};
						Gdiplus::Rect tRect(0, 0, int(nWidth), int(nHeight));
						if (pBitmap->LockBits(&tRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &tData) != Gdiplus::Ok)
							return false;

						const auto* pScan0 = static_cast<const unsigned char*>(tData.Scan0);
						for (UINT y = 0; y < nHeight; y++)
							memcpy(vBuffer.data() + std::size_t(y) * nWidth * 4, pScan0 + std::size_t(y) * tData.Stride, std::size_t(nWidth) * 4);
						pBitmap->UnlockBits(&tData);

						IDirect3DTexture9* pTexture = nullptr;
						if (FAILED(pDevice->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTexture, nullptr)))
							return false;

						D3DLOCKED_RECT tLockedRect = {};
						if (FAILED(pTexture->LockRect(0, &tLockedRect, nullptr, 0)))
						{
							pTexture->Release();
							return false;
						}

						for (UINT y = 0; y < nHeight; y++)
						{
							auto* pDst = reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(tLockedRect.pBits) + uintptr_t(y) * tLockedRect.Pitch);
							const auto* pSrc = vBuffer.data() + std::size_t(y) * nWidth * 4;
							for (UINT x = 0; x < nWidth; x++)
							{
								const unsigned int nA = pSrc[3];
								unsigned char nR = pSrc[2], nG = pSrc[1], nB = pSrc[0];
								if (pTintARGB)
								{
									nR = unsigned char((*pTintARGB >> 16) & 0xFF);
									nG = unsigned char((*pTintARGB >> 8) & 0xFF);
									nB = unsigned char(*pTintARGB & 0xFF);
								}
								pDst[0] = unsigned char(unsigned int(nB) * nA / 255u);
								pDst[1] = unsigned char(unsigned int(nG) * nA / 255u);
								pDst[2] = unsigned char(unsigned int(nR) * nA / 255u);
								pDst[3] = unsigned char(nA);
								pDst += 4;
								pSrc += 4;
							}
						}

						pTexture->UnlockRect(0);
						*ppOut = pTexture;
						return true;
					}
				}
			}
			GlobalFree(hGlobal);
		}
		return false;
	}

	void Textures::load(IDirect3DDevice9* pDevice)
	{
		if (!pDevice)
			return;
		if (!bg)
			CreateTextureFromPNG(pDevice, bg_one, sizeof bg_one, &bg);
		if (!logoWordmark)
		{
			// the source wordmark is flat black (50,50,50); rebake to a brighter steel grey
			static const unsigned int uWordmarkTint = 0xFF828282;
			CreateTextureFromPNG(pDevice, logo_one, sizeof logo_one, &logoWordmark, &uWordmarkTint);
		}
		if (!logoMark)
			CreateTextureFromPNG(pDevice, logo_two, sizeof logo_two, &logoMark);
		if (!avatar)
			CreateTextureFromPNG(pDevice, user, sizeof user, &avatar);
	}

	void Textures::release()
	{
		if (bg) { bg->Release(); bg = nullptr; }
		if (logoWordmark) { logoWordmark->Release(); logoWordmark = nullptr; }
		if (logoMark) { logoMark->Release(); logoMark = nullptr; }
		if (avatar) { avatar->Release(); avatar = nullptr; }
	}
}