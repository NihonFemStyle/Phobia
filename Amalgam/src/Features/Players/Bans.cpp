#include "Bans.h"

#include "../../SDK/SDK.h"
#include "../Configs/Configs.h"
#include "../ImGui/Menu/Menu.h"
#include "../../Utils/Timer/Timer.h"
#include "PlayerUtils.h"

#include <windows.h>
#include <winhttp.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <cctype>
#include <ctime>
#include <format>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <unordered_set>

// Steam Web API VAC / game bans + SteamHistory.net sourcebans integration.
// Bans are fetched on a background worker thread, cached to disk and drawn as icons in the player list.

namespace
{
	uint64_t GetSteamID64(uint32_t uAccountID)
	{
		return CSteamID(uAccountID, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64();
	}

	std::string FormatRelative(uint64_t uTimestamp)
	{
		if (!uTimestamp)
			return "unknown";

		const uint64_t uNow = (uint64_t)std::time(nullptr);
		if (uTimestamp > uNow)
			return "just now";

		const uint64_t uDelta = uNow - uTimestamp;
		const uint64_t uMinute = 60, uHour = 3600, uDay = 86400, uWeek = uDay * 7, uYear = uDay * 365;
		if (uDelta >= uYear)
			return std::format("{}y", uDelta / uYear);
		if (uDelta >= uWeek)
			return std::format("{}w", uDelta / uWeek);
		if (uDelta >= uDay)
			return std::format("{}d", uDelta / uDay);
		if (uDelta >= uHour)
			return std::format("{}h", uDelta / uHour);
		if (uDelta >= uMinute)
			return std::format("{}m", uDelta / uMinute);
		return "just now";
	}

	template <class T>
	T GetOpt(const boost::property_tree::ptree& t, const char* sKey, T defaultValue)
	{
		if (auto o = t.get_optional<T>(sKey))
			return *o;
		return defaultValue;
	}

	bool ContainsToken(const std::string& sHaystack, const char* sToken)
	{
		std::string sLower = sHaystack;
		std::transform(sLower.begin(), sLower.end(), sLower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

		std::string sTok = sToken;
		std::transform(sTok.begin(), sTok.end(), sTok.begin(), [](unsigned char c) { return (char)std::tolower(c); });

		size_t iPos = 0;
		while ((iPos = sLower.find(sTok, iPos)) != std::string::npos)
		{
			const bool bLeft = iPos == 0 || !std::isalnum((unsigned char)sLower[iPos - 1]);
			const size_t iEnd = iPos + sTok.size();
			const bool bRight = iEnd >= sLower.size() || !std::isalnum((unsigned char)sLower[iEnd]);
			if (bLeft && bRight)
				return true;
			iPos++;
		}
		return false;
	}

	bool IsAnticheatSourceBan(const SourceBan_t& tSource)
	{
		const std::string sFields[] = { tSource.m_sReason, tSource.m_sName, tSource.m_sUnbanReason };
		for (const auto& sField : sFields)
		{
			if (ContainsToken(sField, "smac") || ContainsToken(sField, "lac") ||
				ContainsToken(sField, "stac") || ContainsToken(sField, "mattie"))
				return true;
		}
		return false;
	}

	// token set used for sourceban reasons only (chat scanning uses regex below)
	const std::vector<std::string>& RacistSourceBanTokens()
	{
		static const std::vector<std::string> sTokens = {
			"nazi", "racist", "racism", "racists", "slur", "slurs", "hate speech",
			"white power", "white supremac", "kkk", "klan", "hitler", "heil hitler",
			"nigg", "faggot", "fag", "kike", "spic", "chink", "gook", "wetback", "coon",
			"honkey", "honky", "beaner", "towelhead", "sandnigger", "jigaboo",
			"raghead", "mongoloid", "1488"
		};
		return sTokens;
	}

	bool IsRacistSourceBan(const SourceBan_t& tSource)
	{
		const std::string sFields[] = { tSource.m_sReason, tSource.m_sName, tSource.m_sUnbanReason };
		for (const auto& sField : sFields)
		{
			for (const auto& sToken : RacistSourceBanTokens())
			{
				if (ContainsToken(sField, sToken.c_str()))
					return true;
			}
		}
		return false;
	}

	bool IsRacistText(const std::string& sText)
	{
		// word-boundary, case-insensitive regex over known terms and phrases.
		// patterns cover spelling variants of slurs (e.g. "nigger"/"nigga") and phrases
		// while avoiding false positives like "spicy" ("spic" requires a word boundary).
		static const std::regex sRegex(
			R"(\b(nazi|racis[mt]s?|slurs?|hate speech|white power|white suprem[a-z]*|kkk|klan|hitler|heil hitler|nigg(?:a|er)[a-z]*|fagg[a-z]*|kike|spic|chink|gook|wetback|coon|honkey|honky|beaner|towelhead|sandnigg[a-z]*|jigaboo|raghead|mongoloid|1488)\b)",
			std::regex::icase);

		if (sText.empty())
			return false;

		return std::regex_search(sText, sRegex);
	}

	bool IsPedoText(const std::string& sText)
	{
		// word-boundary, case-insensitive regex over known terms and phrases.
		// covers jokes/references to pedophilia across common spellings ("pedo", "p3do",
		// "pedofil*", "jailbait", etc.) while avoiding false positives like "pedometer".
		static const std::regex sRegex(
			R"(\b(pedo(?:phil[a-z]*|fil[a-z]*|s|bear)?|p3do(?:phil[a-z]*|fil[a-z]*|s)?|jailbait|prepubesc[a-z]*|diddler|groomer|molest[a-z]*)\b)",
			std::regex::icase);

		if (sText.empty())
			return false;

		return std::regex_search(sText, sRegex);
	}

	constexpr uint64_t uOneYearSeconds = 365ull * 24ull * 60ull * 60ull;

	bool IsWithinOneYear(uint64_t uTimestamp)
	{
		if (!uTimestamp)
			return false;
		const uint64_t uNow = (uint64_t)std::time(nullptr);
		return uTimestamp > uNow || uNow - uTimestamp < uOneYearSeconds;
	}

	bool HasCheatEvidence(const SteamBanData_t& tBan)
	{
		if (tBan.m_iVACBans > 0)
			return true;
		for (auto& tSource : tBan.m_vSourceBans)
		{
			if (IsAnticheatSourceBan(tSource))
				return true;
		}
		return false;
	}

	bool HasRecentCheatEvidence(const SteamBanData_t& tBan)
	{
		if (tBan.m_iVACBans > 0 && tBan.m_iDaysSinceLastVAC < 365)
			return true;
		for (auto& tSource : tBan.m_vSourceBans)
		{
			if (IsAnticheatSourceBan(tSource) && IsWithinOneYear(tSource.m_uBanTimestamp))
				return true;
		}
		return false;
	}

	// "[U:1:12345678]" -> account id (12345678)
	uint32_t ParseSteamIDAccount(const std::string& sSteamID)
	{
		const auto iStart = sSteamID.find("[U:");
		if (iStart == std::string::npos)
			return 0;

		const auto iEnd = sSteamID.find(']', iStart);
		if (iEnd == std::string::npos)
			return 0;

		const auto iColon = sSteamID.rfind(':', iEnd);
		if (iColon == std::string::npos || iColon + 1 >= iEnd)
			return 0;

		try
		{
			return (uint32_t)std::stoul(sSteamID.substr(iColon + 1, iEnd - iColon - 1));
		}
		catch (...)
		{
			return 0;
		}
	}

	// Parses a TF2BD playerlist body into tOut (tOut keeps pre-filled defaults on parse failure/partial data)
	bool ParseTF2BDJSON(const std::string& sData, TF2BDList_t& tOut)
	{
		try
		{
			boost::property_tree::ptree tRoot;
			std::stringstream ss(sData);
			boost::property_tree::read_json(ss, tRoot);

			if (auto o = tRoot.get_child_optional("file_info"))
			{
				tOut.m_sTitle = GetOpt<std::string>(*o, "title", tOut.m_sTitle);
				tOut.m_sDescription = GetOpt<std::string>(*o, "description", tOut.m_sDescription);
				tOut.m_sUpdateURL = GetOpt<std::string>(*o, "update_url", tOut.m_sUpdateURL);
			}

			bool bRead = false;
			if (auto o = tRoot.get_child_optional("players"))
			{
				for (auto& [sKey, tPlayer] : *o)
				{
					const uint32_t uAccountID = ParseSteamIDAccount(GetOpt<std::string>(tPlayer, "steamid", ""));
					if (!uAccountID)
						continue;

					std::vector<std::string> vAttributes;
					if (auto oAttrs = tPlayer.get_child_optional("attributes"))
					{
						for (auto& [sKey2, tAttr] : *oAttrs)
							vAttributes.push_back(tAttr.get_value<std::string>());
					}
					tOut.m_mPlayers[uAccountID] = std::move(vAttributes);
					bRead = true;
				}
			}

			return bRead || tRoot.get_child_optional("file_info");
		}
		catch (...)
		{
			return false;
		}
	}

	// winhttp.dll loaded at runtime to avoid linking winhttp.lib
	struct WinHttpApi
	{
		HMODULE hMod = nullptr;

		decltype(&WinHttpOpen) Open = nullptr;
		decltype(&WinHttpConnect) Connect = nullptr;
		decltype(&WinHttpOpenRequest) OpenRequest = nullptr;
		decltype(&WinHttpSendRequest) SendRequest = nullptr;
		decltype(&WinHttpReceiveResponse) ReceiveResponse = nullptr;
		decltype(&WinHttpQueryDataAvailable) QueryDataAvailable = nullptr;
		decltype(&WinHttpQueryHeaders) QueryHeaders = nullptr;
		decltype(&WinHttpReadData) ReadData = nullptr;
		decltype(&WinHttpCloseHandle) CloseHandle = nullptr;
		decltype(&WinHttpSetTimeouts) SetTimeouts = nullptr;

		bool Load()
		{
			if (hMod)
				return Open != nullptr;

			hMod = LoadLibraryW(L"winhttp.dll");
			if (!hMod)
				return false;

			Open = reinterpret_cast<decltype(&WinHttpOpen)>(GetProcAddress(hMod, "WinHttpOpen"));
			Connect = reinterpret_cast<decltype(&WinHttpConnect)>(GetProcAddress(hMod, "WinHttpConnect"));
			OpenRequest = reinterpret_cast<decltype(&WinHttpOpenRequest)>(GetProcAddress(hMod, "WinHttpOpenRequest"));
			SendRequest = reinterpret_cast<decltype(&WinHttpSendRequest)>(GetProcAddress(hMod, "WinHttpSendRequest"));
			ReceiveResponse = reinterpret_cast<decltype(&WinHttpReceiveResponse)>(GetProcAddress(hMod, "WinHttpReceiveResponse"));
			QueryDataAvailable = reinterpret_cast<decltype(&WinHttpQueryDataAvailable)>(GetProcAddress(hMod, "WinHttpQueryDataAvailable"));
			QueryHeaders = reinterpret_cast<decltype(&WinHttpQueryHeaders)>(GetProcAddress(hMod, "WinHttpQueryHeaders"));
			ReadData = reinterpret_cast<decltype(&WinHttpReadData)>(GetProcAddress(hMod, "WinHttpReadData"));
			CloseHandle = reinterpret_cast<decltype(&WinHttpCloseHandle)>(GetProcAddress(hMod, "WinHttpCloseHandle"));
			SetTimeouts = reinterpret_cast<decltype(&WinHttpSetTimeouts)>(GetProcAddress(hMod, "WinHttpSetTimeouts"));

			return Open && Connect && OpenRequest && SendRequest && ReceiveResponse && QueryDataAvailable && QueryHeaders && ReadData && CloseHandle && SetTimeouts;
		}
	};
}

void CSteamBans::Run()
{
	if (!Vars::Menu::Bans::Enabled.Value || G::Unload)
		return;

	if (!I::EngineClient->IsInGame())
		return;

	// alert for marked players from the playerlist (Database.json / Players.json), independent of ban API keys
	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		std::lock_guard lock(m_Mutex);
		for (auto& tPlayer : F::PlayerUtils.m_vPlayerCache)
		{
			if (tPlayer.m_bFake || tPlayer.m_bParty || !tPlayer.m_uAccountID)
				continue;

			const uint32_t uAccountID = tPlayer.m_uAccountID;
			if (m_mMarkedAlerted.contains(uAccountID))
				continue;
			if (!F::PlayerUtils.HasReportableMark(uAccountID))
				continue;
			if (F::PlayerUtils.IsIgnored(uAccountID))
				continue;

			m_mMarkedAlerted.insert(uAccountID);
			m_vMarkedAlerts.push_back(uAccountID);
		}
	}

	if (Vars::Menu::Bans::SteamKey.Value.empty() && Vars::Menu::Bans::SteamHistoryKey.Value.empty())
		return;
	EnsureWorker();

	const uint64_t uNow = (uint64_t)std::time(nullptr);
	const uint64_t uInterval = (uint64_t)Vars::Menu::Bans::RefreshInterval.Value * 60;

	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		std::lock_guard lock(m_Mutex);
		for (auto& tPlayer : F::PlayerUtils.m_vPlayerCache)
		{
			if (tPlayer.m_bFake || tPlayer.m_bParty || !tPlayer.m_uAccountID)
				continue;

			const uint32_t uAccountID = tPlayer.m_uAccountID;
			if (m_vPending.contains(uAccountID))
				continue;
			if (auto it = m_mBans.find(uAccountID); it != m_mBans.end() && uNow - it->second.m_uLastUpdate < uInterval)
				continue;

			m_vPending.insert(uAccountID);
		}
	}

