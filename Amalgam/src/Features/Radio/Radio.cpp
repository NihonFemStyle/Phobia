#include "Radio.h"

#include <thread>

static std::array<std::string, RADIO_MAX> chans = {
	"https://azura.drmnbss.org:8000/radio.mp3",               // DNB
	"https://energy1058.radioca.st/stream",                   // JUNGLE
	"https://radiorecord.hostingradio.ru/liquidfunk96.aacp",  // LIQUID DNB
	"https://stream.hardcoreradio.nl:9000/hcr.ogg",           // HARDCORE
	"https://schranz.in/schranz",                             // TECHNO
	"https://radiorecord.hostingradio.ru/rapclassics96.aacp", // CLASSIC RAP
	"https://radio.plaza.one/ogg",                            // VAPORWAVE
};

static BOOL BASS_INIT_ONCE()
{
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
		return FALSE;

	if (!BASS_Init(-1, 44100, 0, nullptr, nullptr))
		return FALSE;

	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
	BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0);

	return TRUE;
}

static void CALLBACK StatusProc(const void* buffer, DWORD length, void* user)
{
	if (buffer && !length && (std::uintptr_t)user == BASS::request_num)
	{
		// got HTTP/ICY tags, and this is still the current request
	}
}

static VOID BASS_OPEN_STREAM(const char* pszUrl)
{
	auto r = ++BASS::request_num;
	BASS_StreamFree(BASS::stream_handle);
	HSTREAM c = BASS_StreamCreateURL(pszUrl, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, StatusProc, (void*)r);
	if (r != BASS::request_num)
	{
		if (c)
			BASS_StreamFree(c);
		return;
	}
	BASS::stream_handle = c;
}

static void CALLBACK MetaSync(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	// metadata synchronized, nothing to display for now
}

static void CALLBACK EndSync(HSYNC handle, DWORD channel, DWORD data, void* user)
{
}

static std::string string_after_delim(std::string const& str, std::string const& delim)
{
	return str.substr(str.find(delim) + delim.size());
}

static VOID BASS_PLAY_STREAM()
{
	int iBuffer = BASS_StreamGetFilePosition(BASS::stream_handle, BASS_FILEPOS_CONNECTED);
	QWORD progress = BASS_StreamGetFilePosition(BASS::stream_handle, BASS_FILEPOS_BUFFER) * 100 / BASS_StreamGetFilePosition(BASS::stream_handle, BASS_FILEPOS_END);

	if (progress > 75 || !iBuffer)
	{
		BASS_ChannelSetSync(BASS::stream_handle, BASS_SYNC_META, 0, &MetaSync, 0);       // Shoutcast
		BASS_ChannelSetSync(BASS::stream_handle, BASS_SYNC_OGG_CHANGE, 0, &MetaSync, 0); // Icecast/OGG
		BASS_ChannelSetSync(BASS::stream_handle, BASS_SYNC_END, 0, &EndSync, 0);
		BASS_ChannelPlay(BASS::stream_handle, FALSE);
	}
}

void RadioManager::RunRadioLoop()
{
	while (true)
	{
		if (G::Unload)
			break;

		if (BASS::bass_init == FALSE)
			continue;

		int wish_chan = Vars::Radio::Station.Value;

		if (Vars::Radio::Enabled.Value)
		{
			if (m_current_channel != wish_chan || m_need_reinit)
			{
				m_need_reinit = false;
				BASS_Start();
				BASS_OPEN_STREAM(chans[wish_chan].c_str());
				SDK::Output("Radio", std::format("Stream {} - {} opened.", wish_chan, chans[wish_chan]).c_str(), INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_DEBUG);
				m_current_channel = wish_chan;
			}

			float vol = Vars::Radio::Volume.Value / 100.f;
			if (Vars::Radio::ScaleToGameVolume.Value && m_pVolume)
				vol = Math::RemapVal(vol, 0.f, 1.f, 0.f, m_pVolume->GetFloat());

			BASS_ChannelSetAttribute(BASS::stream_handle, BASS_ATTRIB_VOL, vol);
			BASS_PLAY_STREAM();
			Sleep(50);
		}
		else
		{
			m_need_reinit = true;
			BASS_StreamFree(BASS::stream_handle);
			Sleep(500);
		}
	}
}

bool RadioManager::StartupRadio()
{
	BASS::bass_lib_handle = BASS::bass_lib.LoadFromMemory((void*)bass_dll_image, sizeof(bass_dll_image));
	if (BASS::bass_lib_handle == NULL)
		return false;

	if (BASS_INIT_ONCE())
	{
		BASS::bass_init = TRUE;
		SDK::Output("Radio", "BASS initialized!", INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_DEBUG);
	}
	else
		return false;

	m_pVolume = H::ConVars.FindVar("volume");

	auto thread = std::thread(&RadioManager::RunRadioLoop, this);
	thread.detach();

	return true;
}