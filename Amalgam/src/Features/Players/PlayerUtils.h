#pragma once
#include "../../SDK/SDK.h"

#define DEFAULT_TAG 0
#define IGNORED_TAG (DEFAULT_TAG-1)
#define CHEATER_TAG (IGNORED_TAG-1)
#define FRIEND_TAG (CHEATER_TAG-1)
#define PARTY_TAG (FRIEND_TAG-1)
#define F2P_TAG (PARTY_TAG-1)
#define SUSPECTED_CHEATER_TAG (F2P_TAG-1)
#define SUSPICIOUS_TAG (SUSPECTED_CHEATER_TAG-1)
#define EXPLOITER_TAG (SUSPICIOUS_TAG-1)
#define RACIST_TAG (EXPLOITER_TAG-1)
#define BLACKLISTED_TAG (RACIST_TAG-1)
#define VAC_BAN_TAG (BLACKLISTED_TAG-1)
#define GAME_BAN_TAG (VAC_BAN_TAG-1)
#define SOURCE_BAN_TAG (GAME_BAN_TAG-1)
#define PEDO_TAG (SOURCE_BAN_TAG-1)
#define TAG_COUNT (-SOURCE_BAN_TAG)

#define LOCAL "Local"
#define FRIEND "Friend"
#define PARTY "Party"
#define ENEMY "Enemy"
#define TEAMMATE "Teammate"
#define PLAYER "Player"

struct ListPlayer
{
	std::string m_sName;
	uint32_t m_uAccountID;
	int m_iUserID;
	int m_iTeam;
	bool m_bAlive;
	bool m_bLocal;
	bool m_bFake;
	bool m_bFriend;
	bool m_bParty;
	bool m_bF2P;
	int m_iLevel;
	int m_iParty;
};

struct PriorityLabel_t
{
	std::string m_sName = "";
	Color_t m_tColor = {};
	int m_iPriority = 0;

	bool m_bLabel = false;
	bool m_bAssignable = true;
	bool m_bLocked = false; // don't allow it to be removed
};

Enum(NameType, None = 0, Local = 1 << 0, Friend = 1 << 1, Party = 1 << 2, Player = 1 << 3, Custom = 1 << 4, Privacy = Local | Friend | Party | Player);

class CPlayerlistUtils
{
public:
	std::unordered_map<uint32_t, std::vector<int>> m_mPlayerTags = {};
	std::unordered_map<uint32_t, std::string> m_mPlayerAliases = {};

	std::vector<PriorityLabel_t> m_vTags = {
		{ "Default", { 200, 200, 200, 255 }, 0, false, false, true },
		{ "Ignored", { 200, 200, 200, 255 }, -1, false, true, true },
		{ "Cheater", { 255, 100, 100, 255 }, 1, false, true, true },
		{ "Friend", { 100, 255, 100, 255 }, 0, true, false, true },
		{ "Party", { 100, 100, 255, 255 }, 0, true, false, true },
		{ "F2P", { 255, 255, 255, 255 }, 0, true, false, true },
		{ "Suspected Cheater", { 255, 170, 40, 255 }, 1, false, false, true },
		{ "Suspicious", { 255, 215, 110, 255 }, 0, false, false, true },
		{ "Exploiter", { 255, 130, 200, 255 }, 1, false, false, true },
		{ "Racist", { 200, 120, 255, 255 }, 1, false, false, true },
		{ "Blacklisted", { 255, 50, 50, 255 }, 2, false, false, true },
		{ "VAC Banned", { 255, 90, 90, 255 }, 0, true, false, true },
		{ "Game Banned", { 200, 160, 60, 255 }, 0, true, false, true },
		{ "SourceBanned", { 180, 130, 255, 255 }, 0, true, false, true }
	};

	std::vector<ListPlayer> m_vPlayerCache = {};
	std::unordered_map<uint32_t, ListPlayer> m_mPriorityCache = {};
	std::unordered_map<uint32_t, std::string> m_mPlayerNames = {};

	bool m_bLoad = true;
	bool m_bSave = false;
	bool m_bPlayerlistLoaded = false;
	bool m_bDatabaseLoaded = false;