	m_Cond.notify_one();
}

void CSteamBans::Check(uint32_t uAccountID)
{
	if (!Vars::Menu::Bans::Enabled.Value || G::Unload)
		return;

	if (!I::EngineClient->IsInGame())
		return;

	if (Vars::Menu::Bans::SteamKey.Value.empty() && Vars::Menu::Bans::SteamHistoryKey.Value.empty())
		return;

	if (!uAccountID)
		return;

	EnsureWorker();

	const uint64_t uNow = (uint64_t)std::time(nullptr);
	const uint64_t uInterval = (uint64_t)Vars::Menu::Bans::RefreshInterval.Value * 60;

	{
		std::lock_guard lock(m_Mutex);
		if (m_vPending.contains(uAccountID))
			return;
		if (auto it = m_mBans.find(uAccountID); it != m_mBans.end() && uNow - it->second.m_uLastUpdate < uInterval)
			return;

		m_vPending.insert(uAccountID);
	}

	m_Cond.notify_one();
}

void CSteamBans::OnMatchStart()
{
	if (!Vars::Menu::Bans::Enabled.Value || G::Unload)
		return;

	m_bMatchCheckPending = true;
}

void CSteamBans::OnLocalSpawn()
{
	if (!m_bMatchCheckPending.exchange(false))
		return;

	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		if (F::PlayerUtils.m_vPlayerCache.empty())
		{
			m_bMatchCheckPending = true; // cache not ready yet, retry next spawn
			return;
		}
	}

	Run();
}

