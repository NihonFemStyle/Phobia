#pragma once

#include "../../SDK/SDK.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define SKIN_NONE 0
#define SKIN_AUSTRALIUM -1
#define SKIN_FULL_UPDATE_COOLDOWN 0.25f

namespace attributes
{
	constexpr uint16_t paintkit_proto_def_index = 834;
	constexpr uint16_t custom_paintkit_seed_lo = 866;
	constexpr uint16_t custom_paintkit_seed_hi = 867;
	constexpr uint16_t set_item_texture_wear = 725;
	constexpr uint16_t set_attached_particle = 134;
	constexpr uint16_t is_festivized = 2053;
	constexpr uint16_t is_australium_item = 2027;
	constexpr uint16_t killstreak_tier = 2025;
	constexpr uint16_t killstreak_effect = 2013;
	constexpr uint16_t killstreak_idleeffect = 2014;
}

__forceinline float IntToStupidFloat(int nValue)
{
	return *reinterpret_cast<float*>(&nValue);
}

struct SkinConfig
{
	int iVariant = SKIN_NONE; // stock (0), australium (SKIN_AUSTRALIUM), or curated skin def index
	int iPaintKit = 0;        // custom paint kit (0 = none)
	float flWear = 0.f;       // wear (0 = factory new)
	int iSeed = 0;            // custom seed
	int iKillstreakTier = 0;  // 1 = killstreak, 2 = specialized, 3 = professional
	int iSheen = 0;           // 1..6 for specialized+
	int iIdleEffect = 0;      // 2005..2008 for professional
	int iParticle = 0;        // unusual weapon effect (701..704, 0 = none)
	bool bFestivized = false;
	bool bAustralium = false;

	bool HasAny() const
	{
		return iVariant || iPaintKit || iKillstreakTier || iSheen || iIdleEffect || iParticle || bFestivized || bAustralium;
	}

	bool operator!=(const SkinConfig& t) const
	{
		return iVariant != t.iVariant || iPaintKit != t.iPaintKit || flWear != t.flWear
			|| iSeed != t.iSeed || iKillstreakTier != t.iKillstreakTier || iSheen != t.iSheen
			|| iIdleEffect != t.iIdleEffect || iParticle != t.iParticle
			|| bFestivized != t.bFestivized || bAustralium != t.bAustralium;
	}
};

struct CuratedSkin_t
{
	int iDefIndex = 0;
	int iBaseDefIndex = 0; // stock weapon def this skin UI-equips onto
	const char* sName = nullptr;
};

class CSkinChanger
{
public:
	void Run();

	SkinConfig GlobalFromVars() const;

	void SetOverride(int iDefIndex);
	void RemoveOverride(int iDefIndex);
	bool HasOverride(int iDefIndex) const { return m_Overrides.contains(iDefIndex); }

	int GetActiveWeaponDefIndex() const;

	void Save();
	void Load();

	static const std::vector<CuratedSkin_t>& CuratedSkins();
	static int GetWeaponGroup(int iDefIndex);
	static std::vector<std::pair<int, const char*>> GetSkinOptions(int iWeaponDefIndex);

	std::unordered_map<int, SkinConfig> m_Overrides = {};
	bool m_bLoaded = false;

private:
	void ApplyWeapon(CBaseCombatWeapon* pWeapon);
	void ApplyConfig(CBaseCombatWeapon* pWeapon, const SkinConfig& config);

	SkinConfig m_GlobalConfig = {};
	bool m_bNeedFullUpdate = false;
	float m_flLastFullUpdate = 0.f;
};

ADD_FEATURE(CSkinChanger, SkinChanger);