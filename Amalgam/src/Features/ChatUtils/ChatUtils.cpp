#include "ChatUtils.h"

#include "../Configs/Configs.h"
#include "../Players/PlayerUtils.h"

#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace
{
	void TrimString(std::string& sInput)
	{
		sInput.erase(sInput.begin(), std::find_if(sInput.begin(), sInput.end(), [](unsigned char c) { return !std::isspace(c); }));
		sInput.erase(std::find_if(sInput.rbegin(), sInput.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), sInput.end());
	}

	std::vector<std::string> ReadFileLines(const std::filesystem::path& sPath)
	{
		std::vector<std::string> vLines = {};
		std::ifstream f(sPath);
		if (!f.is_open())
			return vLines;

		std::string sLine;
		while (std::getline(f, sLine))
		{
			TrimString(sLine);
			if (sLine.empty() || sLine.rfind("//", 0) == 0)
				continue;
			vLines.push_back(sLine);
		}
		return vLines;
	}

	void SplitTokens(const std::string& sSource, char cDelimiter, std::vector<std::string>& vOut)
	{
		size_t iPos = 0;
		while (iPos <= sSource.size())
		{
			auto iEnd = sSource.find(cDelimiter, iPos);
			if (iEnd == std::string::npos)
				iEnd = sSource.size();

			std::string sToken = sSource.substr(iPos, iEnd - iPos);
			TrimString(sToken);
			if (!sToken.empty())
				vOut.push_back(sToken);

			if (iEnd == sSource.size())
				break;
			iPos = iEnd + 1;
		}
	}

	void ReplaceTag(std::string& sResult, const char* sTag, const char* sValue)
	{
		if (!sValue || !sValue[0])
			return;

		const size_t iTag = strlen(sTag);
		size_t iPos = 0;
		while ((iPos = sResult.find(sTag, iPos)) != std::string::npos)
		{
			sResult.replace(iPos, iTag, sValue);
			iPos += strlen(sValue);
		}
	}
}

void CChatUtils::Run()
{
	if (SDK::PlatFloatTime() < m_flNextReload)
		return;

	m_flNextReload = SDK::PlatFloatTime() + 5.f;
	LoadFiles();
}

void CChatUtils::LoadFiles()
{
	const std::filesystem::path sPath = F::Configs.m_sConfigPath;
	const auto sKillSayPath = sPath / "killsay.txt";
	const auto sAutoReplyPath = sPath / "autoreply.txt";

	if (!std::filesystem::exists(sPath))
		std::filesystem::create_directories(sPath);

	if (!std::filesystem::exists(sKillSayPath))
	{
		std::ofstream(sKillSayPath) <<
			"// KillSay - lines sent to chat when YOU kill another player.\n"
			"// One message per line. Lines starting with // and empty lines are ignored.\n"
			"// Edit & save: the file is reloaded automatically (within ~5 seconds).\n"
			"// Available tags (only replaced when data is available):\n"
			"//   {killer} {victim} {target} {triggername} {initiator} {enemyteam} {friendlyteam}\n"
			"//   {lastkilled} {highestscore} {random} {friend} {ignored}\n"
			"\n"
			"Nice shot, {victim}!\n"
			"Get rekt, {victim}\n"
			"ez {victim}\n"
			"pay to win? no, skill {killer}\n";
	}

	if (!std::filesystem::exists(sAutoReplyPath))
	{
		std::ofstream(sAutoReplyPath) <<
			"// AutoReply - sends a reply when a player's chat message contains a trigger.\n"
			"// Format per line: trigger1, trigger2 : reply1, reply2\n"
			"// Triggers & replies are comma separated lists. Matching is case-insensitive substring.\n"
			"// Exactly one random reply is chosen. Lines starting with // and empty lines are ignored.\n"
			"// Available tags: see killsay.txt\n"
			"\n"
			"gg, ez, ty, thx : gg {target}, ez {target}, np {target}\n"
			"you suck, kys, fag : dont talk to me like that {target}\n";
	}

	m_vKillSay.clear();
	m_vAutoReply.clear();

	for (auto& sLine : ReadFileLines(sKillSayPath))
		m_vKillSay.push_back(sLine);

	for (auto& sLine : ReadFileLines(sAutoReplyPath))
	{
		auto iDelim = sLine.find(':');
		if (iDelim == std::string::npos)
			continue;

		AutoReplyRule_t rule;
		SplitTokens(sLine.substr(0, iDelim), ',', rule.m_vTriggers);
		SplitTokens(sLine.substr(iDelim + 1), ',', rule.m_vReplies);
		if (!rule.m_vTriggers.empty() && !rule.m_vReplies.empty())
			m_vAutoReply.push_back(rule);
	}
}

