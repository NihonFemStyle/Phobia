#include "SkinChanger.h"

#include <filesystem>
#include <fstream>
#include "../Configs/Configs.h"
#include "../../Utils/nlohmann/json.hpp"

MAKE_SIGNATURE(GetItemSchema, "client.dll", "48 83 EC ? E8 ? ? ? ? 48 83 C0 ? 48 83 C4 ? C3 CC CC CC", 0x0);
MAKE_SIGNATURE(CEconItemSchema_GetAttributeDefinition, "client.dll", "89 54 24 ? 53 48 83 EC ? 48 8B D9 48 8D 54 24 ? 48 81 C1 ? ? ? ? E8 ? ? ? ? 8B D0 3B 83 ? ? ? ? 73 ? 8B 83 ? ? ? ? 83 F8 ? 74 ? 3B D0 7F ? 48 81 C3 ? ? ? ? 44 8B C2 83 FA ? 74 ? 48 8B 03 8B CA", 0x0);
MAKE_SIGNATURE(CAttributeList_SetRuntimeAttributeValue, "client.dll", "48 89 5C 24 10 55 56 57 48 8B EC 48 83 EC 50 44", 0x0);

class CEconItemAttribute
{
public:
	void* pad = nullptr;
	unsigned int m_iAttributeDefinitionIndex = 0;
	union { int m_iRawValue32 = 0; float m_flValue; };
	int m_nRefundableCurrency = 0;
};

class CAttributeList
{
public:
	void* pad = nullptr;
	CUtlVector<CEconItemAttribute> m_Attributes;
	void* m_pManager = nullptr;

	void SetAttribute(int iIndex, float flValue)
	{
		auto schema = S::GetItemSchema.Call<void*>();
		if (!schema)
			return;

		auto attributeDefinition = S::CEconItemSchema_GetAttributeDefinition.Call<void*>(schema, iIndex);
		if (!attributeDefinition)
			return;

		S::CAttributeList_SetRuntimeAttributeValue.Call<void>(this, attributeDefinition, flValue);
	}
};

// Fixed offset (x64, +0xDB8) from the weapon entity to its CAttributeList
static constexpr uintptr_t WEAPON_ATTRIBUTE_LIST_OFFSET = 3512;

struct Redirect_t
{
	int from;
	int to;
};

static constexpr Redirect_t s_RedirectTable[] =
{
	{ 18, 205 },  // Soldier RocketLauncher -> RocketLauncherR
	{ 13, 200 },  // Scout Scattergun -> ScattergunR
	{ 21, 208 },  // Pyro FlameThrower -> FlameThrowerR
	{ 19, 206 },  // Demoman GrenadeLauncher -> GrenadeLauncherR
	{ 20, 207 },  // Demoman StickybombLauncher -> StickybombLauncherR
	{ 15, 202 },  // Heavy Minigun -> MinigunR
	{ 7, 197 },   // Engineer Wrench -> WrenchR
	{ 29, 211 },  // Medic MediGun -> MediGunR
	{ 14, 201 },  // Sniper SniperRifle -> SniperRifleR
	{ 16, 203 },  // Sniper SMG -> SMGR
	{ 4, 194 },   // Spy Knife -> KnifeR
	{ 24, 210 },  // Spy Revolver -> RevolverR
	{ 22, 209 },  // Engineer Pistol -> PistolR
	{ 9, 199 },   // Engineer Shotgun -> ShotgunR
	{ 10, 199 },  // Soldier Shotgun -> ShotgunR
	{ 11, 199 },  // Heavy Shotgun -> ShotgunR
	{ 12, 199 },  // Pyro Shotgun -> ShotgunR
	{ 0, 190 },   // Scout Bat -> BatR
	{ 6, 196 },   // Soldier Shovel -> ShovelR
	{ 2, 192 },   // Pyro FireAxe -> FireAxeR
	{ 1, 191 },   // Demoman Bottle -> BottleR
	{ 8, 198 },   // Medic Bonesaw -> BonesawR
	{ 3, 193 },   // Sniper Kukri -> KukriR
};

static int RedirectIndex(int nDefIndex)
{
	for (const auto& redirect : s_RedirectTable)
	{
		if (redirect.from == nDefIndex)
			return redirect.to;
	}
	return nDefIndex;
}

static std::string GetSkinFile()
{
	return F::Configs.m_sCorePath + "skins.json";
}