	void RequestNames(const std::vector<uint32_t>& vAccountIDs);
	void PollNames();

private:
	std::vector<int> m_vDummy = {};
	std::unordered_map<uint32_t, std::string> m_mFakerNames = {};
	std::unordered_map<uint32_t, int> m_vNamesPending = {}; // accountID -> name polls remaining
	const char* GetStreamerName(uint32_t uAccountID);

public:
	// the read-only curated masterlist layer (Database.json), takes precedence over local tags
	std::unordered_map<uint32_t, std::vector<int>> m_mDatabaseTags = {};

	void Store();

	uint32_t GetAccountID(int iIndex);
	int GetIndex(uint32_t uAccountID);

	PriorityLabel_t* GetTag(int iID);
	int GetTag(const std::string& sTag);
	inline static bool IsProtectedMark(int iTag)
	{
		return iTag == SUSPECTED_CHEATER_TAG || iTag == SUSPICIOUS_TAG || iTag == EXPLOITER_TAG
			|| iTag == RACIST_TAG || iTag == BLACKLISTED_TAG || iTag == VAC_BAN_TAG
			|| iTag == GAME_BAN_TAG || iTag == SOURCE_BAN_TAG || iTag == PEDO_TAG;
	}
	static uint32_t SteamIDToAccountID(const std::string& sSteamID);
	inline int TagToIndex(int iTag)
	{
		if (iTag <= 0)
			iTag = -iTag;
		else
			iTag += TAG_COUNT;
		return iTag;
	}
	inline int IndexToTag(int iID)
	{
		if (iID <= TAG_COUNT)
			iID = -iID;
		else
			iID -= TAG_COUNT;
		return iID;
	}

	void AddTag(uint32_t uAccountID, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	void AddTag(uint32_t uAccountID, int iID, bool bSave = true, const char* sName = nullptr);
	void AddTag(int iIndex, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	void AddTag(int iIndex, int iID, bool bSave = true, const char* sName = nullptr);
	void RemoveTag(uint32_t uAccountID, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	void RemoveTag(uint32_t uAccountID, int iID, bool bSave = true, const char* sName = nullptr);
	void RemoveTag(int iIndex, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	void RemoveTag(int iIndex, int iID, bool bSave = true, const char* sName = nullptr);
	bool HasTags(uint32_t uAccountID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	bool HasTags(uint32_t uAccountID);
	bool HasTags(int iIndex, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	bool HasTags(int iIndex);
	bool HasTag(uint32_t uAccountID, int iID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	bool HasTag(uint32_t uAccountID, int iID);
	bool HasTag(int iIndex, int iID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags);
	bool HasTag(int iIndex, int iID);

	int GetPriority(uint32_t uAccountID, bool bCache = true);
	int GetPriority(int iIndex, bool bCache = true);
	PriorityLabel_t* GetSignificantTag(uint32_t uAccountID, int iMode = 1); // iMode: 0 - Priorities & Labels, 1 - Priorities, 2 - Labels
	PriorityLabel_t* GetSignificantTag(int iIndex, int iMode = 1); // iMode: 0 - Priorities & Labels, 1 - Priorities, 2 - Labels
	// tags that drive significance/priority: database marks when the account is in Database.json, else local marks
	const std::vector<int>& GetPriorityTags(uint32_t uAccountID);
	bool IsIgnored(uint32_t uAccountID);
	bool IsIgnored(int iIndex);
	bool IsPrioritized(uint32_t uAccountID);
	bool IsPrioritized(int iIndex);

	int GetNameType(int iIndex);
	int GetNameType(uint32_t uAccountID);
	const char* GetPlayerName(int iIndex, const char* sDefault, int* pType = nullptr);
	const char* GetPlayerName(uint32_t uAccountID, const char* sDefault, int* pType = nullptr);
	const char* GetPlayerName(int iIndex);
	const char* GetPlayerName(uint32_t uAccountID);

	std::vector<int>& GetPlayerTags(uint32_t uAccountID) { return m_mPlayerTags.contains(uAccountID) ? m_mPlayerTags[uAccountID] : m_vDummy; }
	std::string* GetPlayerAlias(uint32_t uAccountID) { return m_mPlayerAliases.contains(uAccountID) ? &m_mPlayerAliases[uAccountID] : nullptr; }

	bool InDatabase(uint32_t uAccountID);
	// union of local + database marks (database first), used for alerts/listing
	std::vector<int> GetEffectiveTags(uint32_t uAccountID);
	// any significant mark (cheater or "protected" marks) in the effective tags
	bool HasReportableMark(uint32_t uAccountID);
};

ADD_FEATURE(CPlayerlistUtils, PlayerUtils);