void CChatUtils::Event(IGameEvent* pEvent, uint32_t uHash, CTFPlayer* pLocal)
{
	if (uHash != FNV1A::Hash32Const("player_death"))
		return;
	if (!pLocal)
		return;

	const int iLocal = I::EngineClient->GetLocalPlayer();
	const int iVictim = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
	const int iAttacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	if (iAttacker == iLocal)
		KillSay(iVictim);
}

void CChatUtils::OnChatMessage(int clientIndex, const std::string& sMessage)
{
	if (clientIndex <= 0 || clientIndex == I::EngineClient->GetLocalPlayer())
		return;

	auto pResource = H::Entities.GetResource();
	if (!pResource || !pResource->IsValid(clientIndex) || !pResource->m_bConnected(clientIndex))
		return;

	std::string sContent = sMessage;
	if (const char* sName = pResource->GetName(clientIndex); sName && sName[0])
	{
		auto iPos = sContent.find(sName);
		if (iPos != std::string::npos)
			sContent = sContent.substr(iPos + strlen(sName));
	}

	while (!sContent.empty() && (sContent.front() == '\x1' || sContent.front() == '\x3' || sContent.front() == ' ' || sContent.front() == ':'))
		sContent.erase(sContent.begin());

	if (sContent.empty())
		return;

	AutoReply(clientIndex, sContent);
}