void CSkinChanger::Run()
{
	if (G::Unload)
		return;

	if (!m_bLoaded)
	{
		Load();
		m_bLoaded = true;
		m_bNeedFullUpdate = true;
	}

	if (!Vars::SkinChanger::Enabled.Value)
		return;

	const SkinConfig config = GlobalFromVars();
	if (config != m_GlobalConfig)
	{
		m_GlobalConfig = config;
		m_bNeedFullUpdate = true;
	}

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return;

	// keep the global variant tied to the active weapon: if the player swapped to a
	// weapon the selected skin can't apply to, drop back to "None (stock)"
	const int iActiveDef = GetActiveWeaponDefIndex();
	if (Vars::SkinChanger::iVariant.Value > SKIN_NONE && iActiveDef > 0
		&& GetWeaponGroup(Vars::SkinChanger::iVariant.Value) != GetWeaponGroup(iActiveDef))
	{
		Vars::SkinChanger::iVariant.Value = SKIN_NONE;
	}

	auto& aMyWeapons = pLocal->m_hMyWeapons();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pWeapon = aMyWeapons[i].Get();
		if (!pWeapon)
			continue;

		ApplyWeapon(pWeapon);
	}

	// force a network + render refresh so australium/skin changes show up on redeploy
	if (m_bNeedFullUpdate && I::GlobalVars && I::GlobalVars->curtime - m_flLastFullUpdate > SKIN_FULL_UPDATE_COOLDOWN)
	{
		m_bNeedFullUpdate = false;
		m_flLastFullUpdate = I::GlobalVars->curtime;
		if (I::ClientState)
			I::ClientState->ForceFullUpdate();
	}
}

void CSkinChanger::ApplyWeapon(CBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
		return;

	auto& nDefIndex = pWeapon->m_iItemDefinitionIndex();

	const auto itOverride = m_Overrides.find(nDefIndex);
	if (itOverride != m_Overrides.end())
	{
		if (itOverride->second.HasAny())
			ApplyConfig(pWeapon, itOverride->second);
		return;
	}

	SkinConfig config = GlobalFromVars();
	if (config.HasAny())
		ApplyConfig(pWeapon, config);
}

void CSkinChanger::ApplyConfig(CBaseCombatWeapon* pWeapon, const SkinConfig& config)
{
	auto& nDefIndex = pWeapon->m_iItemDefinitionIndex();

	if (config.iVariant == SKIN_AUSTRALIUM)
	{
		// australium: keep the weapon identity (redirected so a bodygroup can be golded),
		// the material comes from the is_australium_item attribute below
		nDefIndex = RedirectIndex(nDefIndex);
	}
	else if (config.iVariant > SKIN_NONE)
		nDefIndex = config.iVariant;
	else
		nDefIndex = RedirectIndex(nDefIndex);

	auto attributeList = reinterpret_cast<CAttributeList*>(reinterpret_cast<uintptr_t>(pWeapon) + WEAPON_ATTRIBUTE_LIST_OFFSET);
	if (!attributeList)
		return;

	if (config.iVariant == SKIN_AUSTRALIUM || config.iVariant > SKIN_NONE)
	{
		// exact skin items carry their own paint: clear any custom paint kit
		attributeList->SetAttribute(attributes::paintkit_proto_def_index, IntToStupidFloat(0));
	}
	else if (config.iPaintKit > 0)
	{
		attributeList->SetAttribute(attributes::paintkit_proto_def_index, IntToStupidFloat(config.iPaintKit));
		attributeList->SetAttribute(attributes::custom_paintkit_seed_lo, static_cast<float>(config.iSeed & 0xFFFF));
		attributeList->SetAttribute(attributes::custom_paintkit_seed_hi, static_cast<float>((config.iSeed >> 16) & 0xFFFF));
		attributeList->SetAttribute(attributes::set_item_texture_wear, config.flWear);
	}

	attributeList->SetAttribute(attributes::killstreak_tier, static_cast<float>(config.iKillstreakTier));
	attributeList->SetAttribute(attributes::killstreak_effect, config.iKillstreakTier >= 2 ? static_cast<float>(config.iSheen) : 0.f);
	attributeList->SetAttribute(attributes::killstreak_idleeffect, config.iKillstreakTier >= 3 ? static_cast<float>(config.iIdleEffect) : 0.f);
	attributeList->SetAttribute(attributes::set_attached_particle, static_cast<float>(config.iParticle));
	attributeList->SetAttribute(attributes::is_festivized, config.bFestivized ? 1.f : 0.f);
	const bool bGold = config.bAustralium || config.iVariant == SKIN_AUSTRALIUM;
	attributeList->SetAttribute(attributes::is_australium_item, bGold ? 1.f : 0.f);
}

