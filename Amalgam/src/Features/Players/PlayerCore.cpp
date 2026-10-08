#include "PlayerCore.h"

#include "PlayerUtils.h"
#include "../Configs/Configs.h"

#include <fstream>
#include <sstream>

void CPlayerlistCore::Run()
{
	static Timer tTimer = {};
	if (!tTimer.Run(1.f))
		return;

	LoadPlayerlist();
	SavePlayerlist();
}

void CPlayerlistCore::SavePlayerlist()
{
	if (!F::PlayerUtils.m_bSave || F::PlayerUtils.m_bLoad) // terrible if we end up saving while loading
		return;

	F::PlayerUtils.m_bSave = false;

	try
	{
		boost::property_tree::ptree tWrite;

		{
			boost::property_tree::ptree tSub;
			for (auto it = F::PlayerUtils.m_vTags.begin(); it != F::PlayerUtils.m_vTags.end(); it++)
			{
				int iID = std::distance(F::PlayerUtils.m_vTags.begin(), it);
				auto& tTag = *it;

				boost::property_tree::ptree tChild;
				F::Configs.SaveJson(tChild, "Name", tTag.m_sName);
				F::Configs.SaveJson(tChild, "Color", tTag.m_tColor);
				F::Configs.SaveJson(tChild, "Priority", tTag.m_iPriority);
				F::Configs.SaveJson(tChild, "Label", tTag.m_bLabel);

				tSub.put_child(std::to_string(F::PlayerUtils.IndexToTag(iID)), tChild);
			}
			tWrite.put_child("Config", tSub);
		}

		{
			boost::property_tree::ptree tSub;
			for (auto& [uAccountID, vTags] : F::PlayerUtils.m_mPlayerTags)
			{
				if (vTags.empty())
					continue;

				boost::property_tree::ptree tChild;
				for (auto& iID : vTags)
				{
					boost::property_tree::ptree t;
					t.put("", F::PlayerUtils.IndexToTag(iID));
					tChild.push_back({ "", t });
				}

				tSub.put_child(std::to_string(uAccountID), tChild);
			}
			tWrite.put_child("Tags", tSub);
		}

		{
			boost::property_tree::ptree tSub;
			for (auto& [uAccountID, sAlias] : F::PlayerUtils.m_mPlayerAliases)
			{
				if (!sAlias.empty())
					tSub.put(std::to_string(uAccountID), sAlias);
			}
			tWrite.put_child("Aliases", tSub);
		}

		{
			boost::property_tree::ptree tSub;
			for (auto& [uAccountID, sName] : F::PlayerUtils.m_mPlayerNames)
			{
				if (!sName.empty())
					tSub.put(std::to_string(uAccountID), sName);
			}
			tWrite.put_child("LastSeenNames", tSub);
		}

		const std::string sPath = F::Configs.m_sCorePath + "Players.json";

		// skip the disk write when nothing actually changed: m_bSave is set by
		// several runtime paths (name lookups, transient join/leave churn), and
		// rewriting an identical snapshot every second is wasted main-thread time
		std::stringstream ss;
		boost::property_tree::write_json(ss, tWrite);
		const std::string sJson = ss.str();

		static std::string sLastSaved;
		if (sJson == sLastSaved)
			return;

		std::ofstream file(sPath);
		if (!file.is_open())
			return;
		file << sJson;
		sLastSaved = sJson;

		SDK::Output("Phobia", "Saved playerlist", INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_INFO);
	}
	catch (...)
	{
		SDK::Output("Phobia", "Save playerlist failed", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);
	}
}

