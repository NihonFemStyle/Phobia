#include "PlayerUtils.h"

#include "../ImGui/Menu/Menu.h"
#include "../Output/Output.h"
#include "../../SDK/Definitions/Types.h"

#include <cctype>

namespace
{
	constexpr std::array<std::string_view, 32> kFakerNames = {
		"Sprinkles", "Chip", "Waffle", "Biscuit", "Muffin", "Bagel", "Croissant", "Donut",
		"Pretzel", "Pickle", "Gravy", "Sausage", "Bacon", "Tofu", "Noodle", "Ramen",
		"Bulgogi", "Kebab", "Falafel", "Hummus", "Taco", "Burrito", "Quesadilla", "Nachos",
		"Paella", "Risotto", "Pasta", "Gnocchi", "Polenta", "Arepa", "Empanada", "Samosa"
	};
}

const char* CPlayerlistUtils::GetStreamerName(uint32_t uAccountID)
{
	if (!uAccountID)
		return PLAYER;

	if (auto it = m_mFakerNames.find(uAccountID); it != m_mFakerNames.end())
		return it->second.c_str();

	const uint32_t uHash = uAccountID * 2654435761u;
	const uint32_t uIndex = uHash % kFakerNames.size();
	const uint32_t uSuffix = (uHash >> 13) % 90 + 10;

	auto& sName = m_mFakerNames[uAccountID];
	sName = std::format("{} {}", kFakerNames[uIndex], uSuffix);
	return sName.c_str();
}

uint32_t CPlayerlistUtils::GetAccountID(int iIndex)
{
	auto pResource = H::Entities.GetResource();
	if (pResource && pResource->m_bValid(iIndex) && !pResource->IsFakePlayer(iIndex))
		return pResource->m_iAccountID(iIndex);
	return 0;
}

int CPlayerlistUtils::GetIndex(uint32_t uAccountID)
{
	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return 0;
	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (pResource->m_bValid(n) && !pResource->IsFakePlayer(n) && pResource->m_iAccountID(n) == uAccountID)
			return n;
	}
	return 0;
}

PriorityLabel_t* CPlayerlistUtils::GetTag(int iID)
{
	if (iID > -1 && iID < m_vTags.size())
		return &m_vTags[iID];

	return nullptr;
}

int CPlayerlistUtils::GetTag(const std::string& sTag)
{
	auto uHash = FNV1A::Hash32(sTag.c_str());

	int iID = -1;
	for (auto& tTag : m_vTags)
	{
		iID++;
		if (uHash == FNV1A::Hash32(tTag.m_sName.c_str()))
			return iID;
	}

	return -1;
}

