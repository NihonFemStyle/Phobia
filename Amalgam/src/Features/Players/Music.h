#pragma once

#include "../../Utils/Macros/Macros.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <d3d9.h>

// Now playing info pulled from Windows Global System Media Transport Controls (SMTC),
// the same API that powers the Windows 11 media flyout. Works for any app that reports
// session-based media (Spotify, Chrome/MSE, browsers, video players, etc).
struct MusicInfo_t
{
	std::wstring m_sTitle;
	std::wstring m_sArtist;
	std::wstring m_sAlbum;

	double m_flPosition = 0.0;  // seconds
	double m_flDuration = 0.0;  // seconds
	double m_flFetchedAt = 0.0; // seconds since a steady clock epoch, for interpolation

	int m_iStatus = 0; // GlobalSystemMediaTransportControlsSessionPlaybackStatus value
};

class CMusic
{
public:
	void Start();
	void Unload();

	void Draw();

	void ResetBox();

private:
	void WorkerMain();

	// blocking SMTC helpers (run on the worker thread)
	void PollOnce();

	// render-thread only
	void UpdateArtwork(const std::vector<uint8_t>& vBytes);

	// smooth playback-position tracking (render-thread only). The SMTC timeline position
	// is refreshed only coarsely by most players (often every few seconds), so we anchor a
	// monotonic wall clock to the last trustworthy sample and re-anchor when the source
	// catches up or jumps (seek / track change).
	double m_flAnchorPos = 0.0;
	double m_flAnchorAt = 0.0;
	double m_flLastSampleAt = -1.0;
	std::string m_sAnchorKey;

	std::thread m_Worker;
	std::atomic<bool> m_bRunning = false;
	std::atomic<bool> m_bUnload = false;

	std::mutex m_Mutex;
	MusicInfo_t m_tInfo;
	std::vector<uint8_t> m_vArtworkBytes; // newest artwork (PNG/JPEG) waiting on the frame thread
	bool m_bHasSession = false;            // a media session exists right now

	// artwork caching on the worker thread
	std::string m_sLastArtworkKey;
	double m_flNextArtworkRetry = 0.0;
	bool m_bArtworkFailed = false;

	// render-thread texture
	IDirect3DTexture9* m_pArtworkTex = nullptr;
	uint32_t m_uArtworkW = 0, m_uArtworkH = 0;
};

ADD_FEATURE(CMusic, Music);