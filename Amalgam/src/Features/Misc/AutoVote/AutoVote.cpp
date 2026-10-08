#include "AutoVote.h"

#include "../../Players/PlayerUtils.h"
#include "../../Players/Bans.h"

static bool IsBannedPlayer(uint32_t uAccountID)
{
	auto oBan = F::SteamBans.GetResult(uAccountID);
	if (!oBan)
		return false;

	return oBan->m_bVACBanned || oBan->m_bGameBanned || oBan->m_bCommunityBanned || oBan->m_bHasActiveSourceBan;
}

void CAutoVote::Run()
{
	if (G::Unload || !Vars::Misc::Automation::AutoVote.Value
		|| !Vars::Misc::Automation::AutoVoteDefensive.Value && Vars::Misc::Automation::AutoVoteTargets.Value == 0
		|| !I::EngineClient->IsConnected() || I::EngineClient->IsPlayingDemo())
		return;

	auto pLocal = H::Entities.GetLocal();
	auto pResource = H::Entities.GetResource();
	if (!pLocal || !pResource || pLocal->m_iTeamNum() <= 1)
		return;

	const int iLocal = I::EngineClient->GetLocalPlayer();
	const float flTime = SDK::PlatFloatTime();
	if (m_flNextVoteTime > flTime || m_bVoteInProgress)
		return;

	int iVictim = -1;
	int iTargetPriority = 0;
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerTeam))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer)
			continue;

		const int iIndex = pPlayer->entindex();
		if (iIndex == iLocal || H::Entities.IsFriend(iIndex) || H::Entities.InParty(iIndex))
			continue;

		const uint32_t uAccountID = pResource->m_iAccountID(iIndex);
		if (!uAccountID)
			continue;

		int iPriority = 0;
		if (Vars::Misc::Automation::AutoVoteTargets.Value & Vars::Misc::Automation::AutoVoteTargetsEnum::Prioritized
			&& F::PlayerUtils.IsPrioritized(iIndex))
			iPriority += 1;
		if (Vars::Misc::Automation::AutoVoteTargets.Value & Vars::Misc::Automation::AutoVoteTargetsEnum::Cheaters
			&& (F::PlayerUtils.HasTag(iIndex, F::PlayerUtils.TagToIndex(CHEATER_TAG)) || IsBannedPlayer(uAccountID)))
			iPriority += 2;
		if (Vars::Misc::Automation::AutoVoteTargets.Value & Vars::Misc::Automation::AutoVoteTargetsEnum::Bots
			&& pResource->IsFakePlayer(iIndex, true))
			iPriority += 1;
		if (Vars::Misc::Automation::AutoVoteDefensive.Value
			&& m_mVoteAttempts.find(uAccountID) != m_mVoteAttempts.end())
			iPriority += m_mVoteAttempts[uAccountID];

		if (iPriority > iTargetPriority)
		{
			iTargetPriority = iPriority;
			iVictim = iIndex;
		}
	}

	if (iVictim == -1)
		return;

	player_info_t tInfo;
	if (!I::EngineClient->GetPlayerInfo(iVictim, &tInfo))
		return;

	I::ClientState->SendStringCmd(std::format("callvote kick \"{} cheating\"", tInfo.userID).c_str());
	m_flNextVoteTime = flTime + 1.f;
	m_bVoteInProgress = true;
}

int CAutoVote::ResolveVotekickVictim(const char* sName)
{
	if (!sName || !sName[0])
		return 0;

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return 0;

	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		const int iIndex = pEntity->entindex();
		if (_stricmp(pResource->GetName(iIndex), sName) == 0)
			return iIndex;
	}

	return 0;
}

void CAutoVote::HandleVoteStart(bf_read& msgData)
{
	const int iTeam = msgData.ReadByte();
	const int iVoteID = msgData.ReadLong();
	const int iCaller = msgData.ReadByte();
	char sReason[256]; msgData.ReadString(sReason, sizeof(sReason));
	char sTarget[256]; msgData.ReadString(sTarget, sizeof(sTarget));
	const int iTarget = msgData.ReadByte() >> 1;
	msgData.Seek(0);

	auto pLocal = H::Entities.GetLocal();
	auto pResource = H::Entities.GetResource();
	if (!pLocal || !pResource)
		return;

	const int iLocal = I::EngineClient->GetLocalPlayer();
	const float flTime = SDK::PlatFloatTime();

	if (iCaller == iLocal)
	{	// we called this vote; don't auto-cast a new one too soon
		m_flNextVoteTime = flTime + 10.f;
		m_bVoteInProgress = true;
		return;
	}

	// this vote doesn't matter to us
	if (iTeam == 0 || iTeam != pLocal->m_iTeamNum())
	{
		m_bVoteInProgress = true;
		return;
	}

	const int iVictim = ResolveVotekickVictim(sTarget);
	const int iActor = iVictim ? iVictim : iCaller;

	// track people trying to kick us or our friends (defensive)
	if (iVictim && (iVictim == iLocal || H::Entities.IsFriend(iVictim)) && iCaller != iLocal)
	{
		const uint32_t uCasterAccountID = pResource->m_iAccountID(iCaller);
		if (uCasterAccountID)
			m_mVoteAttempts[uCasterAccountID]++;
	}

	if (Vars::Misc::Automation::AutoF2Ignored.Value
		&& (F::PlayerUtils.IsIgnored(iActor)
		|| H::Entities.IsFriend(iActor)
		|| H::Entities.InParty(iActor)
		|| iActor == iLocal))
	{
		I::ClientState->SendStringCmd(std::format("vote {} option2", iVoteID).c_str());
	}
	else if (Vars::Misc::Automation::AutoF1Priority.Value
		&& (F::PlayerUtils.IsPrioritized(iActor)
		|| F::PlayerUtils.HasTag(iActor, F::PlayerUtils.TagToIndex(CHEATER_TAG)))
		&& !H::Entities.IsFriend(iActor)
		&& !H::Entities.InParty(iActor)
		&& iActor != iLocal)
	{
		I::ClientState->SendStringCmd(std::format("vote {} option1", iVoteID).c_str());
	}
}

void CAutoVote::UserMessage(UserMessageType type, bf_read& msgData)
{
	switch (type)
	{
	case VoteStart:
		HandleVoteStart(msgData);
		break;
	case VoteFailed:
	case VotePass:
		m_bVoteInProgress = false;
		break;
	case CallVoteFailed:
	{
		const int iReason = msgData.ReadByte();
		const float flTimeout = (float)msgData.ReadByte();
		msgData.Seek(0);

		m_flNextVoteTime = SDK::PlatFloatTime() + flTimeout;
		m_bVoteInProgress = false;
		break;
	}
	}
}