uint32_t CPlayerlistUtils::SteamIDToAccountID(const std::string& sSteamID)
{
	std::string sInput = sSteamID;
	while (!sInput.empty() && std::isspace((unsigned char)sInput.front()))
		sInput.erase(sInput.begin());
	while (!sInput.empty() && std::isspace((unsigned char)sInput.back()))
		sInput.pop_back();
	if (sInput.empty())
		return 0;

	try
	{
		// "[U:1:12345678]" / "[U:1:12345678:entry]"
		if (const auto iStart = sInput.find('['); iStart != std::string::npos)
		{
			const auto iEnd = sInput.find(']', iStart);
			if (iEnd == std::string::npos)
				return 0;

			std::vector<std::string> vParts;
			size_t iPos = iStart + 1;
			while (iPos <= iEnd)
			{
				const auto iNext = sInput.find(':', iPos);
				if (iNext == std::string::npos || iNext > iEnd)
				{
					vParts.push_back(sInput.substr(iPos, iEnd - iPos));
					break;
				}
				vParts.push_back(sInput.substr(iPos, iNext - iPos));
				iPos = iNext + 1;
			}
			if (vParts.size() < 3)
				return 0;
			const uint64_t uAccount = std::stoull(vParts[2]);
			return uAccount && uAccount <= 0xFFFFFFFFull ? uint32_t(uAccount) : 0;
		}

		// "STEAM_0:1:12345" / "STEAM_1:1:12345" / "0:1:12345" (legacy two-part)
		{
			std::vector<std::string> vParts;
			size_t iPos = 0;
			while (iPos <= sInput.size())
			{
				const auto iNext = sInput.find(':', iPos);
				if (iNext == std::string::npos)
				{
					vParts.push_back(sInput.substr(iPos));
					break;
				}
				vParts.push_back(sInput.substr(iPos, iNext - iPos));
				iPos = iNext + 1;
			}
			if (vParts.size() >= 3)
			{
				const uint64_t uY = std::stoull(vParts[vParts.size() - 2]);
				const uint64_t uZ = std::stoull(vParts[vParts.size() - 1]);
				const uint64_t uAccount = uZ * 2 + uY;
				return uAccount && uAccount <= 0xFFFFFFFFull ? uint32_t(uAccount) : 0;
			}
		}

		// raw number: steamid64 or account id
		uint64_t uValue = std::stoull(sInput);
		if (uValue > 0xFFFFFFFFull)
		{
			if (uValue < 0x0110000100000000ull) // steamid64 universe marker
				return 0;
			uValue -= 0x0110000100000000ull;
		}
		return uValue && uValue <= 0xFFFFFFFFull ? uint32_t(uValue) : 0;
	}
	catch (...)
	{
		return 0;
	}
}



void CPlayerlistUtils::AddTag(uint32_t uAccountID, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	if (!uAccountID)
		return;

	if (!HasTag(uAccountID, iID))
	{
		mPlayerTags[uAccountID].push_back(iID);
		m_bSave = bSave;
		if (auto pTag = GetTag(iID); pTag && sName)
			F::Output.TagsChanged(sName, "Added", pTag->m_tColor.ToHexA().c_str(), pTag->m_sName.c_str());
	}
}
void CPlayerlistUtils::AddTag(int iIndex, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	AddTag(GetAccountID(iIndex), iID, bSave, sName, mPlayerTags);
}
void CPlayerlistUtils::AddTag(uint32_t uAccountID, int iID, bool bSave, const char* sName)
{
	AddTag(uAccountID, iID, bSave, sName, m_mPlayerTags);
}
void CPlayerlistUtils::AddTag(int iIndex, int iID, bool bSave, const char* sName)
{
	AddTag(iIndex, iID, bSave, sName, m_mPlayerTags);
}

void CPlayerlistUtils::RemoveTag(uint32_t uAccountID, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	if (!uAccountID)
		return;

	auto& vTags = mPlayerTags[uAccountID];
	for (auto it = vTags.begin(); it != vTags.end(); it++)
	{
		if (iID == *it)
		{
			vTags.erase(it);
			m_bSave = bSave;
			if (auto pTag = GetTag(iID); pTag && sName)
				F::Output.TagsChanged(sName, "Removed", pTag->m_tColor.ToHexA().c_str(), pTag->m_sName.c_str());
			break;
		}
	}
	if (vTags.empty())
		mPlayerTags.erase(uAccountID);
}
void CPlayerlistUtils::RemoveTag(int iIndex, int iID, bool bSave, const char* sName, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	RemoveTag(GetAccountID(iIndex), iID, bSave, sName, mPlayerTags);
}
void CPlayerlistUtils::RemoveTag(uint32_t uAccountID, int iID, bool bSave, const char* sName)
{
	RemoveTag(uAccountID, iID, bSave, sName, m_mPlayerTags);
}
void CPlayerlistUtils::RemoveTag(int iIndex, int iID, bool bSave, const char* sName)
{
	RemoveTag(iIndex, iID, bSave, sName, m_mPlayerTags);
}