SkinConfig CSkinChanger::GlobalFromVars() const
{
	SkinConfig config = {};
	config.iVariant = Vars::SkinChanger::iVariant.Value;
	config.iPaintKit = Vars::SkinChanger::iPaintKit.Value;
	config.flWear = Vars::SkinChanger::flWear.Value;
	config.iSeed = Vars::SkinChanger::iSeed.Value;
	config.iKillstreakTier = Vars::SkinChanger::iKillstreakTier.Value;
	config.iSheen = Vars::SkinChanger::iSheen.Value;
	config.iIdleEffect = Vars::SkinChanger::iIdleEffect.Value;
	config.iParticle = Vars::SkinChanger::iParticle.Value;
	config.bFestivized = Vars::SkinChanger::bFestivized.Value;
	config.bAustralium = Vars::SkinChanger::bAustralium.Value;
	return config;
}

void CSkinChanger::SetOverride(int iDefIndex)
{
	if (iDefIndex <= 0)
		return;

	m_Overrides[iDefIndex] = GlobalFromVars();
	m_bNeedFullUpdate = true;
	Save();
}

void CSkinChanger::RemoveOverride(int iDefIndex)
{
	m_Overrides.erase(iDefIndex);
	m_bNeedFullUpdate = true;
	Save();
}

int CSkinChanger::GetActiveWeaponDefIndex() const
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return -1;

	auto pWeapon = pLocal->m_hActiveWeapon()->As<CTFWeaponBase>();
	if (!pWeapon)
		return -1;

	return pWeapon->m_iItemDefinitionIndex();
}

void CSkinChanger::Save()
{
	if (!std::filesystem::exists(F::Configs.m_sCorePath))
		std::filesystem::create_directory(F::Configs.m_sCorePath);

	nlohmann::json jGlobal;
	jGlobal["variant"] = Vars::SkinChanger::iVariant.Value;
	jGlobal["paintkit"] = Vars::SkinChanger::iPaintKit.Value;
	jGlobal["wear"] = Vars::SkinChanger::flWear.Value;
	jGlobal["seed"] = Vars::SkinChanger::iSeed.Value;
	jGlobal["killstreak"] = Vars::SkinChanger::iKillstreakTier.Value;
	jGlobal["sheen"] = Vars::SkinChanger::iSheen.Value;
	jGlobal["idle"] = Vars::SkinChanger::iIdleEffect.Value;
	jGlobal["particle"] = Vars::SkinChanger::iParticle.Value;
	jGlobal["festivized"] = Vars::SkinChanger::bFestivized.Value;
	jGlobal["australium"] = Vars::SkinChanger::bAustralium.Value;

	nlohmann::json jOverrides = nlohmann::json::object();
	for (const auto& [iDef, config] : m_Overrides)
	{
		nlohmann::json jEntry;
		jEntry["variant"] = config.iVariant;
		jEntry["paintkit"] = config.iPaintKit;
		jEntry["wear"] = config.flWear;
		jEntry["seed"] = config.iSeed;
		jEntry["killstreak"] = config.iKillstreakTier;
		jEntry["sheen"] = config.iSheen;
		jEntry["idle"] = config.iIdleEffect;
		jEntry["particle"] = config.iParticle;
		jEntry["festivized"] = config.bFestivized;
		jEntry["australium"] = config.bAustralium;
		jOverrides[std::to_string(iDef)] = jEntry;
	}

	nlohmann::json j;
	j["global"] = jGlobal;
	j["overrides"] = jOverrides;

	std::ofstream file(GetSkinFile(), std::ios::trunc);
	if (file.good())
		file << j.dump(4);
}

