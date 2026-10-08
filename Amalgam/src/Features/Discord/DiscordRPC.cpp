#include "DiscordRPC.h"

#include "../../SDK/SDK.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <ctime>
#include <format>

// Discord Rich Presence via the local Discord client IPC (\\?\pipe\discord-ipc-N).
// A background worker owns the named pipe connection, handshake and heartbeat while
// the game thread feeds it an activity snapshot once a second.

#define DISCORD_APP_ID "1034669614711447602"
#define DISCORD_BUTTON_URL "https://jvnkbin.moe"

void CDiscordRPC::Start()
{
	if (m_bRunning)
		return;
	m_bRunning = true;
	m_Worker = std::thread([this] { WorkerMain(); });
}

void CDiscordRPC::Unload()
{
	if (!m_bRunning)
		return;
	m_bUnload = true;
	if (m_Worker.joinable())
		m_Worker.join();
	m_bRunning = false;
}

void CDiscordRPC::Run()
{
	const bool bEnabled = Vars::Visuals::UI::DiscordRPC.Value;
	if (bEnabled != m_bEnabled)
		m_bEnabled = bEnabled;
	if (!bEnabled)
		return;

	static Timer tTimer = {};
	if (!tTimer.Run(0.5f))
		return;

	Snapshot_t tShot = {};
	if (I::EngineClient->IsInGame())
	{
		const char* sLevel = I::EngineClient->GetLevelName();
		if (sLevel && sLevel[0])
		{
			tShot.m_bActive = true;

			if (m_sCurrentMap != sLevel)
			{
				m_sCurrentMap = sLevel;
				m_uStartTime = uint64_t(std::time(nullptr));
			}
			tShot.m_uStartTime = m_uStartTime;

			tShot.m_sDetails = std::format("{} - {}", GetMode(), FormatMap(sLevel));

			const int iPartySize = std::clamp(H::Entities.GetPartySize(), 1, 6);
			tShot.m_bInParty = iPartySize > 1;
			tShot.m_iPartySize = iPartySize;
			tShot.m_sState = tShot.m_bInParty ? "Travelling with other mercs" : "Playing Solo";

			uint32_t uAccountID = 0;
			player_info_t tInfo = {};
			if (I::EngineClient->GetPlayerInfo(I::EngineClient->GetLocalPlayer(), &tInfo))
				uAccountID = tInfo.friendsID;
			tShot.m_sPartyID = std::format("Phobia-{}", uAccountID);

			auto pLocal = H::Entities.GetLocal();
			const int iClass = pLocal ? pLocal->m_iClass() : TF_CLASS_UNDEFINED;
			static const std::array<const char*, 10> szKeys = { "", "scout", "sniper", "soldier", "demo", "medic", "heavy", "pyro", "spy", "engineer" };
			if (iClass > 0 && iClass < 10)
			{
				tShot.m_sSmallImage = szKeys[iClass];
				tShot.m_sSmallText = SDK::GetClassByIndex(iClass, false);
			}
		}
	}
	else
		m_sCurrentMap.clear();

	{
		std::lock_guard tLock(m_SnapshotMutex);
		if (m_Snapshot.m_bActive != tShot.m_bActive
			|| m_Snapshot.m_bInParty != tShot.m_bInParty
			|| m_Snapshot.m_iPartySize != tShot.m_iPartySize
			|| m_Snapshot.m_uStartTime != tShot.m_uStartTime
			|| m_Snapshot.m_sDetails != tShot.m_sDetails
			|| m_Snapshot.m_sState != tShot.m_sState
			|| m_Snapshot.m_sSmallImage != tShot.m_sSmallImage
			|| m_Snapshot.m_sSmallText != tShot.m_sSmallText
			|| m_Snapshot.m_sPartyID != tShot.m_sPartyID)
		{
			m_Snapshot = std::move(tShot);
			m_bDirty = true;
		}
	}
}