bool CPlayerlistUtils::HasTags(uint32_t uAccountID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	if (!uAccountID)
		return false;

	return !mPlayerTags[uAccountID].empty();
}
bool CPlayerlistUtils::HasTags(int iIndex, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	return HasTags(GetAccountID(iIndex), mPlayerTags);
}
bool CPlayerlistUtils::HasTags(uint32_t uAccountID)
{
	return HasTags(uAccountID, m_mPlayerTags);
}
bool CPlayerlistUtils::HasTags(int iIndex)
{
	return HasTags(iIndex, m_mPlayerTags);
}

bool CPlayerlistUtils::HasTag(uint32_t uAccountID, int iID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	if (!uAccountID)
		return false;

	auto it = std::ranges::find_if(mPlayerTags[uAccountID], [iID](const auto& _iID) { return iID == _iID; });
	return it != mPlayerTags[uAccountID].end();
}
bool CPlayerlistUtils::HasTag(int iIndex, int iID, std::unordered_map<uint32_t, std::vector<int>>& mPlayerTags)
{
	return HasTag(GetAccountID(iIndex), iID, mPlayerTags);
}
bool CPlayerlistUtils::HasTag(uint32_t uAccountID, int iID)
{
	return HasTag(uAccountID, iID, m_mPlayerTags) || HasTag(uAccountID, iID, m_mDatabaseTags);
}
bool CPlayerlistUtils::HasTag(int iIndex, int iID)
{
	return HasTag(GetAccountID(iIndex), iID);
}



int CPlayerlistUtils::GetPriority(uint32_t uAccountID, bool bCache)
{
	if (bCache)
		return H::Entities.GetPriority(uAccountID);

	const int iDefault = m_vTags[TagToIndex(DEFAULT_TAG)].m_iPriority;
	if (!uAccountID)
		return iDefault;

	if (HasTag(uAccountID, TagToIndex(IGNORED_TAG)))
		return m_vTags[TagToIndex(IGNORED_TAG)].m_iPriority;

	std::vector<int> vPriorities;
	{
		for (auto& iID : GetPriorityTags(uAccountID))
		{
			auto pTag = GetTag(iID);
			if (pTag && !pTag->m_bLabel)
				vPriorities.push_back(pTag->m_iPriority);
		}
	}
	if (H::Entities.IsFriend(uAccountID))
	{
		auto& tTag = m_vTags[TagToIndex(FRIEND_TAG)];
		if (!tTag.m_bLabel)
			vPriorities.push_back(tTag.m_iPriority);
	}
	if (H::Entities.InParty(uAccountID))
	{
		auto& tTag = m_vTags[TagToIndex(PARTY_TAG)];
		if (!tTag.m_bLabel)
			vPriorities.push_back(tTag.m_iPriority);
	}
	if (H::Entities.IsF2P(uAccountID))
	{
		auto& tTag = m_vTags[TagToIndex(F2P_TAG)];
		if (!tTag.m_bLabel)
			vPriorities.push_back(tTag.m_iPriority);
	}
	if (vPriorities.empty())
		return iDefault;

	std::sort(vPriorities.begin(), vPriorities.end(), std::greater<int>());
	return vPriorities.front();
}
int CPlayerlistUtils::GetPriority(int iIndex, bool bCache)
{
	if (bCache)
		return H::Entities.GetPriority(iIndex);

	return GetPriority(GetAccountID(iIndex));
}

