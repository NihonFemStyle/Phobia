#include "../SDK/SDK.h"

// we could maybe use tf_datacenter_ping_interval?

static void PopIdName(SteamNetworkingPOPID popID, char* out)
{
	out[0] = static_cast<char>(popID >> 16);
	out[1] = static_cast<char>(popID >> 8);
	out[2] = static_cast<char>(popID);
	out[3] = static_cast<char>(popID >> 24);
	out[4] = 0;
}

static inline int GetDatacenter(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("atl"): return Vars::Misc::Queueing::ForceRegionsEnum::ATL;
	case FNV1A::Hash32Const("ord"): return Vars::Misc::Queueing::ForceRegionsEnum::ORD;
	case FNV1A::Hash32Const("dfw"): return Vars::Misc::Queueing::ForceRegionsEnum::DFW;
	case FNV1A::Hash32Const("lax"): return Vars::Misc::Queueing::ForceRegionsEnum::LAX;
	case FNV1A::Hash32Const("sea"):
	case FNV1A::Hash32Const("eat"): return Vars::Misc::Queueing::ForceRegionsEnum::SEA;
	case FNV1A::Hash32Const("iad"): return Vars::Misc::Queueing::ForceRegionsEnum::IAD;
	case FNV1A::Hash32Const("ams"):
	case FNV1A::Hash32Const("ams4"): return Vars::Misc::Queueing::ForceRegionsEnum::AMS;
	case FNV1A::Hash32Const("fsn"): return Vars::Misc::Queueing::ForceRegionsEnum::FSN;
	case FNV1A::Hash32Const("fra"): return Vars::Misc::Queueing::ForceRegionsEnum::FRA;
	case FNV1A::Hash32Const("hel"): return Vars::Misc::Queueing::ForceRegionsEnum::HEL;
	case FNV1A::Hash32Const("lhr"): return Vars::Misc::Queueing::ForceRegionsEnum::LHR;
	case FNV1A::Hash32Const("mad"): return Vars::Misc::Queueing::ForceRegionsEnum::MAD;
	case FNV1A::Hash32Const("par"): return Vars::Misc::Queueing::ForceRegionsEnum::PAR;
	case FNV1A::Hash32Const("sto"):
	case FNV1A::Hash32Const("sto2"): return Vars::Misc::Queueing::ForceRegionsEnum::STO;
	case FNV1A::Hash32Const("vie"): return Vars::Misc::Queueing::ForceRegionsEnum::VIE;
	case FNV1A::Hash32Const("waw"): return Vars::Misc::Queueing::ForceRegionsEnum::WAW;
	case FNV1A::Hash32Const("eze"): return Vars::Misc::Queueing::ForceRegionsEnum::EZE;
	case FNV1A::Hash32Const("lim"): return Vars::Misc::Queueing::ForceRegionsEnum::LIM;
	case FNV1A::Hash32Const("scl"): return Vars::Misc::Queueing::ForceRegionsEnum::SCL;
	case FNV1A::Hash32Const("gru"): return Vars::Misc::Queueing::ForceRegionsEnum::GRU;
	case FNV1A::Hash32Const("maa2"): return Vars::Misc::Queueing::ForceRegionsEnum::MAA;
	case FNV1A::Hash32Const("dxb"): return Vars::Misc::Queueing::ForceRegionsEnum::DXB;
	case FNV1A::Hash32Const("hkg"): return Vars::Misc::Queueing::ForceRegionsEnum::HKG;
	case FNV1A::Hash32Const("bom2"): return Vars::Misc::Queueing::ForceRegionsEnum::BOM;
	case FNV1A::Hash32Const("seo"): return Vars::Misc::Queueing::ForceRegionsEnum::SEO;
	case FNV1A::Hash32Const("sgp"): return Vars::Misc::Queueing::ForceRegionsEnum::SGP;
	case FNV1A::Hash32Const("tyo"): return Vars::Misc::Queueing::ForceRegionsEnum::TYO;
	case FNV1A::Hash32Const("syd"): return Vars::Misc::Queueing::ForceRegionsEnum::SYD;
	case FNV1A::Hash32Const("jnb"): return Vars::Misc::Queueing::ForceRegionsEnum::JNB;
	}
	return 0;
}