void CDiscordRPC::WorkerMain()
{
	while (!m_bUnload)
	{
		if (m_bEnabled)
		{
			if (!Connect() || !Handshake())
			{
				CloseConnection();
				const uint64_t uStart = GetTickCount64();
				while (!m_bUnload && GetTickCount64() - uStart < 5000)
					Sleep(50);
				continue;
			}

			m_bDirty = true;
			uint64_t uNextHeartbeat = GetTickCount64() + 15000;
			unsigned long long iRefreshes = 0;
			while (!m_bUnload && m_bEnabled && m_hPipe != INVALID_HANDLE_VALUE)
			{
				if (m_bDirty.exchange(false) || ++iRefreshes % 60 == 0)
					SendActivity();

				if (GetTickCount64() >= uNextHeartbeat)
				{
					SendFrame(Opcode_Heartbeat, "");
					uNextHeartbeat = GetTickCount64() + 15000;
				}

				DWORD dwAvail = 0, dwLeft = 0;
				if (!PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, &dwAvail, &dwLeft))
					break;
				if (dwAvail)
				{
					std::string sFrame;
					if (!ReadFrame(sFrame))
						break;
				}

				Sleep(250);
			}
			CloseConnection();
		}
		else
		{
			if (m_hPipe != INVALID_HANDLE_VALUE)
			{
				ClearActivity();
				CloseConnection();
			}
			Sleep(200);
		}
	}
}