PriorityLabel_t* CPlayerlistUtils::GetSignificantTag(uint32_t uAccountID, int iMode)
{
	if (!uAccountID)
		return nullptr;

	std::vector<PriorityLabel_t*> vTags;
	if (!iMode || iMode == 1)
	{
		if (HasTag(uAccountID, TagToIndex(IGNORED_TAG)))
			return &m_vTags[TagToIndex(IGNORED_TAG)];

		for (auto& iID : GetPriorityTags(uAccountID))
		{
			PriorityLabel_t* _pTag = GetTag(iID);
			if (_pTag && !_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.IsFriend(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(FRIEND_TAG)];
			if (!_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.InParty(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(PARTY_TAG)];
			if (!_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.IsF2P(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(F2P_TAG)];
			if (!_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
	}
	if ((!iMode || iMode == 2) && !vTags.size())
	{
		for (auto& iID : GetPriorityTags(uAccountID))
		{
			PriorityLabel_t* _pTag = GetTag(iID);
			if (_pTag && _pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.IsFriend(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(FRIEND_TAG)];
			if (_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.InParty(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(PARTY_TAG)];
			if (_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
		if (H::Entities.IsF2P(uAccountID))
		{
			auto _pTag = &m_vTags[TagToIndex(F2P_TAG)];
			if (_pTag->m_bLabel)
				vTags.push_back(_pTag);
		}
	}
	if (vTags.empty())
		return nullptr;

	std::sort(vTags.begin(), vTags.end(), [&](const PriorityLabel_t* a, const PriorityLabel_t* b) -> bool
	{
		// sort by priority if unequal
		if (a->m_iPriority != b->m_iPriority)
			return a->m_iPriority > b->m_iPriority;

		return a->m_sName < b->m_sName;
	});
	return vTags.front();
}
PriorityLabel_t* CPlayerlistUtils::GetSignificantTag(int iIndex, int iMode)
{
	return GetSignificantTag(GetAccountID(iIndex), iMode);
}

const std::vector<int>& CPlayerlistUtils::GetPriorityTags(uint32_t uAccountID)
{
	if (m_mDatabaseTags.contains(uAccountID))
		return m_mDatabaseTags.at(uAccountID);
	if (m_mPlayerTags.contains(uAccountID))
		return m_mPlayerTags.at(uAccountID);
	return m_vDummy;
}

bool CPlayerlistUtils::InDatabase(uint32_t uAccountID)
{
	auto it = m_mDatabaseTags.find(uAccountID);
	return it != m_mDatabaseTags.end() && !it->second.empty();
}

std::vector<int> CPlayerlistUtils::GetEffectiveTags(uint32_t uAccountID)
{
	std::vector<int> vTags;
	if (auto it = m_mDatabaseTags.find(uAccountID); it != m_mDatabaseTags.end())
	{
		for (auto& iID : it->second)
			vTags.push_back(iID);
	}
	if (auto it = m_mPlayerTags.find(uAccountID); it != m_mPlayerTags.end())
	{
		for (auto& iID : it->second)
		{
			if (std::find(vTags.begin(), vTags.end(), iID) == vTags.end())
				vTags.push_back(iID);
		}
	}
	return vTags;
}

bool CPlayerlistUtils::HasReportableMark(uint32_t uAccountID)
{
	for (auto& iID : GetEffectiveTags(uAccountID))
	{
		if (iID == TagToIndex(CHEATER_TAG) || IsProtectedMark(IndexToTag(iID)))
			return true;
	}
	return false;
}

bool CPlayerlistUtils::IsIgnored(uint32_t uAccountID)
{
	const int iPriority = GetPriority(uAccountID);
	const int iIgnored = m_vTags[TagToIndex(IGNORED_TAG)].m_iPriority;
	return iPriority <= iIgnored;
}
bool CPlayerlistUtils::IsIgnored(int iIndex)
{
	return IsIgnored(GetAccountID(iIndex));
}

bool CPlayerlistUtils::IsPrioritized(uint32_t uAccountID)
{
	if (!uAccountID)
		return false;

	const int iPriority = GetPriority(uAccountID);
	const int iDefault = m_vTags[TagToIndex(DEFAULT_TAG)].m_iPriority;
	return iPriority > iDefault;
}
bool CPlayerlistUtils::IsPrioritized(int iIndex)
{
	return IsPrioritized(GetAccountID(iIndex));
}



int CPlayerlistUtils::GetNameType(int iIndex)
{
	if (Vars::Visuals::UI::StreamerMode.Value)
	{
		if (iIndex == I::EngineClient->GetLocalPlayer())
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Local)
				return NameTypeEnum::Local;
		}
		else if (H::Entities.IsFriend(iIndex))
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Friends)
				return NameTypeEnum::Friend;
		}
		else if (H::Entities.InParty(iIndex))
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Party)
				return NameTypeEnum::Party;
		}
		else if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::All)
			return NameTypeEnum::Player;
	}
	if (const uint32_t uAccountID = GetAccountID(iIndex); uAccountID && GetPlayerAlias(uAccountID))
		return NameTypeEnum::Custom;
	return NameTypeEnum::None;
}

int CPlayerlistUtils::GetNameType(uint32_t uAccountID)
{
	if (Vars::Visuals::UI::StreamerMode.Value)
	{
		if (uAccountID == I::SteamUser->GetSteamID().GetAccountID())
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Local)
				return NameTypeEnum::Local;
		}
		else if (H::Entities.IsFriend(uAccountID))
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Friends)
				return NameTypeEnum::Friend;
		}
		else if (H::Entities.InParty(uAccountID))
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Party)
				return NameTypeEnum::Party;
		}
		else if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::All)
			return NameTypeEnum::Player;
	}
	if (GetPlayerAlias(uAccountID))
		return NameTypeEnum::Custom;
	return NameTypeEnum::None;
}