void CChatUtils::AutoReply(int clientIndex, const std::string& sMessage)
{
	if (m_vAutoReply.empty())
		return;

	const char* sTarget = GetPlayerNameSafe(clientIndex);
	const char* sInitiator = nullptr;
	if (SDK::PlatFloatTime() < m_flVoteExpire)
		sInitiator = GetPlayerNameSafe(m_iVoteCaller);

	auto sInput = sMessage;
	std::transform(sInput.begin(), sInput.end(), sInput.begin(), [](unsigned char c) { return char(std::tolower(c)); });

	for (auto& rule : m_vAutoReply)
	{
		for (auto& sTrigger : rule.m_vTriggers)
		{
			auto sLower = sTrigger;
			std::transform(sLower.begin(), sLower.end(), sLower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
			if (sInput.find(sLower) != std::string::npos)
			{
				const auto& sReply = rule.m_vReplies[SDK::RandomInt(0, int(rule.m_vReplies.size()) - 1)];
				SendChat(Resolve(sReply, nullptr, nullptr, sTarget, sInitiator));
				return;
			}
		}
	}
}

void CChatUtils::KillSay(int iVictim)
{
	if (m_vKillSay.empty() || iVictim <= 0)
		return;

	const char* sVictim = GetPlayerNameSafe(iVictim);
	if (!sVictim)
		return;

	m_sLastKilled = sVictim;

	const char* sTarget = nullptr, * sInitiator = nullptr;
	if (SDK::PlatFloatTime() < m_flVoteExpire)
	{
		sTarget = GetPlayerNameSafe(m_iVoteTarget);
		sInitiator = GetPlayerNameSafe(m_iVoteCaller);
	}

	const int iLocal = I::EngineClient->GetLocalPlayer();
	const auto& sLine = m_vKillSay[SDK::RandomInt(0, int(m_vKillSay.size()) - 1)];
	SendChat(Resolve(sLine, GetPlayerNameSafe(iLocal), sVictim, sTarget, sInitiator));
}

void CChatUtils::SendChat(const std::string& sMessage)
{
	if (sMessage.empty())
		return;

	I::ClientState->SendStringCmd(std::format("say {}", sMessage).c_str());
}

std::string CChatUtils::Resolve(const std::string& sTemplate, const char* sKiller, const char* sVictim, const char* sTarget, const char* sInitiator) const
{
	std::string sResult = sTemplate;

	ReplaceTag(sResult, "{killer}", sKiller);
	ReplaceTag(sResult, "{victim}", sVictim);
	ReplaceTag(sResult, "{target}", sTarget);
	ReplaceTag(sResult, "{triggername}", sTarget);
	ReplaceTag(sResult, "{initiator}", sInitiator);

	ReplaceTag(sResult, "{lastkilled}", m_sLastKilled.c_str());
	ReplaceTag(sResult, "{highestscore}", GetHighestScoreName());
	ReplaceTag(sResult, "{random}", GetRandomPlayerName());
	ReplaceTag(sResult, "{friend}", GetRandomPlayerName(FRIEND_TAG));
	ReplaceTag(sResult, "{ignored}", GetRandomPlayerName(IGNORED_TAG));

	if (const char* sFriendly = GetTeamName(GetLocalTeamNum()))
	{
		ReplaceTag(sResult, "{friendlyteam}", sFriendly);
		if (const char* sEnemy = GetTeamName(5 - GetLocalTeamNum()))
			ReplaceTag(sResult, "{enemyteam}", sEnemy);
	}

	return sResult;
}

bool CChatUtils::IsValidPlayer(int iIndex) const
{
	if (iIndex <= 0 || iIndex > I::EngineClient->GetMaxClients())
		return false;
	if (!H::Entities.GetResource() || H::Entities.GetResource()->IsFakePlayer(iIndex))
		return false;

	auto pResource = H::Entities.GetResource();
	return pResource->m_bValid(iIndex) && pResource->m_bConnected(iIndex);
}

const char* CChatUtils::GetPlayerNameSafe(int iIndex) const
{
	if (!IsValidPlayer(iIndex))
		return nullptr;

	return H::Entities.GetResource()->GetName(iIndex);
}

const char* CChatUtils::GetHighestScoreName() const
{
	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return nullptr;

	int iBestScore = -1;
	int iBestIndex = 0;
	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (!IsValidPlayer(n))
			continue;

		const int iScore = pResource->m_iScore(n);
		if (iScore > iBestScore)
			iBestScore = iScore, iBestIndex = n;
	}

	return iBestIndex ? GetPlayerNameSafe(iBestIndex) : nullptr;
}

const char* CChatUtils::GetRandomPlayerName(int iTag) const
{
	std::vector<int> vCandidates = {};
	const int iLocal = I::EngineClient->GetLocalPlayer();

	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (!IsValidPlayer(n) || n == iLocal)
			continue;

		if (iTag != 0 && !F::PlayerUtils.HasTag(n, F::PlayerUtils.TagToIndex(iTag)))
			continue;

		vCandidates.push_back(n);
	}

	if (vCandidates.empty())
		return nullptr;

	return GetPlayerNameSafe(vCandidates[SDK::RandomInt(0, int(vCandidates.size()) - 1)]);
}

int CChatUtils::GetLocalTeamNum() const
{
	auto pLocal = H::Entities.GetLocal();
	return pLocal ? pLocal->m_iTeamNum() : 0;
}

const char* CChatUtils::GetTeamName(int iTeam) const
{
	switch (iTeam)
	{
	case 2: return "RED";
	case 3: return "BLU";
	default: return nullptr;
	}
}

void CChatUtils::OnVoteStart(bf_read* pMsg)
{
	if (!pMsg)
		return;

	const int iOriginalBit = pMsg->m_iCurBit;

	// VoteStart payload layout (bit 0 = payload start):
	//   byte iTeam, long iVoteID, byte iCaller, string reason, string target, byte iTarget
	pMsg->Seek(0);
	pMsg->ReadByte();  // iTeam
	pMsg->ReadLong();  // iVoteID
	const int iCaller = pMsg->ReadByte();
	pMsg->Seek(iOriginalBit);

	if (iCaller)
	{
		const int iTarget = pMsg->ReadByte() >> 1;
		pMsg->Seek(iOriginalBit);

		m_iVoteCaller = iCaller;
		m_iVoteTarget = iTarget;
		m_flVoteExpire = SDK::PlatFloatTime() + 10.f;
	}
}