bool CDiscordRPC::Connect()
{
	for (int i = 0; i < 10; i++)
	{
		m_hPipe = CreateFileW(std::format(L"\\\\.\\pipe\\discord-ipc-{}", i).c_str(),
			GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (m_hPipe != INVALID_HANDLE_VALUE)
			return true;
	}
	return false;
}

bool CDiscordRPC::Handshake()
{
	SendFrame(Opcode_Handshake, std::format("{{\"v\":1,\"client_id\":\"{}\"}}", DISCORD_APP_ID));
	std::string sResponse;
	return ReadFrame(sResponse);
}

void CDiscordRPC::SendFrame(uint32_t uOpcode, const std::string& sPayload)
{
	if (m_hPipe == INVALID_HANDLE_VALUE)
		return;

	DWORD dwWritten = 0;
	WriteFile(m_hPipe, &uOpcode, sizeof(uOpcode), &dwWritten, nullptr);

	uint32_t uLength = uint32_t(sPayload.size());
	WriteFile(m_hPipe, &uLength, sizeof(uLength), &dwWritten, nullptr);

	if (!sPayload.empty())
		WriteFile(m_hPipe, sPayload.data(), uLength, &dwWritten, nullptr);
}

bool CDiscordRPC::ReadFrame(std::string& sOut)
{
	if (m_hPipe == INVALID_HANDLE_VALUE)
		return false;

	uint32_t uOpcode = 0, uLength = 0;
	DWORD dwRead = 0;
	if (!ReadFile(m_hPipe, &uOpcode, sizeof(uOpcode), &dwRead, nullptr) || dwRead != sizeof(uOpcode))
		return false;
	if (!ReadFile(m_hPipe, &uLength, sizeof(uLength), &dwRead, nullptr) || dwRead != sizeof(uLength))
		return false;

	sOut.resize(uLength);
	if (uLength && (!ReadFile(m_hPipe, sOut.data(), uLength, &dwRead, nullptr) || dwRead != uLength))
		return false;
	return true;
}

std::string CDiscordRPC::BuildActivityJSON(const Snapshot_t& tSnapshot)
{
	if (!tSnapshot.m_bActive)
		return "\"activity\":{}";

	std::string sJSON = std::format(
		"\"activity\":{{\"details\":\"{}\",\"state\":\"{}\"",
		tSnapshot.m_sDetails, tSnapshot.m_sState);

	if (tSnapshot.m_uStartTime)
		sJSON += std::format(",\"timestamps\":{{\"start\":{}}}", tSnapshot.m_uStartTime);

	sJSON += ",\"assets\":{\"large_image\":\"logo\",\"large_text\":\"Phobia\"";
	if (!tSnapshot.m_sSmallImage.empty())
		sJSON += std::format(",\"small_image\":\"{}\",\"small_text\":\"{}\"", tSnapshot.m_sSmallImage, tSnapshot.m_sSmallText);
	sJSON += "}";

	if (tSnapshot.m_bInParty)
		sJSON += std::format(",\"party\":{{\"id\":\"{}\",\"size\":[{},6]}}", tSnapshot.m_sPartyID, tSnapshot.m_iPartySize);

	sJSON += ",\"buttons\":[{\"label\":\"Phobia\",\"url\":\"" DISCORD_BUTTON_URL "\"}]";
	sJSON += ",\"metadata\":{\"button_urls\":[\"" DISCORD_BUTTON_URL "\"]}";
	sJSON += ",\"instance\":true}";

	return sJSON;
}

void CDiscordRPC::SendActivity()
{
	Snapshot_t tSnapshot;
	{
		std::lock_guard tLock(m_SnapshotMutex);
		tSnapshot = m_Snapshot;
	}

	SendFrame(Opcode_Frame, std::format(
		"{{\"cmd\":\"SET_ACTIVITY\",\"args\":{{\"pid\":{},{}}},\"nonce\":\"Phobia{}\"}}",
		GetCurrentProcessId(), BuildActivityJSON(tSnapshot), m_iNonce++));
}

void CDiscordRPC::ClearActivity()
{
	SendFrame(Opcode_Frame, std::format(
		"{{\"cmd\":\"SET_ACTIVITY\",\"args\":{{\"pid\":{},\"activity\":{{}}}},\"nonce\":\"Phobia{}\"}}",
		GetCurrentProcessId(), m_iNonce++));
}

void CDiscordRPC::CloseConnection()
{
	if (m_hPipe != INVALID_HANDLE_VALUE)
		CloseHandle(m_hPipe);
	m_hPipe = INVALID_HANDLE_VALUE;
}

std::string CDiscordRPC::GetMode()
{
	auto pRules = I::TFGameRules();
	if (!pRules)
		return "Community";

	const int iGroup = pRules->GetCurrentMatchGroup();
	if (iGroup >= k_eTFMatchGroup_MvM_First && iGroup <= k_eTFMatchGroup_MvM_Last)
		return "MvM";
	if (iGroup >= k_eTFMatchGroup_Ladder_First && iGroup <= k_eTFMatchGroup_Ladder_Last)
		return "Competitive";
	if (iGroup >= k_eTFMatchGroup_Casual_First && iGroup <= k_eTFMatchGroup_Casual_Last)
		return "Casuals";
	if (pRules->m_bPlayingMannVsMachine())
		return "MvM";
	return "Community";
}

std::string CDiscordRPC::FormatMap(const char* sLevel)
{
	std::string sMap = sLevel ? sLevel : "";
	if (const auto iSlash = sMap.rfind('/'); iSlash != std::string::npos)
		sMap = sMap.substr(iSlash + 1);
	if (const auto iDot = sMap.find(".bsp"); iDot != std::string::npos)
		sMap.erase(iDot);

	static const std::array<const char*, 14> vPrefixes = {
		"pass_", "arena_", "plr_", "koth_", "hightower_", "mvm_", "ctf_", "vsh_", "cp_", "pl_", "tc_", "sd_", "rd_", "pd_"
	};
	for (const char* sPrefix : vPrefixes)
	{
		const size_t iLen = std::strlen(sPrefix);
		if (sMap.size() > iLen && sMap.compare(0, iLen, sPrefix) == 0)
		{
			sMap.erase(0, iLen);
			break;
		}
	}

	bool bWordStart = true;
	for (auto& c : sMap)
	{
		if (c == '_')
		{
			c = ' ';
			bWordStart = true;
		}
		else if (std::isalpha((unsigned char)c))
		{
			c = bWordStart ? char(std::toupper((unsigned char)c)) : char(std::tolower((unsigned char)c));
			bWordStart = false;
		}
		else
			bWordStart = false;
	}

	return sMap;
}