const char* CPlayerlistUtils::GetPlayerName(int iIndex, const char* sDefault, int* pType)
{
	int iType = GetNameType(iIndex);
	if (pType) *pType = iType;

	switch (iType)
	{
	case NameTypeEnum::Local:
		if (!Vars::Visuals::UI::StreamerName.Value.empty())
			return Vars::Visuals::UI::StreamerName.Value.c_str();
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(GetAccountID(iIndex));
		return LOCAL;
	case NameTypeEnum::Friend:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(GetAccountID(iIndex));
		return FRIEND;
	case NameTypeEnum::Party:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(GetAccountID(iIndex));
		return PARTY;
	case NameTypeEnum::Player:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(GetAccountID(iIndex));
		if (auto pTag = GetSignificantTag(iIndex, 0))
			return pTag->m_sName.c_str();
		else if (auto pResource = H::Entities.GetResource(); pResource && pResource->m_bValid(iIndex))
			return pResource->m_iTeam(I::EngineClient->GetLocalPlayer()) != pResource->m_iTeam(iIndex) ? ENEMY : TEAMMATE;
		return PLAYER;
	case NameTypeEnum::Custom:
		if (auto sAlias = GetPlayerAlias(GetAccountID(iIndex)))
			return sAlias->c_str();
	}
	return sDefault;
}

const char* CPlayerlistUtils::GetPlayerName(uint32_t uAccountID, const char* sDefault, int* pType)
{
	int iType = GetNameType(uAccountID);
	if (pType) *pType = iType;

	switch (iType)
	{
	case NameTypeEnum::Local:
		if (!Vars::Visuals::UI::StreamerName.Value.empty())
			return Vars::Visuals::UI::StreamerName.Value.c_str();
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(uAccountID);
		return LOCAL;
	case NameTypeEnum::Friend:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(uAccountID);
		return FRIEND;
	case NameTypeEnum::Party:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(uAccountID);
		return PARTY;
	case NameTypeEnum::Player:
		if (Vars::Visuals::UI::RandomNames.Value)
			return GetStreamerName(uAccountID);
		if (auto pTag = GetSignificantTag(uAccountID, 0))
			return pTag->m_sName.c_str();
		else if (auto pResource = H::Entities.GetResource(); (iType = GetIndex(uAccountID)) && pResource && pResource->m_bValid(iType))
			return pResource->m_iTeam(I::EngineClient->GetLocalPlayer()) != pResource->m_iTeam(iType) ? ENEMY : TEAMMATE;
		return PLAYER;
	case NameTypeEnum::Custom:
		if (auto sAlias = GetPlayerAlias(uAccountID))
			return sAlias->c_str();
	}
	return sDefault;
}