void CSteamBans::OnFrame()
{
	if (G::Unload)
		return;

	if (m_bListsLoaded && !m_bMarksScanned && !F::PlayerUtils.m_bLoad)
	{
		ScanTF2BDLists();
		m_bMarksScanned = true;
	}

	if (!Vars::Menu::Bans::Enabled.Value)
		return;

	std::vector<std::string> vMessages;
	{
		std::lock_guard lock(m_Mutex);
		vMessages.swap(m_vListMessages);
	}
	if (!vMessages.empty())
	{
		for (auto& sMsg : vMessages)
			SDK::Output("TF2BD", sMsg.c_str(), INFO_COLOR, OUTPUT_MENU | OUTPUT_DEBUG | OUTPUT_CONSOLE);
	}

	std::vector<std::pair<uint32_t, int>> vMarks;
	{
		std::lock_guard lock(m_Mutex);
		for (auto& [uAccountID, tBan] : m_mBans)
		{
			if (tBan.m_iVACBans > 0)
				vMarks.emplace_back(uAccountID, F::PlayerUtils.TagToIndex(VAC_BAN_TAG));
			if (tBan.m_iGameBans > 0)
				vMarks.emplace_back(uAccountID, F::PlayerUtils.TagToIndex(GAME_BAN_TAG));
			if (!tBan.m_vSourceBans.empty())
				vMarks.emplace_back(uAccountID, F::PlayerUtils.TagToIndex(SOURCE_BAN_TAG));

			bool bCheatingReason = false, bRacistReason = false;
			for (auto& tSource : tBan.m_vSourceBans)
			{
				if (IsAnticheatSourceBan(tSource))
					bCheatingReason = true;
				if (IsRacistSourceBan(tSource))
					bRacistReason = true;
			}
			if (bCheatingReason || (tBan.m_iVACBans > 0 && tBan.m_iDaysSinceLastVAC < 365))
				vMarks.emplace_back(uAccountID, F::PlayerUtils.TagToIndex(SUSPECTED_CHEATER_TAG));
			if (bRacistReason)
				vMarks.emplace_back(uAccountID, F::PlayerUtils.TagToIndex(RACIST_TAG));
		}
	}
	if (!vMarks.empty())
	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		const int iCheaterTag = F::PlayerUtils.TagToIndex(CHEATER_TAG);
		const int iSuspectedTag = F::PlayerUtils.TagToIndex(SUSPECTED_CHEATER_TAG);
		for (auto& [uAccountID, iTag] : vMarks)
		{
			if (iTag == iSuspectedTag && F::PlayerUtils.HasTag(uAccountID, iCheaterTag))
				continue; // no need to suspect a confirmed cheater
			if (!F::PlayerUtils.HasTag(uAccountID, iTag))
				F::PlayerUtils.AddTag(uAccountID, iTag, true, nullptr);
		}
	}

	if (!(Vars::Logging::Logs.Value & Vars::Logging::LogsEnum::Bans))
		return;

	std::vector<uint32_t> vAlerts;
	{
		std::lock_guard lock(m_Mutex);
		vAlerts.swap(m_vAlerts);
		std::vector<uint32_t> vMarked;
		vMarked.swap(m_vMarkedAlerts);
		vAlerts.insert(vAlerts.end(), vMarked.begin(), vMarked.end());
	}
	if (vAlerts.empty())
		return;

	// an account can be queued by both the ban worker and the marked-player sweep - alert once
	{
		std::unordered_set<uint32_t> vSeen;
		std::vector<uint32_t> vUnique;
		vUnique.reserve(vAlerts.size());
		for (const uint32_t uAccountID : vAlerts)
		{
			if (vSeen.insert(uAccountID).second)
				vUnique.push_back(uAccountID);
		}
		vAlerts.swap(vUnique);
	}

	static std::string s_sRed = Color_t(255, 100, 100).ToHex();
	const Color_t tColor = Color_t(255, 100, 100);
	for (const uint32_t uAccountID : vAlerts)
	{
		// keep alerts coming from the ban worker (have ban data) and from the playerlist sweep (marked players)
		if (!GetResult(uAccountID) && !m_mMarkedAlerted.contains(uAccountID))
			continue;

		std::string sName = "Unknown";
		{
			std::lock_guard tLock(F::Menu.m_tMutex);
			for (auto& tPlayer : F::PlayerUtils.m_vPlayerCache)
			{
				if (tPlayer.m_uAccountID == uAccountID)
				{
					sName = tPlayer.m_sName;
					break;
				}
			}
		}

		// the player's marks - Database.json when present, else local (Players.json)
		const bool bInDatabase = F::PlayerUtils.InDatabase(uAccountID);
		const char* sSource = bInDatabase ? "found in Database" : "found in Local Listings";
		std::string sTags;
		{
			std::lock_guard tLock(F::Menu.m_tMutex);
			for (const int iID : F::PlayerUtils.GetPriorityTags(uAccountID))
			{
				if (iID == F::PlayerUtils.TagToIndex(DEFAULT_TAG))
					continue;
				if (auto* pTag = F::PlayerUtils.GetTag(iID); pTag && !pTag->m_sName.empty())
				{
					if (!sTags.empty())
						sTags += ", ";
					sTags += pTag->m_sName;
				}
			}
		}
		if (sTags.empty())
			sTags = bInDatabase ? "masterlist marks" : "marked";

		int iTo = (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Toasts ? OUTPUT_TOAST : 0)
			| (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Party ? OUTPUT_PARTY : 0)
			| (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Console ? OUTPUT_CONSOLE : 0)
			| (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Menu ? OUTPUT_MENU : 0)
			| (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Debug ? OUTPUT_DEBUG : 0);
		if (iTo)
			SDK::Output("Ban Alert", std::format("{} - {} {} for {}", Vars::Menu::CheatTag.Value, sName, sSource, sTags).c_str(),
				tColor, iTo, ICON_MD_GAVEL);

		if (Vars::Logging::Bans::LogTo.Value & Vars::Logging::LogToEnum::Chat)
			SDK::Output(Vars::Menu::CheatTag.Value.c_str(),
				std::format("{}{}\x1 {} for {}{}", s_sRed, sName, sSource, s_sRed, sTags).c_str(),
				tColor, OUTPUT_CHAT);
	}
}

