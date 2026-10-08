#pragma once
#include "../../../SDK/SDK.h"

class CAutoVote
{
public:
	void Run();
	void UserMessage(UserMessageType type, bf_read& msgData);

private:
	void HandleVoteStart(bf_read& msgData);
	int ResolveVotekickVictim(const char* sName);

	std::unordered_map<uint32_t, int> m_mVoteAttempts = {};
	float m_flNextVoteTime = 0.f;
	bool m_bVoteInProgress = false;
};

ADD_FEATURE(CAutoVote, AutoVote);