// Returns the block-list bit for a datacenter; "true" in bUS means test the
// FastQueueBlockUS var (Americas/others), false means FastQueueBlockEU.
static inline int GetFastQueueBlock(uint32_t uHash, bool& bUS)
{
	bUS = false;
	switch (uHash)
	{
	case FNV1A::Hash32Const("ams"):
	case FNV1A::Hash32Const("ams4"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Netherlands;
	case FNV1A::Hash32Const("fra"):
	case FNV1A::Hash32Const("fsn"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Germany;
	case FNV1A::Hash32Const("lhr"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::UK;
	case FNV1A::Hash32Const("mad"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Spain;
	case FNV1A::Hash32Const("par"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::France;
	case FNV1A::Hash32Const("lux"):
	case FNV1A::Hash32Const("lux1"):
	case FNV1A::Hash32Const("lux2"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Luxembourg;
	case FNV1A::Hash32Const("sto"):
	case FNV1A::Hash32Const("sto2"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Sweden;
	case FNV1A::Hash32Const("waw"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Poland;
	case FNV1A::Hash32Const("sof"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Bulgaria;
	case FNV1A::Hash32Const("hel"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Finland;
	case FNV1A::Hash32Const("vie"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Austria;
	case FNV1A::Hash32Const("mln1"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Italy;

	case FNV1A::Hash32Const("bom"):
	case FNV1A::Hash32Const("bom2"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Mumbai;
	case FNV1A::Hash32Const("maa"):
	case FNV1A::Hash32Const("maa2"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Chennai;
	case FNV1A::Hash32Const("dxb"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Dubai;
	case FNV1A::Hash32Const("hkg"):
	case FNV1A::Hash32Const("hkg4"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::HongKong;
	case FNV1A::Hash32Const("sha"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Shanghai;
	case FNV1A::Hash32Const("can"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Guangzhou;
	case FNV1A::Hash32Const("tsn"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Tianjin;
	case FNV1A::Hash32Const("tyo"):
	case FNV1A::Hash32Const("tyo1"):
	case FNV1A::Hash32Const("tyo2"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Tokyo;
	case FNV1A::Hash32Const("sgp"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Singapore;
	case FNV1A::Hash32Const("seo"): return Vars::Misc::Queueing::FastQueueBlockEUEnum::Seoul;

	case FNV1A::Hash32Const("dfw"):
	case FNV1A::Hash32Const("dfw2"):
	case FNV1A::Hash32Const("dfwm"):
	case FNV1A::Hash32Const("msa1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Texas;
	case FNV1A::Hash32Const("jfk"):
	case FNV1A::Hash32Const("mny1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::NewYork;
	case FNV1A::Hash32Const("atl"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Atlanta;
	case FNV1A::Hash32Const("iad"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Washington;
	case FNV1A::Hash32Const("ord"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Chicago;
	case FNV1A::Hash32Const("lax"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::California;
	case FNV1A::Hash32Const("sea"):
	case FNV1A::Hash32Const("eat"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Seattle;
	case FNV1A::Hash32Const("okc"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Oklahoma;
	case FNV1A::Hash32Const("msy1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::NewOrleans;
	case FNV1A::Hash32Const("mat1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Virginia;
	case FNV1A::Hash32Const("mmi1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Florida;
	case FNV1A::Hash32Const("mas1"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Boston;

	case FNV1A::Hash32Const("gru"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Brazil;
	case FNV1A::Hash32Const("lim"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Peru;
	case FNV1A::Hash32Const("scl"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Chile;
	case FNV1A::Hash32Const("eze"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Argentina;

	case FNV1A::Hash32Const("jnb"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Africa;
	case FNV1A::Hash32Const("syd"):
		bUS = true; return Vars::Misc::Queueing::FastQueueBlockUSEnum::Australia;
	}
	return 0;
}

MAKE_HOOK(ISteamNetworkingUtils_GetPingToDataCenter, U::Memory.GetVirtual(I::SteamNetworkingUtils, 8), int,
	void* rcx, SteamNetworkingPOPID popID, SteamNetworkingPOPID* pViaRelayPoP)
{
	DEBUG_RETURN(ISteamNetworkingUtils_GetPingToDataCenter, rcx, popID, pViaRelayPoP);

	int iReturn = CALL_ORIGINAL(rcx, popID, pViaRelayPoP);
	if (iReturn < 0)
		return iReturn;

	// Fast queue takes priority: block listed datacenters with an absurd ping and
	// report a tiny fake ping for any low-ping datacenter to bias the matchmaker.
	if (Vars::Misc::Queueing::FastQueue.Value)
	{
		char sPopID[5];
		PopIdName(popID, sPopID);

		bool bUS = false;
		if (auto iBlock = GetFastQueueBlock(FNV1A::Hash32(sPopID), bUS))
		{
			const int iBlocked = bUS
				? Vars::Misc::Queueing::FastQueueBlockUS.Value
				: Vars::Misc::Queueing::FastQueueBlockEU.Value;
			if (iBlocked & iBlock)
				return SDK::RandomInt(700, 1250);
		}

		if (iReturn <= Vars::Misc::Queueing::FastQueueMaxPing.Value)
			return std::min(iReturn, SDK::RandomInt(5, 20));
		return iReturn;
	}

	if (!Vars::Misc::Queueing::ForceRegions.Value)
		return iReturn;

	char sPopID[5];
	PopIdName(popID, sPopID);
	if (auto uDatacenter = GetDatacenter(FNV1A::Hash32(sPopID)))
		return Vars::Misc::Queueing::ForceRegions.Value & uDatacenter ? 1 : 1000;

	return iReturn;
}

MAKE_HOOK(CTFPartyClient_RequestQueueForMatch, S::CTFPartyClient_RequestQueueForMatch(), void,
	void* rcx, int eMatchGroup)
{
	DEBUG_RETURN(CTFPartyClient_RequestQueueForMatch, rcx, eMatchGroup);

	I::TFGCClientSystem->SetPendingPingRefresh(true);
	I::TFGCClientSystem->PingThink();

	CALL_ORIGINAL(rcx, eMatchGroup);
}