void CSteamBans::Unload()
{
	m_bUnload = true;
	m_Cond.notify_all();
	if (m_Worker.joinable())
		m_Worker.join();
	if (m_bCacheDirty)
		SaveCache();
}

std::optional<SteamBanData_t> CSteamBans::GetResult(uint32_t uAccountID)
{
	std::lock_guard lock(m_Mutex);
	if (auto it = m_mBans.find(uAccountID); it != m_mBans.end())
		return it->second;
	return std::nullopt;
}

std::vector<std::string> CSteamBans::GetTooltips(const SteamBanData_t& tBan) const
{
	std::vector<std::string> vTooltips(3);

	if (tBan.m_iVACBans > 0)
	{
		vTooltips[0] = std::format("{} VAC ban(s)", tBan.m_iVACBans);
		vTooltips[0] += std::format("\nLast ban {} day(s) ago", tBan.m_iDaysSinceLastVAC);
	}

	if (tBan.m_iGameBans > 0)
	{
		vTooltips[1] = std::format("{} game ban(s)", tBan.m_iGameBans);
		vTooltips[1] += std::format("\nLast ban {} day(s) ago", tBan.m_iDaysSinceLastGame);
	}

	if (!tBan.m_vSourceBans.empty())
	{
		vTooltips[2] = std::format("{} sourceban(s)", tBan.m_vSourceBans.size());
		int i = 0;
		for (auto& tSource : tBan.m_vSourceBans)
		{
			if (i++ == 5)
			{
				vTooltips[2] += std::format("\n... and {} more", tBan.m_vSourceBans.size() - 5);
				break;
			}

			vTooltips[2] += std::format("\n- {} [{}]: {}", tSource.m_sServer.empty() ? "?" : tSource.m_sServer, tSource.m_sState, FormatRelative(tSource.m_uBanTimestamp));
			std::string sReason = tSource.m_sReason.empty() ? "No reason" : tSource.m_sReason;
			if (sReason.size() > 80)
			{
				sReason.resize(80);
				sReason += "...";
			}
			vTooltips[2] += std::format("\n  {}", sReason);
		}
	}

	return vTooltips;
}

void CSteamBans::LoadLists()
{
	if (m_bListsLoaded)
		return;

	std::unordered_map<std::string, TF2BDList_t> mLoaded;
	try
	{
		const std::string sDir = F::Configs.m_sCorePath + "Lists\\";
		std::filesystem::create_directories(sDir);
		for (const auto& oEntry : std::filesystem::directory_iterator(sDir))
		{
			if (!oEntry.is_regular_file())
				continue;

			const std::string sFilename = oEntry.path().filename().string();
			if (sFilename.rfind("playerlist.", 0) != 0 || sFilename.size() < 5 ||
				sFilename.compare(sFilename.size() - 5, 5, ".json") != 0)
				continue;

			TF2BDList_t tList;
			try
			{
				std::ifstream file(oEntry.path());
				if (!file.is_open())
					continue;
				std::stringstream ss;
				ss << file.rdbuf();
				if (!ParseTF2BDJSON(ss.str(), tList))
					continue;
			}
			catch (...)
			{
				continue;
			}

			mLoaded[sFilename.substr(0, sFilename.size() - 5)] = std::move(tList);
		}
	}
	catch (...)
	{
	}

	size_t uListCount = 0;
	{
		std::lock_guard lock(m_Mutex);
		m_mTF2BDLists = std::move(mLoaded);
		for (auto& [sKey, tList] : m_mTF2BDLists)
		{
			uListCount++;
			if (!tList.m_sUpdateURL.empty())
				m_vTF2BDPending.push_back(sKey);
		}
	}
	m_bListsLoaded = true;

	SDK::Output("TF2BD", std::format("Loaded {} TF2BD player list(s)", uListCount).c_str(), INFO_COLOR, OUTPUT_MENU | OUTPUT_DEBUG | OUTPUT_CONSOLE);

	EnsureWorker();
	m_Cond.notify_one();
}

void CSteamBans::RefreshLists()
{
	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		std::lock_guard lock(m_Mutex);
		for (auto& tPlayer : F::PlayerUtils.m_vPlayerCache)
		{
			if (tPlayer.m_bFake || tPlayer.m_bParty || !tPlayer.m_uAccountID)
				continue;
			if (!m_vPending.contains(tPlayer.m_uAccountID))
				m_vPending.insert(tPlayer.m_uAccountID);
		}

		for (auto& [sKey, tList] : m_mTF2BDLists)
		{
			if (tList.m_sUpdateURL.empty())
				continue;
			if (std::find(m_vTF2BDPending.begin(), m_vTF2BDPending.end(), sKey) == m_vTF2BDPending.end())
				m_vTF2BDPending.push_back(sKey);
		}
	}

	SDK::Output("Phobia", "Refreshing player lists (TF2BD, Steam bans, SourceBans)", INFO_COLOR, OUTPUT_MENU | OUTPUT_DEBUG | OUTPUT_CONSOLE);

	EnsureWorker();
	m_Cond.notify_one();
}