void CSkinChanger::Load()
{
	std::ifstream file(GetSkinFile());
	if (!file.good())
		return;

	try
	{
		nlohmann::json j = nlohmann::json::parse(file);

		if (j.contains("global"))
		{
			auto& g = j["global"];
			Vars::SkinChanger::iVariant.Value = g.value("variant", Vars::SkinChanger::iVariant.Value);
			Vars::SkinChanger::iPaintKit.Value = g.value("paintkit", Vars::SkinChanger::iPaintKit.Value);
			Vars::SkinChanger::flWear.Value = g.value("wear", Vars::SkinChanger::flWear.Value);
			Vars::SkinChanger::iSeed.Value = g.value("seed", Vars::SkinChanger::iSeed.Value);
			Vars::SkinChanger::iKillstreakTier.Value = g.value("killstreak", Vars::SkinChanger::iKillstreakTier.Value);
			Vars::SkinChanger::iSheen.Value = g.value("sheen", Vars::SkinChanger::iSheen.Value);
			Vars::SkinChanger::iIdleEffect.Value = g.value("idle", Vars::SkinChanger::iIdleEffect.Value);
			Vars::SkinChanger::iParticle.Value = g.value("particle", Vars::SkinChanger::iParticle.Value);
			Vars::SkinChanger::bFestivized.Value = g.value("festivized", Vars::SkinChanger::bFestivized.Value);
			Vars::SkinChanger::bAustralium.Value = g.value("australium", Vars::SkinChanger::bAustralium.Value);
		}

		if (j.contains("overrides"))
		{
			m_Overrides.clear();
			for (auto it = j["overrides"].begin(); it != j["overrides"].end(); ++it)
			{
				int iDef = std::stoi(it.key());
				auto& e = it.value();

				SkinConfig config = {};
				config.iVariant = e.value("variant", 0);
				config.iPaintKit = e.value("paintkit", 0);
				config.flWear = e.value("wear", 0.f);
				config.iSeed = e.value("seed", 0);
				config.iKillstreakTier = e.value("killstreak", 0);
				config.iSheen = e.value("sheen", 0);
				config.iIdleEffect = e.value("idle", 0);
				config.iParticle = e.value("particle", 0);
				config.bFestivized = e.value("festivized", false);
				config.bAustralium = e.value("australium", false);

				m_Overrides[iDef] = config;
			}
		}
	}
	catch (...)
	{
		return;
	}
}

const std::vector<CuratedSkin_t>& CSkinChanger::CuratedSkins()
{
	static const std::vector<CuratedSkin_t> s_Skins =
	{
		{ 15002, 13, "Night Terror (Scattergun)" },
		{ 15015, 13, "Tartan Torpedo (Scattergun)" },
		{ 15021, 13, "Country Crusher (Scattergun)" },
		{ 15029, 13, "Backcountry Blaster (Scattergun)" },
		{ 15036, 13, "Spruce Deuce (Scattergun)" },
		{ 15053, 13, "Current Event (Scattergun)" },
		{ 15065, 13, "Macabre Web (Scattergun)" },
		{ 15069, 13, "Nutcracker (Scattergun)" },
		{ 15106, 13, "Blue Mew (Scattergun)" },
		{ 15107, 13, "Flower Power (Scattergun)" },
		{ 15108, 13, "Shotto Hell (Scattergun)" },
		{ 15131, 13, "Coffin Nail (Scattergun)" },
		{ 15151, 13, "Killer Bee (Scattergun)" },
		{ 15157, 13, "Corsair (Scattergun)" },

		{ 15006, 18, "Woodland Warrior (RL)" },
		{ 15014, 18, "Sand Cannon (RL)" },
		{ 15028, 18, "American Pastoral (RL)" },
		{ 15043, 18, "Smalltown Bringdown (RL)" },
		{ 15052, 18, "Shell Shocker (RL)" },
		{ 15057, 18, "Aqua Marine (RL)" },
		{ 15081, 18, "Autumn (RL)" },
		{ 15150, 18, "Warhawk (RL)" },

		{ 15005, 21, "Forest Fire (Flamethrower)" },
		{ 15017, 21, "Barn Burner (Flamethrower)" },
		{ 15030, 21, "Bovine Blazemaker (Flamethrower)" },
		{ 15034, 21, "Earth, Sky and Fire (Flamethrower)" },
		{ 15049, 21, "Flash Fryer (Flamethrower)" },
		{ 15054, 21, "Turbine Torcher (Flamethrower)" },
		{ 15066, 21, "Autumn (Flamethrower)" },
		{ 15067, 21, "Pumpkin Patch (Flamethrower)" },
		{ 15068, 21, "Nutcracker (Flamethrower)" },
		{ 15089, 21, "Balloonicorn (Flamethrower)" },
		{ 15090, 21, "Rainbow (Flamethrower)" },

		{ 15077, 19, "Autumn (GL)" },
		{ 15079, 19, "Macabre Web (GL)" },
		{ 15091, 19, "Rainbow (GL)" },
		{ 15092, 19, "Sweet Dreams (GL)" },

		{ 15009, 20, "Sudden Flurry (Stickybomb)" },
		{ 15012, 20, "Carpet Bomber (Stickybomb)" },
		{ 15024, 20, "Blasted Bombardier (Stickybomb)" },
		{ 15038, 20, "Rooftop Wrangler (Stickybomb)" },
		{ 15045, 20, "Liquid Asset (Stickybomb)" },
		{ 15048, 20, "Pink Elephant (Stickybomb)" },

		{ 15004, 15, "King of the Jungle (Minigun)" },
		{ 15020, 15, "Iron Wood (Minigun)" },
		{ 15026, 15, "Antique Annihilator (Minigun)" },
		{ 15031, 15, "War Room (Minigun)" },
		{ 15040, 15, "Citizen Pain (Minigun)" },
		{ 15055, 15, "Brick House (Minigun)" },

		{ 15008, 29, "Masked Mender (Medigun)" },
		{ 15010, 29, "Wrapped Reviver (Medigun)" },
		{ 15025, 29, "Reclaimed Reanimator (Medigun)" },
		{ 15039, 29, "Civil Servant (Medigun)" },
		{ 15050, 29, "Spark of Life (Medigun)" },
		{ 15078, 29, "Wildwood (Medigun)" },

		{ 15000, 14, "Night Owl (Sniper Rifle)" },
		{ 15007, 14, "Purple Range (Sniper Rifle)" },
		{ 15019, 14, "Lumber From Down Under (Sniper Rifle)" },
		{ 15023, 14, "Shot in the Dark (Sniper Rifle)" },
		{ 15033, 14, "Bogtrotter (Sniper Rifle)" },
		{ 15059, 14, "Thunderbolt (Sniper Rifle)" },
		{ 15071, 14, "Boneyard (Sniper Rifle)" },
		{ 15072, 14, "Wildwood (Sniper Rifle)" },
		{ 15111, 14, "Balloonicorn (Sniper Rifle)" },
		{ 15112, 14, "Rainbow (Sniper Rifle)" },

		{ 15011, 24, "Psychedelic Slugger (Revolver)" },
		{ 15027, 24, "Old Country (Revolver)" },
		{ 15042, 24, "Mayor (Revolver)" },
		{ 15051, 24, "Dead Reckoner (Revolver)" },
		{ 15062, 24, "Boneyard (Revolver)" },
		{ 15063, 24, "Wildwood (Revolver)" },
		{ 15103, 24, "Flower Power (Revolver)" },

		{ 15073, 7, "Nutcracker (Wrench)" },
		{ 15074, 7, "Autumn (Wrench)" },
		{ 15075, 7, "Boneyard (Wrench)" },

		{ 15094, 4, "Blue Mew (Knife)" },
		{ 15095, 4, "Brain Candy (Knife)" },
		{ 15096, 4, "Stabbed to Hell (Knife)" },
		{ 15118, 4, "Dressed to Kill (Knife)" },
		{ 15119, 4, "Top Shelf (Knife)" },
	};
	return s_Skins;
}