const char* CPlayerlistUtils::GetPlayerName(int iIndex)
{
	auto pResource = H::Entities.GetResource();
	return pResource && pResource->IsValid(iIndex) ? pResource->GetName(iIndex) : PLAYER_ERROR_NAME;
}

const char* CPlayerlistUtils::GetPlayerName(uint32_t uAccountID)
{
	auto pResource = H::Entities.GetResource();
	int iIndex = GetIndex(uAccountID);
	return pResource && pResource->IsValid(iIndex) ? pResource->GetName(iIndex) : PLAYER_ERROR_NAME;
}



void CPlayerlistUtils::RequestNames(const std::vector<uint32_t>& vAccountIDs)
{
	if (!I::SteamFriends)
		return;

	int iRequested = 0;
	for (const auto& uAccountID : vAccountIDs)
	{
		if (!uAccountID || m_vNamesPending.contains(uAccountID))
			continue;

		I::SteamFriends->RequestUserInformation(CSteamID(0x0110000100000000ull + uAccountID), true);
		m_vNamesPending[uAccountID] = 1500; // poll budget (~15-30s before giving up)
		iRequested++;
	}

	if (iRequested)
		SDK::Output("Lookup", std::format("Requesting current usernames from Steam for {} player(s)", iRequested).c_str(),
			INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_PERSON_SEARCH);
}

void CPlayerlistUtils::PollNames()
{
	if (m_vNamesPending.empty())
		return;

	// PollNames is invoked from CL_Move every command and from the menu draw;
	// throttle so we don't hammer Steam IPC with GetFriendPersonaName each frame
	static Timer tTimer = {};
	if (!tTimer.Run(0.1f))
		return;

	int iResolved = 0;
	for (auto it = m_vNamesPending.begin(); it != m_vNamesPending.end();)
	{
		const auto uAccountID = it->first;
		const char* pName = I::SteamFriends ? I::SteamFriends->GetFriendPersonaName(CSteamID(0x0110000100000000ull + uAccountID)) : nullptr;
		if (pName && pName[0])
		{
			auto& sName = m_mPlayerNames[uAccountID];
			if (sName != pName)
			{
				sName = pName;
				m_bSave = true;
			}
			it = m_vNamesPending.erase(it);
			iResolved++;
			continue;
		}

		if (--it->second <= 0)
		{
			it = m_vNamesPending.erase(it);
			continue;
		}
		++it;
	}

	if (iResolved)
		SDK::Output("Lookup", std::format("Fetched {} current username(s) from Steam", iResolved).c_str(),
			INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_PERSON_SEARCH);
}

void CPlayerlistUtils::Store()
{
	static Timer tTimer = {};
	if (!tTimer.Run(1.f))
		return;

	std::lock_guard tLock(F::Menu.m_tMutex);
	m_vPlayerCache.clear();

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (!pResource->m_bValid(n) || !pResource->m_bConnected(n))
			continue;

		uint32_t uAccountID = pResource->m_iAccountID(n);
		m_vPlayerCache.emplace_back(
			pResource->GetName(n),
			uAccountID,
			pResource->m_iUserID(n),
			pResource->m_iTeam(n),
			pResource->m_bAlive(n),
			n == I::EngineClient->GetLocalPlayer(),
			pResource->IsFakePlayer(n),
			H::Entities.IsFriend(uAccountID),
			H::Entities.InParty(uAccountID),
			H::Entities.IsF2P(uAccountID),
			H::Entities.GetLevel(uAccountID),
			H::Entities.GetParty(uAccountID)
		);

		if (uAccountID && !pResource->IsFakePlayer(n))
		{
			const std::string sName = pResource->GetName(n);
			auto it = m_mPlayerNames.find(uAccountID);
			if (it == m_mPlayerNames.end())
			{
				// don't flag a save just for seeding a first-seen name each time
				// a transient player exists in the server; persist lazily on real changes
				m_mPlayerNames[uAccountID] = sName;
			}
			else if (it->second != sName)
			{
				it->second = sName;
				m_bSave = true;
			}
		}
	}
}