void CSteamBans::ScanTF2BDLists()
{
	// lists whose "cheater" listings are proven accurate by the community: mark as confirmed cheaters
	static const std::vector<std::string> vConfirmedLists = {
		"playerlist.official", "playerlist.mcd", "playerlist.megacheatdb", "playerlist.rgl-gg",
		"playerlist.shadefall", "playerlist.sterling", "playerlist.tacobot",
		"playerlist.trusted", "playerlist.vorobey-hackerpolice", "playerlist.lunarisv"
	};

	auto IsConfirmedList = [&](const std::string& sKey) -> bool
	{
		if (sKey.rfind("playerlist.sleepy", 0) == 0)
			return sKey != "playerlist.sleepy-no-proof"; // all sleepy lists are confirmed except the no-proof one
		return std::find(vConfirmedLists.begin(), vConfirmedLists.end(), sKey) != vConfirmedLists.end();
	};

	std::vector<std::pair<std::string, TF2BDList_t>> vLists;
	{
		std::lock_guard lock(m_Mutex);
		for (auto& [sKey, tList] : m_mTF2BDLists)
			vLists.emplace_back(sKey, tList);
	}
	if (vLists.empty())
		return;

	const int iCheater = F::PlayerUtils.TagToIndex(CHEATER_TAG);
	const int iSuspected = F::PlayerUtils.TagToIndex(SUSPECTED_CHEATER_TAG);
	const int iSuspicious = F::PlayerUtils.TagToIndex(SUSPICIOUS_TAG);
	const int iExploiter = F::PlayerUtils.TagToIndex(EXPLOITER_TAG);
	const int iRacist = F::PlayerUtils.TagToIndex(RACIST_TAG);
	const int iBlacklisted = F::PlayerUtils.TagToIndex(BLACKLISTED_TAG);

	std::vector<std::pair<uint32_t, int>> vMarks;
	std::unordered_set<uint32_t> vPlayers;
	for (auto& [sKey, tList] : vLists)
	{
		const bool bConfirmed = IsConfirmedList(sKey);
		const bool bBlacklist = sKey == "playerlist.nemesisblacklist";
		for (auto& [uAccountID, vAttributes] : tList.m_mPlayers)
		{
			if (!uAccountID)
				continue;

			if (bBlacklist)
			{
				vMarks.emplace_back(uAccountID, iBlacklisted);
				vPlayers.insert(uAccountID);
				continue;
			}

			for (auto& sAttr : vAttributes)
			{
				if (sAttr == "cheater" || sAttr == "bot")
					vMarks.emplace_back(uAccountID, bConfirmed ? iCheater : iSuspected);
				else if (sAttr == "suspicious" || sAttr == "unsure")
					vMarks.emplace_back(uAccountID, iSuspicious);
				else if (sAttr == "exploiter")
					vMarks.emplace_back(uAccountID, iExploiter);
				else if (sAttr == "racist")
					vMarks.emplace_back(uAccountID, iRacist);
			}
			vPlayers.insert(uAccountID);
		}
	}

	int iAdded = 0;
	int iCreated = 0;
	{
		std::lock_guard tLock(F::Menu.m_tMutex);
		for (auto& [uAccountID, iTag] : vMarks)
		{
			if (iTag == iSuspected && F::PlayerUtils.HasTag(uAccountID, iCheater))
				continue; // no need to suspect a confirmed cheater

			const bool bNew = !F::PlayerUtils.HasTags(uAccountID);
			if (!F::PlayerUtils.HasTag(uAccountID, iTag))
			{
				F::PlayerUtils.AddTag(uAccountID, iTag, true, nullptr);
				iAdded++;
				if (bNew)
					iCreated++;
			}
		}
	}

	SDK::Output("TF2BD", std::format("Marked {} of {} listed player(s) from TF2BD lists ({} new player entry/entries)", iAdded, vPlayers.size(), iCreated).c_str(),
		INFO_COLOR, OUTPUT_MENU | OUTPUT_DEBUG | OUTPUT_CONSOLE, ICON_MD_GAVEL);
}

void CSteamBans::OnChatMessage(uint32_t uAccountID, const std::string& sText)
{
	if (G::Unload || !uAccountID || sText.empty())
		return;

	const bool bRacist = IsRacistText(sText);
	const bool bPedo = IsPedoText(sText);
	if (!bRacist && !bPedo)
		return;

	std::lock_guard tLock(F::Menu.m_tMutex);
	if (bRacist)
	{
		const int iTag = F::PlayerUtils.TagToIndex(RACIST_TAG);
		if (!F::PlayerUtils.HasTag(uAccountID, iTag))
			F::PlayerUtils.AddTag(uAccountID, iTag, true, nullptr);
	}
	if (bPedo)
	{
		const int iTag = F::PlayerUtils.TagToIndex(PEDO_TAG);
		if (!F::PlayerUtils.HasTag(uAccountID, iTag))
			F::PlayerUtils.AddTag(uAccountID, iTag, true, nullptr);
	}
}

bool CSteamBans::HasTF2BD(uint32_t uAccountID)
{
	if (!uAccountID)
		return false;

	std::lock_guard lock(m_Mutex);
	for (auto& [sKey, tList] : m_mTF2BDLists)
	{
		if (tList.m_mPlayers.contains(uAccountID))
			return true;
	}
	return false;
}

std::vector<std::string> CSteamBans::GetTF2BDTooltips(uint32_t uAccountID)
{
	std::vector<std::string> vOut;
	if (!uAccountID)
		return vOut;

	std::lock_guard lock(m_Mutex);
	for (auto& [sKey, tList] : m_mTF2BDLists)
	{
		auto it = tList.m_mPlayers.find(uAccountID);
		if (it == tList.m_mPlayers.end())
			continue;

		std::string sLine = tList.m_sTitle.empty() ? sKey : tList.m_sTitle;
		if (!it->second.empty())
		{
			sLine += " [";
			for (size_t i = 0; i < it->second.size(); i++)
			{
				if (i)
					sLine += ", ";
				sLine += it->second[i];
			}
			sLine += "]";
		}
		vOut.push_back(std::move(sLine));
	}
	return vOut;
}

