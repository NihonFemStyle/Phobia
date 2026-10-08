#include "../SDK/SDK.h"

#include "../Features/Players/PlayerUtils.h"

MAKE_SIGNATURE(GetPlayerNameForSteamID_GetFriendPersonaName_Call, "client.dll", "41 B9 ? ? ? ? 44 8B C3 48 8B C8", 0x0);

MAKE_HOOK(ISteamFriends_GetFriendPersonaName, U::Memory.GetVirtual(I::SteamFriends, 7), const char*,
	void* rcx, CSteamID steamIDFriend)
{
	DEBUG_RETURN(ISteamFriends_GetFriendPersonaName, rcx, steamIDFriend);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::GetPlayerNameForSteamID_GetFriendPersonaName_Call();

	if (dwRetAddr == dwDesired && Vars::Visuals::UI::StreamerMode.Value)
	{
		const int iType = F::PlayerUtils.GetNameType(steamIDFriend.GetAccountID());
		if (iType & NameTypeEnum::Privacy)
			return F::PlayerUtils.GetPlayerName(steamIDFriend.GetAccountID(), "Player");
	}

	return CALL_ORIGINAL(rcx, steamIDFriend);
}