void CPlayerlistCore::LoadPlayerlist()
{
	if (!F::PlayerUtils.m_bLoad)
		return;

	try
	{
		if (!std::filesystem::exists(F::Configs.m_sCorePath + "Players.json"))
		{
			F::PlayerUtils.m_bPlayerlistLoaded = true;
			F::PlayerUtils.m_bDatabaseLoaded = true;
			F::PlayerUtils.m_bLoad = false;
			return;
		}

		boost::property_tree::ptree tRead;
		read_json(F::Configs.m_sCorePath + "Players.json", tRead);

		F::PlayerUtils.m_mPlayerTags.clear();
		F::PlayerUtils.m_mPlayerAliases.clear();
		F::PlayerUtils.m_mPlayerNames.clear();
		F::PlayerUtils.m_vTags = {
			{ "Default", { 200, 200, 200, 255 }, 0, false, false, true },
			{ "Ignored", { 200, 200, 200, 255 }, -1, false, true, true },
			{ "Cheater", { 255, 100, 100, 255 }, 1, false, true, true },
			{ "Friend", { 100, 255, 100, 255 }, 0, true, false, true },
			{ "Party", { 100, 50, 255, 255 }, 0, true, false, true },
			{ "F2P", { 255, 255, 255, 255 }, 0, true, false, true },
			{ "Suspected Cheater", { 255, 170, 40, 255 }, 1, false, false, true },
			{ "Suspicious", { 255, 215, 110, 255 }, 0, false, false, true },
			{ "Exploiter", { 255, 130, 200, 255 }, 1, false, false, true },
			{ "Racist", { 200, 120, 255, 255 }, 1, false, false, true },
			{ "Blacklisted", { 255, 50, 50, 255 }, 2, false, false, true },
			{ "VAC Banned", { 255, 90, 90, 255 }, 0, true, false, true },
			{ "Game Banned", { 200, 160, 60, 255 }, 0, true, false, true },
			{ "SourceBanned", { 180, 130, 255, 255 }, 0, true, false, true },
			{ "Pedo Jokes", { 0, 170, 150, 255 }, 1, false, false, true }
		};

		if (auto tSub = tRead.get_child_optional("Config"))
		{
			for (auto& [sName, tChild] : *tSub)
			{
				PriorityLabel_t tTag = {};
				F::Configs.LoadJson(tChild, "Name", tTag.m_sName);
				F::Configs.LoadJson(tChild, "Color", tTag.m_tColor);
				F::Configs.LoadJson(tChild, "Priority", tTag.m_iPriority);
				F::Configs.LoadJson(tChild, "Label", tTag.m_bLabel);

				int iID = F::PlayerUtils.TagToIndex(std::stoi(sName));
				if (iID > -1 && iID < F::PlayerUtils.m_vTags.size())
				{
					F::PlayerUtils.m_vTags[iID].m_sName = tTag.m_sName;
					F::PlayerUtils.m_vTags[iID].m_tColor = tTag.m_tColor;
					F::PlayerUtils.m_vTags[iID].m_iPriority = tTag.m_iPriority;
					F::PlayerUtils.m_vTags[iID].m_bLabel = tTag.m_bLabel;
				}
				else
					F::PlayerUtils.m_vTags.push_back(tTag);
			}
		}
		else
			SDK::Output("Phobia", "Playerlist config not found", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);

		if (auto tSub = tRead.get_child_optional("Tags"))
		{
			for (auto& [sName, tChild] : *tSub)
			{
				uint32_t uAccountID = std::stoul(sName);
				for (auto& tTag : tChild | std::views::values)
				{
					const std::string& sTag = tTag.data();

					int iID = F::PlayerUtils.TagToIndex(std::stoi(sTag));
					auto pTag = F::PlayerUtils.GetTag(iID);
					if (!pTag || pTag->m_bLabel)
						continue;

					if (!F::PlayerUtils.HasTag(uAccountID, iID))
						F::PlayerUtils.AddTag(uAccountID, iID, false);
				}
			}
		}
		else
			SDK::Output("Phobia", "Playerlist tags not found", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);

		if (auto tSub = tRead.get_child_optional("Aliases"))
		{
			for (auto& [sName, jAlias] : *tSub)
			{
				uint32_t uAccountID = std::stoul(sName);
				const std::string& sAlias = jAlias.data();

				if (!sAlias.empty())
					F::PlayerUtils.m_mPlayerAliases[uAccountID] = sAlias;
			}
		}
		else
			SDK::Output("Phobia", "Playerlist aliases not found", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);

		auto tOpt = tRead.get_child_optional("LastSeenNames");
		if (!tOpt)
			tOpt = tRead.get_child_optional("Names"); // migrate older playerlists
		if (auto tSub = tOpt)
		{
			for (auto& [sName, jName] : *tSub)
			{
				uint32_t uAccountID = std::stoul(sName);
				const std::string& sPlayerName = jName.data();

				if (!sPlayerName.empty())
					F::PlayerUtils.m_mPlayerNames[uAccountID] = sPlayerName;
			}
		}

		// read-only curated masterlist (Database.json), takes precedence over local marks
		F::PlayerUtils.m_mDatabaseTags.clear();
		if (std::filesystem::exists(F::Configs.m_sCorePath + "Database.json"))
		{
			try
			{
				boost::property_tree::ptree tDb;
				read_json(F::Configs.m_sCorePath + "Database.json", tDb);

				// masterlist tag definitions override the local ones (in-memory only, never written back)
				if (auto tSub = tDb.get_child_optional("Config"))
				{
					for (auto& [sName, tChild] : *tSub)
					{
						PriorityLabel_t tTag = {};
						F::Configs.LoadJson(tChild, "Name", tTag.m_sName);
						F::Configs.LoadJson(tChild, "Color", tTag.m_tColor);
						F::Configs.LoadJson(tChild, "Priority", tTag.m_iPriority);
						F::Configs.LoadJson(tChild, "Label", tTag.m_bLabel);

						int iID = F::PlayerUtils.TagToIndex(std::stoi(sName));
						if (iID > -1 && iID < F::PlayerUtils.m_vTags.size())
						{
							F::PlayerUtils.m_vTags[iID].m_sName = tTag.m_sName;
							F::PlayerUtils.m_vTags[iID].m_tColor = tTag.m_tColor;
							F::PlayerUtils.m_vTags[iID].m_iPriority = tTag.m_iPriority;
							F::PlayerUtils.m_vTags[iID].m_bLabel = tTag.m_bLabel;
						}
						else
							F::PlayerUtils.m_vTags.push_back(tTag);
					}
				}

				if (auto tSub = tDb.get_child_optional("Tags"))
				{
					for (auto& [sName, tChild] : *tSub)
					{
						uint32_t uAccountID = std::stoul(sName);
						std::vector<int> vTags;
						for (auto& tTag : tChild | std::views::values)
						{
							const std::string& sTag = tTag.data();

							int iID = F::PlayerUtils.TagToIndex(std::stoi(sTag));
							auto pTag = F::PlayerUtils.GetTag(iID);
							if (!pTag || pTag->m_bLabel)
								continue;

							if (std::find(vTags.begin(), vTags.end(), iID) == vTags.end())
								vTags.push_back(iID);
						}
						if (!vTags.empty())
							F::PlayerUtils.m_mDatabaseTags[uAccountID] = std::move(vTags);
					}
				}

				SDK::Output("Phobia", "Loaded playerlist database", INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_INFO);
			}
			catch (...)
			{
				// a corrupt masterlist must not break the local playerlist
				F::PlayerUtils.m_mDatabaseTags.clear();
				SDK::Output("Phobia", "Failed to load playerlist database", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);
			}
		}

		SDK::Output("Phobia", "Loaded playerlist", INFO_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_INFO);
	}
	catch (...)
	{
		SDK::Output("Phobia", "Load playerlist failed", ERROR_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG, ICON_MD_CANCEL);
	}

	F::PlayerUtils.m_bPlayerlistLoaded = true;
	F::PlayerUtils.m_bDatabaseLoaded = true;
	F::PlayerUtils.m_bLoad = false;
}