void CSteamBans::EnsureWorker()
{
	if (m_bRunning)
		return;

	m_bUnload = false;
	m_bRunning = true;

	LoadCache();
	m_Worker = std::thread([this] { WorkerMain(); });
}

void CSteamBans::WorkerMain()
{
	constexpr int iMaxBatch = 100;

	while (!m_bUnload)
	{
		std::vector<uint32_t> vBatch;
		std::vector<std::string> vLists;
		{
			std::unique_lock lock(m_Mutex);
			m_Cond.wait_for(lock, std::chrono::milliseconds(300), [this] { return !m_vPending.empty() || !m_vTF2BDPending.empty() || m_bUnload; });
			if (m_bUnload)
				break;

			vLists.swap(m_vTF2BDPending);

			auto it = m_vPending.begin();
			while (it != m_vPending.end() && vBatch.size() < iMaxBatch)
			{
				vBatch.push_back(*it);
				it = m_vPending.erase(it);
			}
		}

		if (!vLists.empty())
		{
			int iUpdated = 0;
			for (const auto& sKey : vLists)
			{
				if (FetchTF2BDList(sKey))
					iUpdated++;
			}
			if (iUpdated)
			{
				std::lock_guard lock(m_Mutex);
				m_vListMessages.push_back(std::format("Updated {} TF2BD player list(s)", iUpdated));
			}
			continue;
		}

		if (vBatch.empty())
			continue;

		if (!Vars::Menu::Bans::SteamKey.Value.empty())
			FetchSteamBans(vBatch);

		if (!Vars::Menu::Bans::SteamHistoryKey.Value.empty())
			FetchSourceBans(vBatch);

		{
			std::lock_guard lock(m_Mutex);
			for (const uint32_t uAccountID : vBatch)
			{
				if (m_mAlerted.contains(uAccountID))
					continue;

				auto it = m_mBans.find(uAccountID);
				if (it == m_mBans.end())
					continue;

				const SteamBanData_t& tBan = it->second;
				if (!(tBan.m_iGameBans > 0 || tBan.m_bHasActiveSourceBan || HasCheatEvidence(tBan)))
					continue;

				m_mAlerted.insert(uAccountID);
				m_vAlerts.push_back(uAccountID);
			}
		}

		if (m_bCacheDirty)
			SaveCache();
	}

	m_bRunning = false;
}

bool CSteamBans::FetchURL(const std::string& sHost, const std::string& sPath, bool bSecure, std::string& sOut)
{
	static WinHttpApi sApi;
	if (!sApi.Load())
		return false;

	const std::wstring wsHost(sHost.begin(), sHost.end());
	const std::wstring wsPath(sPath.begin(), sPath.end());
	const std::wstring wsUserAgent = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";

	HINTERNET hSession = sApi.Open(wsUserAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return false;

	sApi.SetTimeouts(hSession, 5000, 5000, 10000, 10000);

	HINTERNET hConnect = sApi.Connect(hSession, wsHost.c_str(), bSecure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect)
	{
		sApi.CloseHandle(hSession);
		return false;
	}

	HINTERNET hRequest = sApi.OpenRequest(hConnect, L"GET", wsPath.c_str(), nullptr, nullptr, nullptr, bSecure ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest)
	{
		sApi.CloseHandle(hConnect);
		sApi.CloseHandle(hSession);
		return false;
	}

	auto fCleanup = [&]()
	{
		sApi.CloseHandle(hRequest);
		sApi.CloseHandle(hConnect);
		sApi.CloseHandle(hSession);
	};

	if (!sApi.SendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0))
	{
		fCleanup();
		return false;
	}

	if (!sApi.ReceiveResponse(hRequest, nullptr))
	{
		fCleanup();
		return false;
	}

	DWORD dwStatusCode = 0;
	DWORD dwCodeSize = sizeof(dwStatusCode);
	if (!sApi.QueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwCodeSize, WINHTTP_NO_HEADER_INDEX) || dwStatusCode != 200)
	{
		fCleanup();
		return false;
	}

	DWORD dwAvailable = 0;
	while (sApi.QueryDataAvailable(hRequest, &dwAvailable) && dwAvailable)
	{
		std::string sChunk(dwAvailable, '\0');
		DWORD dwRead = 0;
		if (!sApi.ReadData(hRequest, sChunk.data(), dwAvailable, &dwRead))
			break;
		if (dwRead)
		{
			sChunk.resize(dwRead);
			sOut += sChunk;
		}
	}

	fCleanup();
	return !sOut.empty();
}

void CSteamBans::FetchSteamBans(const std::vector<uint32_t>& vAccountIDs)
{
	std::unordered_map<uint64_t, uint32_t> mByID;
	std::string sIDs;
	for (auto uAccountID : vAccountIDs)
	{
		const uint64_t uID64 = GetSteamID64(uAccountID);
		if (mByID.contains(uID64))
			continue;
		mByID[uID64] = uAccountID;
		if (!sIDs.empty())
			sIDs += ",";
		sIDs += std::to_string(uID64);
	}

	const std::string sPath = std::format("/ISteamUser/GetPlayerBans/v1/?key={}&steamids={}", Vars::Menu::Bans::SteamKey.Value, sIDs);

	std::string sResponse;
	if (!FetchURL("api.steampowered.com", sPath, true, sResponse))
		return;

	try
	{
		boost::property_tree::ptree tRoot;
		std::stringstream ss(sResponse);
		boost::property_tree::read_json(ss, tRoot);

		if (!tRoot.get_child_optional("players"))
			return;

		const uint64_t uNow = (uint64_t)std::time(nullptr);
		for (auto& [sKey, tPlayer] : tRoot.get_child("players"))
		{
			const std::string sSteamId = GetOpt<std::string>(tPlayer, "SteamId", "0");
			uint64_t uID64 = 0;
			try
			{
				uID64 = std::stoull(sSteamId);
			}
			catch (...)
			{
				continue;
			}

			const auto it = mByID.find(uID64);
			if (it == mByID.end())
				continue;

			const bool bVAC = GetOpt<bool>(tPlayer, "VACBanned", false);
			const int iVACBans = GetOpt<int>(tPlayer, "NumberOfVACBans", 0);
			const int iGameBans = GetOpt<int>(tPlayer, "NumberOfGameBans", 0);
			const int iDaysSince = GetOpt<int>(tPlayer, "DaysSinceLastBan", 0);

			std::lock_guard lock(m_Mutex);
			auto& tBan = m_mBans[it->second];
			tBan.m_uSteamID64 = uID64;
			tBan.m_uLastUpdate = uNow;
			tBan.m_bVACBanned = bVAC;
			tBan.m_iVACBans = iVACBans;
			tBan.m_iDaysSinceLastVAC = bVAC ? iDaysSince : 0;
			tBan.m_bGameBanned = iGameBans > 0;
			tBan.m_iGameBans = iGameBans;
			tBan.m_iDaysSinceLastGame = !bVAC && tBan.m_bGameBanned ? iDaysSince : 0;
			tBan.m_bCommunityBanned = GetOpt<bool>(tPlayer, "CommunityBanned", false);
			tBan.m_sEconomyBan = GetOpt<std::string>(tPlayer, "EconomyBan", "none");
			m_bCacheDirty = true;
		}
	}
	catch (...)
	{
	}
}

