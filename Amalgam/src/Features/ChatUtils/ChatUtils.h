#pragma once
#include "../../SDK/SDK.h"

class CChatUtils
{
public:
	void Run();
	void Event(IGameEvent* pEvent, uint32_t uHash, CTFPlayer* pLocal);
	void OnChatMessage(int clientIndex, const std::string& sMessage);
	void OnVoteStart(bf_read* pMsg);

private:
	struct AutoReplyRule_t
	{
		std::vector<std::string> m_vTriggers = {};
		std::vector<std::string> m_vReplies = {};
	};

	void LoadFiles();
	void KillSay(int iVictim);
	void AutoReply(int clientIndex, const std::string& sMessage);
	void SendChat(const std::string& sMessage);
	std::string Resolve(const std::string& sTemplate, const char* sKiller = nullptr, const char* sVictim = nullptr, const char* sTarget = nullptr, const char* sInitiator = nullptr) const;

	bool IsValidPlayer(int iIndex) const;
	const char* GetPlayerNameSafe(int iIndex) const;
	const char* GetHighestScoreName() const;
	const char* GetRandomPlayerName(int iTag = 0) const;
	int GetLocalTeamNum() const;
	const char* GetTeamName(int iTeam) const;

	std::vector<std::string> m_vKillSay = {};
	std::vector<AutoReplyRule_t> m_vAutoReply = {};

	std::string m_sLastKilled = "";
	int m_iVoteCaller = 0;
	int m_iVoteTarget = 0;
	float m_flVoteExpire = 0.f;
	float m_flNextReload = 0.f;
};

ADD_FEATURE(CChatUtils, ChatUtils);