int CSkinChanger::GetWeaponGroup(int iDefIndex)
{
	switch (iDefIndex)
	{
		case 13: case 200: return 13;  // Scattergun / ScattergunR
		case 22: case 23: case 209: return 22; // Pistol (scout/engi) / PistolR
		case 0: case 190: return 0;   // Bat / BatR
		case 18: case 205: return 18; // Rocket Launcher / RocketLauncherR
		case 9: case 10: case 11: case 12: case 199: return 9; // Shotgun (engi/soldier/heavy/pyro) / ShotgunR
		case 21: case 208: return 21; // Flame Thrower / FlameThrowerR
		case 19: case 206: return 19; // Grenade Launcher / GrenadeLauncherR
		case 20: case 207: return 20; // Stickybomb Launcher / StickybombLauncherR
		case 15: case 202: return 15; // Minigun / MinigunR
		case 7: case 197: return 7;   // Wrench / WrenchR
		case 29: case 211: return 29; // Medi Gun / MediGunR
		case 14: case 201: return 14; // Sniper Rifle / SniperRifleR
		case 16: case 203: return 16; // SMG / SMGR
		case 4: case 194: return 4;   // Knife / KnifeR
		case 24: case 210: return 24; // Revolver / RevolverR
		default: return iDefIndex;
	}
}

std::vector<std::pair<int, const char*>> CSkinChanger::GetSkinOptions(int iWeaponDefIndex)
{
	std::vector<std::pair<int, const char*>> vOptions;
	vOptions.push_back({ SKIN_AUSTRALIUM, "Australia" });
	vOptions.push_back({ SKIN_NONE, "None (stock)" });

	const int iGroup = GetWeaponGroup(iWeaponDefIndex);
	for (const auto& skin : CuratedSkins())
	{
		if (GetWeaponGroup(skin.iBaseDefIndex) == iGroup)
			vOptions.push_back({ skin.iDefIndex, skin.sName });
	}
	return vOptions;
}