void CSteamBans::FetchSourceBans(const std::vector<uint32_t>& vAccountIDs)
{
	std::unordered_map<uint64_t, uint32_t> mByID;
	std::unordered_map<uint32_t, uint64_t> mByAccount;
	std::string sIDs;
	for (auto uAccountID : vAccountIDs)
	{
		const uint64_t uID64 = GetSteamID64(uAccountID);
		if (mByID.contains(uID64))
			continue;
		mByID[uID64] = uAccountID;
		mByAccount[uAccountID] = uID64;
		if (!sIDs.empty())
			sIDs += ",";
		sIDs += std::to_string(uID64);
	}

	const std::string sPath = std::format("/api/sourcebans?key={}&steamids={}", Vars::Menu::Bans::SteamHistoryKey.Value, sIDs);

	std::string sResponse;
	if (!FetchURL("steamhistory.net", sPath, true, sResponse))
		return;

	try
	{
		boost::property_tree::ptree tRoot;
		std::stringstream ss(sResponse);
		boost::property_tree::read_json(ss, tRoot);

		if (!tRoot.get_child_optional("response"))
			return;

		const uint64_t uNow = (uint64_t)std::time(nullptr);

		std::unordered_map<uint32_t, std::vector<SourceBan_t>> mBans;
		for (auto& [sKey, tEntry] : tRoot.get_child("response"))
		{
			const std::string sSteamId = GetOpt<std::string>(tEntry, "SteamID", "0");
			uint64_t uID64 = 0;
			try
			{
				uID64 = std::stoull(sSteamId);
			}
			catch (...)
			{
				continue;
			}

			const auto it = mByID.find(uID64);
			if (it == mByID.end())
				continue;

			SourceBan_t tSource = {};
			tSource.m_sName = GetOpt<std::string>(tEntry, "Name", "");
			tSource.m_sState = GetOpt<std::string>(tEntry, "CurrentState", "");
			tSource.m_sReason = GetOpt<std::string>(tEntry, "BanReason", "");
			tSource.m_sUnbanReason = GetOpt<std::string>(tEntry, "UnbanReason", "");
			tSource.m_uBanTimestamp = GetOpt<uint64_t>(tEntry, "BanTimestamp", 0);
			tSource.m_uUnbanTimestamp = GetOpt<uint64_t>(tEntry, "UnbanTimestamp", 0);
			tSource.m_sServer = GetOpt<std::string>(tEntry, "Server", "");
			mBans[it->second].emplace_back(std::move(tSource));
		}

		{
			std::lock_guard lock(m_Mutex);
			for (auto& [uAccountID, uID64] : mByAccount)
			{
				auto& tBan = m_mBans[uAccountID];
				tBan.m_uSteamID64 = uID64;
				tBan.m_uLastUpdate = uNow;

				auto it = mBans.find(uAccountID);
				if (it != mBans.end())
				{
					tBan.m_vSourceBans = std::move(it->second);
					tBan.m_bHasActiveSourceBan = false;
					for (auto& tSource : tBan.m_vSourceBans)
					{
						if (!tSource.m_sState.empty() && (tSource.m_sState == "Permanent" || tSource.m_sState == "Temp-Ban"))
						{
							tBan.m_bHasActiveSourceBan = true;
							break;
						}
					}
				}
				else
					tBan.m_vSourceBans.clear();

				m_bCacheDirty = true;
			}
		}
	}
	catch (...)
	{
	}
}

bool CSteamBans::FetchTF2BDList(const std::string& sKey)
{
	std::string sURL;
	TF2BDList_t tList;
	{
		std::lock_guard lock(m_Mutex);
		auto it = m_mTF2BDLists.find(sKey);
		if (it == m_mTF2BDLists.end())
			return false;
		sURL = it->second.m_sUpdateURL;
		tList = it->second;
	}

	if (sURL.empty())
		return false;

	std::string sHost, sPath = "/";
	bool bSecure = false;
	const auto iScheme = sURL.find("://");
	if (iScheme != std::string::npos)
	{
		bSecure = sURL.compare(0, iScheme, "https") == 0;
		const std::string sRest = sURL.substr(iScheme + 3);
		const auto iSlash = sRest.find('/');
		if (iSlash == std::string::npos)
			sHost = sRest;
		else
		{
			sHost = sRest.substr(0, iSlash);
			sPath = sRest.substr(iSlash);
		}
	}
	else
		sHost = sURL;

	if (sHost.empty())
		return false;

	std::string sResponse;
	if (!FetchURL(sHost, sPath, bSecure, sResponse))
		return false;

	TF2BDList_t tFetched = tList;
	if (!ParseTF2BDJSON(sResponse, tFetched))
		return false;

	{
		std::lock_guard lock(m_Mutex);
		m_mTF2BDLists[sKey] = tFetched;
	}

	SaveTF2BDList(sKey, tFetched);
	return true;
}

