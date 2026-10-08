#pragma once

#include "../../Utils/Macros/Macros.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct SourceBan_t
{
	std::string m_sName;
	std::string m_sState;
	std::string m_sReason;
	std::string m_sUnbanReason;
	uint64_t m_uBanTimestamp = 0;
	uint64_t m_uUnbanTimestamp = 0;
	std::string m_sServer;
};

struct SteamBanData_t
{
	uint64_t m_uSteamID64 = 0;
	uint64_t m_uLastUpdate = 0;

	bool m_bVACBanned = false;
	int m_iVACBans = 0;
	int m_iDaysSinceLastVAC = 0;

	bool m_bGameBanned = false;
	int m_iGameBans = 0;
	int m_iDaysSinceLastGame = 0;

	bool m_bCommunityBanned = false;
	std::string m_sEconomyBan;

	bool m_bHasActiveSourceBan = false;
	std::vector<SourceBan_t> m_vSourceBans;
};

// TF2BD (tf2_bot_detector) playerlist list, parsed from Core\Lists\playerlist.*.json
struct TF2BDList_t
{
	std::string m_sTitle;
	std::string m_sDescription;
	std::string m_sUpdateURL;
	std::unordered_map<uint32_t, std::vector<std::string>> m_mPlayers; // accountID -> attributes
};

class CSteamBans
{
public:
	void Run();
	void Check(uint32_t uAccountID);
	void OnMatchStart();
	void OnLocalSpawn();
	void OnFrame();
	void Unload();

	std::optional<SteamBanData_t> GetResult(uint32_t uAccountID);
	std::vector<std::string> GetTooltips(const SteamBanData_t& tBan) const;

	void LoadLists();
	void RefreshLists();
	void ScanTF2BDLists();
	void OnChatMessage(uint32_t uAccountID, const std::string& sText);
	bool HasTF2BD(uint32_t uAccountID);
	std::vector<std::string> GetTF2BDTooltips(uint32_t uAccountID);

private:
	void EnsureWorker();
	void WorkerMain();

	bool FetchURL(const std::string& sHost, const std::string& sPath, bool bSecure, std::string& sOut);
	void FetchSteamBans(const std::vector<uint32_t>& vAccountIDs);
	void FetchSourceBans(const std::vector<uint32_t>& vAccountIDs);
	bool FetchTF2BDList(const std::string& sKey);
	void SaveTF2BDList(const std::string& sKey, const TF2BDList_t& tList);

	void SaveCache();
	void LoadCache();

	std::thread m_Worker;
	std::atomic<bool> m_bRunning = false;
	std::atomic<bool> m_bUnload = false;
	std::atomic<bool> m_bCacheDirty = false;
	std::mutex m_Mutex;
	std::condition_variable m_Cond;
	std::unordered_set<uint32_t> m_vPending;
	std::unordered_map<uint32_t, SteamBanData_t> m_mBans;
	std::unordered_set<uint32_t> m_mAlerted;
	std::vector<uint32_t> m_vAlerts;
	std::unordered_set<uint32_t> m_mMarkedAlerted; // marked players already alerted this session (from the playerlist, not ban data)
	std::vector<uint32_t> m_vMarkedAlerts;
	std::atomic<bool> m_bMatchCheckPending = false;
	bool m_bCacheLoaded = false;

	std::unordered_map<std::string, TF2BDList_t> m_mTF2BDLists; // keyed by playerlist file stem
	std::vector<std::string> m_vTF2BDPending;
	std::vector<std::string> m_vListMessages; // status messages queued for the frame thread
	bool m_bListsLoaded = false;
	bool m_bMarksScanned = false;
};

ADD_FEATURE(CSteamBans, SteamBans);