void CSteamBans::SaveTF2BDList(const std::string& sKey, const TF2BDList_t& tList)
{
	boost::property_tree::ptree tRoot;
	tRoot.put("$schema", "https://raw.githubusercontent.com/PazerOP/tf2_bot_detector/master/schemas/v3/playerlist.schema.json");

	boost::property_tree::ptree tFileInfo;
	tFileInfo.put("title", tList.m_sTitle);
	tFileInfo.put("description", tList.m_sDescription);
	tFileInfo.put("update_url", tList.m_sUpdateURL);
	boost::property_tree::ptree tAuthors;
	boost::property_tree::ptree tAuthor;
	tAuthor.put("", "Phobia TF2BD");
	tAuthors.push_back(std::make_pair("", tAuthor));
	tFileInfo.put_child("authors", tAuthors);
	tRoot.put_child("file_info", tFileInfo);

	for (auto& [uAccountID, vAttributes] : tList.m_mPlayers)
	{
		boost::property_tree::ptree tPlayer;
		tPlayer.put("steamid", std::format("[U:1:{}]", uAccountID));

		boost::property_tree::ptree tAttributes;
		for (auto& sAttr : vAttributes)
		{
			boost::property_tree::ptree tAttr;
			tAttr.put("", sAttr);
			tAttributes.push_back(std::make_pair("", tAttr));
		}
		tPlayer.put_child("attributes", tAttributes);

		tRoot.push_back(std::make_pair("players", tPlayer));
	}

	try
	{
		const std::string sPath = F::Configs.m_sCorePath + "Lists\\" + sKey + ".json";
		std::ofstream file(sPath, std::ios::trunc);
		if (!file.is_open())
			return;
		boost::property_tree::write_json(file, tRoot);
	}
	catch (...)
	{
	}
}

void CSteamBans::SaveCache()
{
	boost::property_tree::ptree tRoot;

	std::vector<std::pair<uint32_t, SteamBanData_t>> vSnap;
	{
		std::lock_guard lock(m_Mutex);
		for (auto& [uAccountID, tBan] : m_mBans)
			vSnap.emplace_back(uAccountID, tBan);
	}

	for (auto& [uAccountID, tBan] : vSnap)
	{
		boost::property_tree::ptree tPlayer;
		tPlayer.put("AccountID", uAccountID);
		tPlayer.put("SteamID64", tBan.m_uSteamID64);
		tPlayer.put("LastUpdate", tBan.m_uLastUpdate);
		tPlayer.put("VACBanned", tBan.m_bVACBanned);
		tPlayer.put("VACBans", tBan.m_iVACBans);
		tPlayer.put("DaysSinceLastVAC", tBan.m_iDaysSinceLastVAC);
		tPlayer.put("GameBanned", tBan.m_bGameBanned);
		tPlayer.put("GameBans", tBan.m_iGameBans);
		tPlayer.put("DaysSinceLastGame", tBan.m_iDaysSinceLastGame);
		tPlayer.put("CommunityBanned", tBan.m_bCommunityBanned);
		tPlayer.put("EconomyBan", tBan.m_sEconomyBan);
		tPlayer.put("ActiveSourceBan", tBan.m_bHasActiveSourceBan);

		for (auto& tSource : tBan.m_vSourceBans)
		{
			boost::property_tree::ptree tSourceTree;
			tSourceTree.put("Name", tSource.m_sName);
			tSourceTree.put("State", tSource.m_sState);
			tSourceTree.put("Reason", tSource.m_sReason);
			tSourceTree.put("UnbanReason", tSource.m_sUnbanReason);
			tSourceTree.put("BanTimestamp", tSource.m_uBanTimestamp);
			tSourceTree.put("UnbanTimestamp", tSource.m_uUnbanTimestamp);
			tSourceTree.put("Server", tSource.m_sServer);
			tPlayer.push_back(std::make_pair("SourceBans", tSourceTree));
		}

		tRoot.push_back(std::make_pair("Players", tPlayer));
	}

	try
	{
		std::ofstream file(F::Configs.m_sCorePath + "SteamBans.json", std::ios::trunc);
		if (!file.is_open())
			return;
		boost::property_tree::write_json(file, tRoot);
	}
	catch (...)
	{
	}

	m_bCacheDirty = false;
}

void CSteamBans::LoadCache()
{
	if (m_bCacheLoaded)
		return;

	try
	{
		std::ifstream file(F::Configs.m_sCorePath + "SteamBans.json");
		if (!file.is_open())
			return;

		boost::property_tree::ptree tRoot;
		boost::property_tree::read_json(file, tRoot);

		if (!tRoot.get_child_optional("Players"))
			return;

		std::unordered_map<uint32_t, SteamBanData_t> mLoaded;
		for (auto& [sKey, tPlayer] : tRoot.get_child("Players"))
		{
			const uint32_t uAccountID = GetOpt<uint32_t>(tPlayer, "AccountID", 0);
			if (!uAccountID)
				continue;

			SteamBanData_t tBan;
			tBan.m_uSteamID64 = GetOpt<uint64_t>(tPlayer, "SteamID64", 0);
			tBan.m_uLastUpdate = GetOpt<uint64_t>(tPlayer, "LastUpdate", 0);
			tBan.m_bVACBanned = GetOpt<bool>(tPlayer, "VACBanned", false);
			tBan.m_iVACBans = GetOpt<int>(tPlayer, "VACBans", 0);
			tBan.m_iDaysSinceLastVAC = GetOpt<int>(tPlayer, "DaysSinceLastVAC", 0);
			tBan.m_bGameBanned = GetOpt<bool>(tPlayer, "GameBanned", false);
			tBan.m_iGameBans = GetOpt<int>(tPlayer, "GameBans", 0);
			tBan.m_iDaysSinceLastGame = GetOpt<int>(tPlayer, "DaysSinceLastGame", 0);
			tBan.m_bCommunityBanned = GetOpt<bool>(tPlayer, "CommunityBanned", false);
			tBan.m_sEconomyBan = GetOpt<std::string>(tPlayer, "EconomyBan", "none");
			tBan.m_bHasActiveSourceBan = GetOpt<bool>(tPlayer, "ActiveSourceBan", false);

			if (auto o = tPlayer.get_child_optional("SourceBans"))
			{
				for (auto& [sKey2, tSource] : *o)
				{
					SourceBan_t tSourceBan;
					tSourceBan.m_sName = GetOpt<std::string>(tSource, "Name", "");
					tSourceBan.m_sState = GetOpt<std::string>(tSource, "State", "");
					tSourceBan.m_sReason = GetOpt<std::string>(tSource, "Reason", "");
					tSourceBan.m_sUnbanReason = GetOpt<std::string>(tSource, "UnbanReason", "");
					tSourceBan.m_uBanTimestamp = GetOpt<uint64_t>(tSource, "BanTimestamp", 0);
					tSourceBan.m_uUnbanTimestamp = GetOpt<uint64_t>(tSource, "UnbanTimestamp", 0);
					tSourceBan.m_sServer = GetOpt<std::string>(tSource, "Server", "");
					tBan.m_vSourceBans.emplace_back(std::move(tSourceBan));
				}
			}

			mLoaded[uAccountID] = std::move(tBan);
		}

		{
			std::lock_guard lock(m_Mutex);
			m_mBans = std::move(mLoaded);
		}
	}
	catch (...)
	{
	}

	m_bCacheLoaded = true;
}