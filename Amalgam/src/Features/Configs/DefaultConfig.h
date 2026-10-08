#pragma once

inline constexpr const char* g_pDefaultConfigJson =
    R"({
    "Binds": {
        "0": {
            "Name": "Aimbot",
            "Type": "0",
            "Info": "0",
            "Key": "6",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "1": {
            "Name": "Critical Shit",
            "Type": "0",
            "Info": "0",
            "Key": "81",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "2": {
            "Name": "Blood Pressure",
            "Type": "0",
            "Info": "2",
            "Key": "18",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "3": {
            "Name": "The Jackson",
            "Type": "0",
            "Info": "0",
            "Key": "69",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "4": {
            "Name": "Insulin",
            "Type": "0",
            "Info": "0",
            "Key": "82",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "5": {
            "Name": "Crim Walk",
            "Type": "0",
            "Info": "1",
            "Key": "220",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "6": {
            "Name": "Shoulder Surf",
            "Type": "0",
            "Info": "1",
            "Key": "4",
            "Enabled": "true",
            "Visibility": "0",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "7": {
            "Name": "Screen Safe",
            "Type": "0",
            "Info": "1",
            "Key": "46",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "8": {
            "Name": "HitscanFOV",
            "Type": "2",
            "Info": "0",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "true",
            "Parent": "-1"
        },
        "9": {
            "Name": "ProjectileFOV",
            "Type": "2",
            "Info": "1",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "10": {
            "Name": "MeleeFOV",
            "Type": "2",
            "Info": "2",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "11": {
            "Name": "ThrowableFOV",
            "Type": "2",
            "Info": "3",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "12": {
            "Name": "Sniper",
            "Type": "1",
            "Info": "7",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "8"
        },
        "13": {
            "Name": "SpyHFOV",
            "Type": "1",
            "Info": "8",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "8"
        },
        "14": {
            "Name": "Spec3rd",
            "Type": "4",
            "Info": "2",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "12"
        },
        "15": {
            "Name": "Spec1st",
            "Type": "4",
            "Info": "1",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "12"
        },
        "16": {
            "Name": "Spec3rd",
            "Type": "4",
            "Info": "2",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "19"
        },
        "17": {
            "Name": "Spec1st",
            "Type": "4",
            "Info": "1",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "19"
        },
        "18": {
            "Name": "Enabled",
            "Type": "0",
            "Info": "0",
            "Key": "6",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "true",
            "Parent": "0"
        },
        "19": {
            "Name": "Hitscan",
            "Type": "2",
            "Info": "0",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "true",
            "Parent": "23"
        },
        "20": {
            "Name": "Rocket Jump",
            "Type": "0",
            "Info": "0",
            "Key": "2",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "24"
        },
        "21": {
            "Name": "Clickaim",
            "Type": "0",
            "Info": "1",
            "Key": "5",
            "Enabled": "true",
            "Visibility": "1",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "22": {
            "Name": "TypeSwap",
            "Type": "0",
            "Info": "0",
            "Key": "16",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "true",
            "Parent": "18"
        },
        "23": {
            "Name": "Sniper",
            "Type": "1",
            "Info": "7",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "18"
        },
        "24": {
            "Name": "Soldier Only",
            "Type": "1",
            "Info": "1",
            "Key": "0",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "-1"
        },
        "25": {
            "Name": "Aim on Click",
            "Type": "0",
            "Info": "0",
            "Key": "1",
            "Enabled": "true",
            "Visibility": "2",
            "Not": "false",
            "Active": "false",
            "Parent": "21"
        }
    },
    "Vars": {
        "Vars::Menu::CheatTitle": {
            "-1": "Phobia"
        },
        "Vars::Menu::CheatTag": {
            "-1": "Phobia"
        },
        "Vars::Menu::CheatSubtitle": {
            "-1": "TF2 Software, Done PsychoStyle"
        },
        "Vars::Menu::PrimaryKey": {
            "-1": "45"
        },
        "Vars::Menu::SecondaryKey": {
            "-1": "119"
        },
        "Vars::Menu::BindWindow": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Menu::BindWindowTitle": {
            "-1": "true"
        },
        "Vars::Menu::MenuShowsBinds": {
            "-1": "true"
        },
        "Vars::Menu::BindPreview": {
            "-1": "true"
        },
        "Vars::Menu::Indicators": {
            "-1": "63",
            "7": "0"
        },
        "Vars::Menu::BindsDisplay": {
            "-1": {
                "x": "2",
                "y": "439"
            }
        },
        "Vars::Menu::TicksDisplay": {
            "-1": {
                "x": "342",
                "y": "855"
            }
        },
        "Vars::Menu::CritsDisplay": {
            "-1": {
                "x": "0",
                "y": "847"
            }
        },
        "Vars::Menu::SpectatorsDisplay": {
            "-1": {
                "x": "1905",
                "y": "442"
            }
        },
        "Vars::Menu::PingDisplay": {
            "-1": {
                "x": "1920",
                "y": "31"
            }
        },
        "Vars::Menu::ConditionsDisplay": {
            "-1": {
                "x": "575",
                "y": "848"
            }
        },
        "Vars::Menu::SeedPredictionDisplay": {
            "-1": {
                "x": "1863",
                "y": "71"
            }
        },
        "Vars::Menu::SnapOverlays": {
            "-1": "true"
        },
        "Vars::Menu::Overlay::Style": {
            "-1": "1"
        },
        "Vars::Menu::Overlay::Animated": {
            "-1": "false"
        },
        "Vars::Menu::Overlay::Rounded": {
            "-1": "true"
        },
        "Vars::Menu::Overlay::Glow": {
            "-1": "false"
        },
        "Vars::Menu::Overlay::Rail": {
            "-1": "false"
        },
        "Vars::Menu::Overlay::Ember": {
            "-1": "false"
        },
        "Vars::Menu::Overlay::Border": {
            "-1": "true"
        },
        "Vars::Menu::Overlay::GlowIntensity": {
            "-1": "0.600000024"
        },
        "Vars::Menu::Overlay::Opacity": {
            "-1": "0.850000024"
        },
        "Vars::Menu::Scale": {
            "-1": "1"
        },
        "Vars::Menu::CheapText": {
            "-1": "false"
        },
        "Vars::Menu::Chrome::Header": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::HeaderGlow": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::HeaderFx": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::Rail": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::SubtabIcons": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::Ember": {
            "-1": "true"
        },
        "Vars::Menu::Chrome::Splash": {
            "-1": "true"
        },
        "Vars::Menu::Theme::Accent": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::Background": {
            "-1": {
                "r": "31",
                "g": "31",
                "b": "31",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::Active": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::Inactive": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SectionBand": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SectionIcon": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SectionTitle": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SectionBackground": {
            "-1": {
                "r": "31",
                "g": "31",
                "b": "31",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SectionBorder": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "0"
            }
        },
        "Vars::Menu::Theme::SubtabBackground": {
            "-1": {
                "r": "31",
                "g": "31",
                "b": "31",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SubtabBorder": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "0"
            }
        },
        "Vars::Menu::Theme::SubtabSelected": {
            "-1": {
                "r": "128",
                "g": "70",
                "b": "70",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SubtabHovered": {
            "-1": {
                "r": "118",
                "g": "56",
                "b": "56",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SubtabAccent": {
            "-1": {
                "r": "255",
                "g": "118",
                "b": "118",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SubtabText": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SubtabTextSelected": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SliderTrack": {
            "-1": {
                "r": "43",
                "g": "24",
                "b": "24",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SliderFill": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SliderBorder": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::SliderAccent": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetBackground": {
            "-1": {
                "r": "36",
                "g": "20",
                "b": "29",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetBackgroundHover": {
            "-1": {
                "r": "31",
                "g": "31",
                "b": "31",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetBackgroundActive": {
            "-1": {
                "r": "49",
                "g": "26",
                "b": "39",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetBorder": {
            "-1": {
                "r": ")"
    R"(32",
                "g": "32",
                "b": "53",
                "a": "0"
            }
        },
        "Vars::Menu::Theme::WidgetBorderFocus": {
            "-1": {
                "r": "114",
                "g": "157",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Menu::Theme::WidgetIcon": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetText": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetTextDim": {
            "-1": {
                "r": "163",
                "g": "132",
                "b": "145",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetTextSub": {
            "-1": {
                "r": "152",
                "g": "152",
                "b": "152",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetTextSubHover": {
            "-1": {
                "r": "210",
                "g": "216",
                "b": "233",
                "a": "255"
            }
        },
        "Vars::Menu::Theme::WidgetCheckbox": {
            "-1": {
                "r": "185",
                "g": "82",
                "b": "82",
                "a": "255"
            }
        },
        "Vars::Menu::Bans::Enabled": {
            "-1": "true"
        },
        "Vars::Menu::Bans::SteamKey": {
            "-1": "F31A784EA215BE383FA061D1DB6C5C8F"
        },
        "Vars::Menu::Bans::SteamHistoryKey": {
            "-1": "703d372ab350bcee4f537b525c9d3b7a"
        },
        "Vars::Menu::Bans::RefreshInterval": {
            "-1": "120"
        },
        "Vars::Menu::Watermark::Enabled": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Menu::Watermark::Parts": {
            "-1": "13"
        },
        "Vars::Menu::Watermark::Style": {
            "-1": "1"
        },
        "Vars::Menu::Watermark::FlowSpeed": {
            "-1": "0.0500000007"
        },
        "Vars::Menu::Watermark::Icon": {
            "-1": "7"
        },
        "Vars::Menu::Watermark::Font": {
            "-1": "Serpentine"
        },
        "Vars::Menu::Watermark::FontSize": {
            "-1": "21"
        },
        "Vars::Menu::Music::Enabled": {
            "-1": "true"
        },
        "Vars::Menu::Music::ShowTitle": {
            "-1": "true"
        },
        "Vars::Menu::Music::ShowArtist": {
            "-1": "true"
        },
        "Vars::Menu::Music::ShowProgress": {
            "-1": "true"
        },
        "Vars::Menu::Music::Opacity": {
            "-1": "0.99999994"
        },
        "Vars::Menu::Music::Box": {
            "-1": {
                "x": "198",
                "y": "984",
                "w": "395",
                "h": "94"
            }
        },
        "Vars::Colors::Local": {
            "-1": {
                "r": "0",
                "g": "255",
                "b": "246",
                "a": "255"
            }
        },
        "Vars::Colors::FOVCircle": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "100"
            },
            "7": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::SmartFlickCrosshair": {
            "-1": {
                "r": "236",
                "g": "114",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::SoftAimCrosshair": {
            "-1": {
                "r": "120",
                "g": "255",
                "b": "120",
                "a": "255"
            }
        },
        "Vars::Colors::SpellFootstep": {
            "-1": {
                "r": "255",
                "g": "0",
                "b": "139",
                "a": "255"
            }
        },
        "Vars::Colors::WorldModulation": {
            "-1": {
                "r": "110",
                "g": "110",
                "b": "110",
                "a": "255"
            }
        },
        "Vars::Colors::SkyModulation": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::PropModulation": {
            "-1": {
                "r": "123",
                "g": "123",
                "b": "123",
                "a": "255"
            }
        },
        "Vars::Colors::ParticleModulation": {
            "-1": {
                "r": "117",
                "g": "252",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::FogModulation": {
            "-1": {
                "r": "237",
                "g": "255",
                "b": "0",
                "a": "255"
            }
        },
        "Vars::Colors::Line": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::LineIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::BoneHitboxEdge": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::BoneHitboxEdgeIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::BoneHitboxFace": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::BoneHitboxFaceIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::TargetHitboxEdge": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::TargetHitboxEdgeIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "150",
                "b": "150",
                "a": "0"
            }
        },
        "Vars::Colors::TargetHitboxFace": {
            "-1": {
                "r": "255",
                "g": "150",
                "b": "150",
                "a": "0"
            }
        },
        "Vars::Colors::TargetHitboxFaceIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "150",
                "b": "150",
                "a": "0"
            }
        },
        "Vars::Colors::BoundHitboxEdge": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::BoundHitboxEdgeIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::BoundHitboxFace": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::BoundHitboxFaceIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::PlayerPath": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::PlayerPathIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::ProjectilePath": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::ProjectilePathIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::TrajectoryPath": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::TrajectoryPathIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::ShotPath": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::ShotPathIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Colors::SplashRadius": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "0"
            }
        },
        "Vars::Colors::SplashRadiusIgnoreZ": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Aimbot::General::AimType": {
            "-1": "0",
            "16": "1",
            "25": "1",
            "17": "5",
            "18": "3",
            "22": "2"
        },
        "Vars::Aimbot::General::TargetSelection": {
            "-1": "0"
        },
        "Vars::Aimbot::General::Target": {
            "-1": "127"
        },
        "Vars::Aimbot::General::Ignore": {
            "-1": "1080"
        },
        "Vars::Aimbot::General::AimFOV": {
            "12": "15",
            "-1": "37",
            "11": "43",
            "14": "10",
            "8": "37",
            "9": "48",
            "10": "180",
            "13": "17",
            "15": "5"
        },
        "Vars::Aimbot::General::MaxTargets": {
            "-1": "1"
        },
        "Vars::Aimbot::General::IgnoreInvisible": {
            "-1": "60"
        },
        "Vars::Aimbot::General::AssistStrength": {
            "-1": "27"
        },
        "Vars::Aimbot::General::TickTolerance": {
            "-1": "6"
        },
        "Vars::Aimbot::General::AutoShoot": {
            "-1": "false",
            "18": "true",
            "25": "true"
        },
        "Vars::Aimbot::General::TargetLock": {
            "-1": "true"
        },
        "Vars::Aimbot::General::FOVCircle": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Aimbot::General::LeadAndRestrict": {
            "-1": "true"
        },
        "Vars::Aimbot::General::NoSpread": {
            "-1": "true"
        },
        "Vars::Aimbot::General::SmartFlick": {
            "-1": "false"
        },
        "Vars::Aimbot::General::Overflick": {
            "-1": "true"
        },
        "Vars::Aimbot::General::DistanceMin": {
            "-1": "3"
        },
        "Vars::Aimbot::General::DistanceMax": {
            "-1": "6"
        },
        "Vars::Aimbot::General::TimeMin": {
            "-1": "200"
        },
        "Vars::Aimbot::General::TimeMax": {
            "-1": "350"
        },
        "Vars::Aimbot::Hitscan::Hitboxes": {
            "-1": "63"
        },
        "Vars::Aimbot::Hitscan::MultipointHitboxes": {
            "-1": "0"
        },
        "Vars::Aimbot::Hitscan::Modifiers": {
            "-1": "126"
        },
        "Vars::Aimbot::Hitscan::MultipointScale": {
            "-1": "80"
        },
        "Vars::Aimbot::Hitscan::TapfireDistance": {
            "-1": "1000"
        },
        "Vars::Aimbot::Projectile::Method": {
            "-1": "1"
        },
        "Vars::Aimbot::Projectile::StrafePrediction": {
            "-1": "3"
        },
        "Vars::Aimbot::Projectile::SplashPrediction": {
            "-1": "1"
        },
        "Vars::Aimbot::Projectile::AutoDetonate": {
            "-1": "15"
        },
        "Vars::Aimbot::Projectile::AutoAirblast": {
            "-1": "2"
        },
        "Vars::Aimbot::Projectile::Hitboxes": {
            "-1": "23"
        },
        "Vars::Aimbot::Projectile::Modifiers": {
            "-1": "31"
        },
        "Vars::Aimbot::Projectile::MaxSimulationTime": {
            "-1": "1"
        },
        "Vars::Aimbot::Projectile::HitChance": {
            "-1": "60"
        },
        "Vars::Aimbot::Projectile::AutodetRadius": {
            "-1": "80"
        },
        "Vars::Aimbot::Projectile::SplashRadius": {
            "-1": "90"
        },
        "Vars::Aimbot::Projectile::AutoRelease": {
            "-1": "0"
        },
        "Vars::Aimbot::Projectile::DoubleDonkAbove": {
            "-1": "0"
        },
        "Vars::Aimbot::Projectile::DoubleDonk": {
            "-1": "true"
        },
        "Vars::Aimbot::Projectile::AlwaysCharge": {
            "-1": "false"
        },
        "Vars::Aimbot::Projectile::SmartSwap": {
            "-1": "true"
        },
        "Vars::Aimbot::Melee::AutoBackstab": {
            "-1": "true"
        },
        "Vars::Aimbot::Melee::IgnoreRazorback": {
            "-1": "true"
        },
        "Vars::Aimbot::Melee::SwingPrediction": {
            "-1": "true"
        },
        "Vars::Aimbot::Melee::WhipTeam": {
            "-1": "false"
        },
        "Vars::Aimbot::Healing::HealPriority": {
            "-1": "2"
        },
        "Vars::Aimbot::Healing::DangerIgnore": {
            "-1": "8"
        },
        "Vars::Aimbot::Healing::AutoHeal": {
            "-1": "true"
        },
        "Vars::Aimbot::Healing::AutoArrow": {
            "-1": "true"
        },
        "Vars::Aimbot::Healing::AutoRepair": {
            "-1": "false"
        },
        "Vars::Aimbot::Healing::AutoSandvich": {
            "-1": "true"
        },
        "Vars::Aimbot::Healing::AutoVaccinator": {
            "-1": "true"
        },
        "Vars::Aimbot::Healing::ActivateOnVoice": {
            "-1": "true"
        },
        "Vars::CritHack::ForceCrits": {
            "-1": "false",
            "1": "true"
        },
        "Vars::CritHack::AvoidRandomCrits": {
            "-1": "true"
        },
        "Vars::CritHack::AlwaysMeleeCrit": {
            "-1": "false"
        },
        "Vars::CritHack::CritEffects": {
            "-1": "false"
        },
        "Vars::Backtrack::Latency": {
            "-1": "10",
            "5": "145"
        },
        "Vars::Backtrack::Interp": {
            "-1": "85"
        },
        "Vars::Backtrack::Window")"
    R"(: {
            "-1": "200"
        },
        "Vars::Backtrack::PreferOnShot": {
            "-1": "false"
        },
        "Vars::Backtrack::PreferCrosshair": {
            "-1": "true"
        },
        "Vars::Doubletap::Doubletap": {
            "-1": "false",
            "2": "true"
        },
        "Vars::Doubletap::Warp": {
            "-1": "false",
            "3": "true"
        },
        "Vars::Doubletap::RechargeTicks": {
            "4": "true",
            "-1": "false"
        },
        "Vars::Doubletap::AntiWarp": {
            "-1": "true"
        },
        "Vars::Doubletap::TickLimit": {
            "-1": "22"
        },
        "Vars::Doubletap::WarpRate": {
            "-1": "2"
        },
        "Vars::Doubletap::RechargeLimit": {
            "-1": "24"
        },
        "Vars::Doubletap::PassiveRecharge": {
            "-1": "2",
            "5": "0"
        },
        "Vars::Fakelag::Fakelag": {
            "-1": "0",
            "5": "2"
        },
        "Vars::Fakelag::Options": {
            "-1": "0"
        },
        "Vars::Fakelag::PlainTicks": {
            "-1": "12"
        },
        "Vars::Fakelag::RandomTicks": {
            "-1": {
                "Min": "1",
                "Max": "22"
            }
        },
        "Vars::Fakelag::UnchokeOnAttack": {
            "-1": "true"
        },
        "Vars::Fakelag::RetainBlastJump": {
            "-1": "true"
        },
        "Vars::AutoPeek::Enabled": {
            "-1": "false"
        },
        "Vars::Speedhack::Scale": {
            "-1": "1"
        },
        "Vars::AntiAim::Enabled": {
            "-1": "false"
        },
        "Vars::AntiAim::PitchReal": {
            "-1": "0"
        },
        "Vars::AntiAim::PitchFake": {
            "-1": "0"
        },
        "Vars::AntiAim::YawReal": {
            "-1": "0"
        },
        "Vars::AntiAim::YawFake": {
            "-1": "0"
        },
        "Vars::AntiAim::RealYawBase": {
            "-1": "0"
        },
        "Vars::AntiAim::FakeYawBase": {
            "-1": "0"
        },
        "Vars::AntiAim::RealYawOffset": {
            "-1": "0"
        },
        "Vars::AntiAim::FakeYawOffset": {
            "-1": "0"
        },
        "Vars::AntiAim::RealYawValue": {
            "-1": "90"
        },
        "Vars::AntiAim::FakeYawValue": {
            "-1": "-90"
        },
        "Vars::AntiAim::SpinSpeed": {
            "-1": "15"
        },
        "Vars::AntiAim::MinWalk": {
            "-1": "true"
        },
        "Vars::AntiAim::HidePitchOnShot": {
            "-1": "false"
        },
        "Vars::Resolver::Enabled": {
            "-1": "false"
        },
        "Vars::Resolver::AutoResolve": {
            "-1": "false"
        },
        "Vars::Resolver::AutoResolveCheatersOnly": {
            "-1": "false"
        },
        "Vars::Resolver::AutoResolveHeadshotOnly": {
            "-1": "false"
        },
        "Vars::Resolver::AutoResolveYawAmount": {
            "-1": "90"
        },
        "Vars::Resolver::AutoResolvePitchAmount": {
            "-1": "90"
        },
        "Vars::Resolver::CycleYaw": {
            "-1": "0"
        },
        "Vars::Resolver::CyclePitch": {
            "-1": "0"
        },
        "Vars::Resolver::CycleView": {
            "-1": "false"
        },
        "Vars::Resolver::CycleMinwalk": {
            "-1": "false"
        },
        "Vars::ESP::ActiveGroups": {
            "-1": "-1",
            "7": "-16"
        },
        "Vars::Visuals::UI::StreamerMode": {
            "-1": "0"
        },
        "Vars::Visuals::UI::StreamerName": {
            "-1": ""
        },
        "Vars::Visuals::UI::RandomNames": {
            "-1": "false"
        },
        "Vars::Visuals::UI::ChatTags": {
            "-1": "15",
            "7": "0"
        },
        "Vars::Visuals::UI::FieldOfView": {
            "-1": "120",
            "7": "90"
        },
        "Vars::Visuals::UI::ZoomFieldOfView": {
            "-1": "0",
            "6": "70"
        },
        "Vars::Visuals::UI::AspectRatio": {
            "-1": "0",
            "7": "0"
        },
        "Vars::Visuals::UI::RevealScoreboard": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::UI::ScoreboardUtility": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::UI::ScoreboardColors": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::UI::CleanScreenshots": {
            "-1": "true"
        },
        "Vars::Visuals::UI::DiscordRPC": {
            "-1": "false"
        },
        "Vars::Visuals::Thirdperson::Enabled": {
            "-1": "false",
            "6": "true"
        },
        "Vars::Visuals::Thirdperson::Crosshair": {
            "-1": "true"
        },
        "Vars::Visuals::Thirdperson::Distance": {
            "-1": "40"
        },
        "Vars::Visuals::Thirdperson::Right": {
            "-1": "25"
        },
        "Vars::Visuals::Thirdperson::Up": {
            "-1": "0"
        },
        "Vars::Visuals::Removals::Interpolation": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::Lerp": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::Disguises": {
            "-1": "false"
        },
        "Vars::Visuals::Removals::Taunts": {
            "-1": "false"
        },
        "Vars::Visuals::Removals::Scope": {
            "-1": "false",
            "6": "true"
        },
        "Vars::Visuals::Removals::PostProcessing": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::ScreenOverlays": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::ScreenEffects": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::ViewPunch": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::AngleForcing": {
            "-1": "false"
        },
        "Vars::Visuals::Removals::Ragdolls": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Removals::Gibs": {
            "-1": "false"
        },
        "Vars::Visuals::Removals::MOTD": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Effects::BulletTracer": {
            "-1": "C.A.P.P.E.R",
            "7": "Default"
        },
        "Vars::Visuals::Effects::CritTracer": {
            "-1": "Sniper rail",
            "7": "Default"
        },
        "Vars::Visuals::Effects::MedigunBeam": {
            "-1": "Bombonomicon",
            "7": "Default"
        },
        "Vars::Visuals::Effects::MedigunCharge": {
            "-1": "Plasma",
            "7": "Default"
        },
        "Vars::Visuals::Effects::ProjectileTrail": {
            "-1": "Energy",
            "7": "Default"
        },
        "Vars::Visuals::Effects::SpellFootsteps": {
            "-1": "1",
            "7": "0"
        },
        "Vars::Visuals::Effects::RagdollEffects": {
            "-1": "31",
            "7": "0"
        },
        "Vars::Visuals::Effects::DrawIconsThroughWalls": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Effects::DrawDamageNumbersThroughWalls": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Viewmodel::CrosshairAim": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Viewmodel::ViewmodelAim": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Viewmodel::OffsetX": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::OffsetY": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::OffsetZ": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::Pitch": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::Yaw": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::Roll": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::SwayScale": {
            "-1": "0"
        },
        "Vars::Visuals::Viewmodel::SwayInterp": {
            "-1": "0"
        },
        "Vars::Visuals::World::Modulations": {
            "-1": "15",
            "7": "0"
        },
        "Vars::Visuals::World::SkyboxChanger": {
            "-1": "sky_nightfall_01",
            "7": "Off"
        },
        "Vars::Visuals::World::WorldTexture": {
            "-1": "Default"
        },
        "Vars::Visuals::World::NearPropFade": {
            "-1": "false"
        },
        "Vars::Visuals::World::NoPropFade": {
            "-1": "false"
        },
        "Vars::Visuals::Beams::Model": {
            "-1": "sprites\/physbeam.vmt"
        },
        "Vars::Visuals::Beams::Life": {
            "-1": "2"
        },
        "Vars::Visuals::Beams::Width": {
            "-1": "2"
        },
        "Vars::Visuals::Beams::EndWidth": {
            "-1": "2"
        },
        "Vars::Visuals::Beams::FadeLength": {
            "-1": "10"
        },
        "Vars::Visuals::Beams::Amplitude": {
            "-1": "2"
        },
        "Vars::Visuals::Beams::Brightness": {
            "-1": "255"
        },
        "Vars::Visuals::Beams::Speed": {
            "-1": "0.200000003"
        },
        "Vars::Visuals::Beams::Segments": {
            "-1": "2"
        },
        "Vars::Visuals::Beams::Color": {
            "-1": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            }
        },
        "Vars::Visuals::Beams::Flags": {
            "-1": "65792"
        },
        "Vars::Visuals::Line::TracersEnabled": {
            "-1": "false"
        },
        "Vars::Visuals::Line::DrawDuration": {
            "-1": "1"
        },
        "Vars::Visuals::Hitbox::BonesEnabled": {
            "-1": "2",
            "7": "0"
        },
        "Vars::Visuals::Hitbox::BoundsEnabled": {
            "-1": "6",
            "7": "0"
        },
        "Vars::Visuals::Hitbox::DrawDuration": {
            "-1": "1"
        },
        "Vars::Visuals::Prediction::PlayerPath": {
            "-1": "4",
            "7": "0"
        },
        "Vars::Visuals::Prediction::ProjectilePath": {
            "-1": "4",
            "7": "0"
        },
        "Vars::Visuals::Prediction::SwingLines": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Visuals::Prediction::PlayerDrawDuration": {
            "-1": "5"
        },
        "Vars::Visuals::Prediction::ProjectileDrawDuration": {
            "-1": "5"
        },
        "Vars::Visuals::Prediction::SmartFlickCrosshair": {
            "-1": "true"
        },
        "Vars::Visuals::Prediction::SoftAimCrosshair": {
            "-1": "true"
        },
        "Vars::Visuals::Simulation::TrajectoryPath": {
            "-1": "4",
            "7": "0"
        },
        "Vars::Visuals::Simulation::ShotPath": {
            "-1": "4",
            "7": "0"
        },
        "Vars::Visuals::Simulation::SplashRadius": {
            "-1": "31",
            "7": "0"
        },
        "Vars::Visuals::Simulation::ProjectileCamera": {
            "-1": "false"
        },
        "Vars::Visuals::Simulation::ProjectileWindow": {
            "-1": {
                "x": "269",
                "y": "781",
                "w": "200",
                "h": "200"
            }
        },
        "Vars::Visuals::Simulation::Box": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Movement::AutoStrafe": {
            "-1": "2",
            "7": "0"
        },
        "Vars::Misc::Movement::AutoStrafeTurnScale": {
            "-1": "0.699999988"
        },
        "Vars::Misc::Movement::AutoStrafeMaxDelta": {
            "-1": "180"
        },
        "Vars::Misc::Movement::Bunnyhop": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Movement::EdgeJump": {
            "-1": "false"
        },
        "Vars::Misc::Movement::AutoJumpbug": {
            "-1": "false"
        },
        "Vars::Misc::Movement::BreakJump": {
            "-1": "false"
        },
        "Vars::Misc::Movement::AutoRocketJump": {
            "20": "true",
            "-1": "false"
        },
        "Vars::Misc::Movement::AutoCTap": {
            "-1": "false"
        },
        "Vars::Misc::Movement::AutoFaNJump": {
            "-1": "false"
        },
        "Vars::Misc::Movement::AutoRevJump": {
            "-1": "false"
        },
        "Vars::Misc::Movement::FastStop": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Movement::FastAccelerate": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Movement::DuckSpeed": {
            "-1": "false"
        },
        "Vars::Misc::Movement::ShieldTurnRate": {
            "-1": "true"
        },
        "Vars::Misc::Movement::NoPush": {
            "-1": "false"
        },
        "Vars::Misc::Movement::MovementLock": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AntiBackstab": {
            "-1": "0"
        },
        "Vars::Misc::Automation::TauntControl": {
            "-1": "false"
        },
        "Vars::Misc::Automation::KartControl": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AntiAutobalance": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AntiAFK": {
            "-1": "true"
        },
        "Vars::Misc::Automation::AutoF2Ignored": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AutoF1Priority": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AutoVote": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AutoVoteDefensive": {
            "-1": "false"
        },
        "Vars::Misc::Automation::AutoVoteTargets": {
            "-1": "7"
        },
        "Vars::Misc::Automation::AcceptItemDrops": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Exploits::PureBypass": {
            "-1": "true"
        },
        "Vars::Misc::Exploits::CheatsBypass": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::UnlockCVars": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::EquipRegionUnlock": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::BackpackExpander": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::NoisemakerSpam": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::PingReducer": {
            "-1": "false"
        },
        "Vars::Misc::Exploits::PingTarget": {
            "-1": "1"
        },
        "Vars::Misc::Game::NetworkFix": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Game::SetupBonesOptimization": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Game::AntiCheatCompatibility": {
            "-1": "false"
        },
        "Vars::Misc::Queueing::ForceRegions": {
            "-1": "63"
        },
        "Vars::Misc::)"
    R"(Queueing::FastQueue": {
            "-1": "true"
        },
        "Vars::Misc::Queueing::FastQueueMaxPing": {
            "-1": "200"
        },
        "Vars::Misc::Queueing::FastQueueBlockEU": {
            "-1": "4194303"
        },
        "Vars::Misc::Queueing::FastQueueBlockUS": {
            "-1": "258048"
        },
        "Vars::Misc::Queueing::ExtendQueue": {
            "-1": "false"
        },
        "Vars::Misc::Queueing::AutoCasualQueue": {
            "-1": "false"
        },
        "Vars::Misc::MannVsMachine::InstantRespawn": {
            "-1": "false"
        },
        "Vars::Misc::MannVsMachine::InstantRevive": {
            "-1": "false"
        },
        "Vars::Misc::MannVsMachine::AllowInspect": {
            "-1": "false"
        },
        "Vars::Misc::MannVsMachine::TrailMarkers": {
            "-1": "true"
        },
        "Vars::Misc::MannVsMachine::DormantESP": {
            "-1": "true"
        },
        "Vars::Misc::Sound::Block": {
            "-1": "9",
            "7": "0"
        },
        "Vars::Misc::Sound::HitsoundAlways": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Sound::RemoveDSP": {
            "-1": "true",
            "7": "false"
        },
        "Vars::Misc::Sound::GiantWeaponSounds": {
            "-1": "true",
            "7": "false"
        },
        "Vars::SkinChanger::Enabled": {
            "-1": "false"
        },
        "Vars::SkinChanger::iVariant": {
            "-1": "0"
        },
        "Vars::SkinChanger::iPaintKit": {
            "-1": "0"
        },
        "Vars::SkinChanger::flWear": {
            "-1": "0"
        },
        "Vars::SkinChanger::iSeed": {
            "-1": "0"
        },
        "Vars::SkinChanger::iKillstreakTier": {
            "-1": "0"
        },
        "Vars::SkinChanger::iSheen": {
            "-1": "6"
        },
        "Vars::SkinChanger::iIdleEffect": {
            "-1": "0"
        },
        "Vars::SkinChanger::iParticle": {
            "-1": "0"
        },
        "Vars::SkinChanger::bFestivized": {
            "-1": "false"
        },
        "Vars::SkinChanger::bAustralium": {
            "-1": "false"
        },
        "Vars::Logging::Logs": {
            "-1": "511"
        },
        "Vars::Logging::NotificationPosition": {
            "-1": "3"
        },
        "Vars::Logging::NotificationTime": {
            "-1": "2.5"
        },
        "Vars::Logging::MaxNotifications": {
            "-1": "3"
        },
        "Vars::Logging::NotificationProgress": {
            "-1": "true"
        },
        "Vars::Logging::VoteStart::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::VoteCast::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::ClassChange::LogTo": {
            "-1": "57",
            "7": "56"
        },
        "Vars::Logging::Damage::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::CheatDetection::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::Tags::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::Aliases::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::Resolver::LogTo": {
            "-1": "59",
            "7": "56"
        },
        "Vars::Logging::Bans::LogTo": {
            "-1": "59"
        },
        "Vars::CheatDetection::Methods": {
            "-1": "127"
        },
        "Vars::CheatDetection::MarkAsSuspect": {
            "-1": "true"
        },
        "Vars::CheatDetection::DetectionsRequired": {
            "-1": "13"
        },
        "Vars::CheatDetection::MinChoking": {
            "-1": "20"
        },
        "Vars::CheatDetection::MinFlick": {
            "-1": "20"
        },
        "Vars::CheatDetection::MaxNoise": {
            "-1": "1"
        },
        "Vars::CheatDetection::PingThresholdHigh": {
            "-1": "100"
        },
        "Vars::CheatDetection::PingThresholdLow": {
            "-1": "5"
        },
        "Vars::Debug::CrashLogging": {
            "-1": "true"
        },
        "Vars::Debug::SendCrashLogs": {
            "-1": "true"
        },
        "Vars::Radio::Enabled": {
            "-1": "false"
        },
        "Vars::Radio::Station": {
            "-1": "3"
        },
        "Vars::Radio::Volume": {
            "-1": "1"
        },
        "Vars::Radio::ScaleToGameVolume": {
            "-1": "false"
        }
    },
    "Groups": {
        "0": {
            "Name": "Enemies",
            "Color": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            },
            "TagsOverrideColor": "true",
            "Targets": "7",
            "Conditions": "13",
            "Players": "0",
            "Buildings": "0",
            "Projectiles": "0",
            "ESP": "8385873",
            "Chams": {
                "Visible": [
                    {
                        "Material": "Original",
                        "Color": {
                            "r": "255",
                            "g": "255",
                            "b": "255",
                            "a": "255"
                        }
                    }
                ],
                "Occluded": [
                    {
                        "Material": "Shaded",
                        "Color": {
                            "r": "0",
                            "g": "0",
                            "b": "0",
                            "a": "255"
                        }
                    },
                    {
                        "Material": "Fresnel",
                        "Color": {
                            "r": "7",
                            "g": "3",
                            "b": "3",
                            "a": "255"
                        }
                    }
                ]
            },
            "Glow": {
                "Stencil": "1",
                "Blur": "0"
            },
            "OffscreenArrows": "true",
            "OffscreenArrowsOffset": "100",
            "OffscreenArrowsMaxDistance": "1000",
            "PickupTimer": "false",
            "Backtrack": "7",
            "BacktrackChams": {
                "Visible": [
                    {
                        "Material": "Flat",
                        "Color": {
                            "r": "50",
                            "g": "28",
                            "b": "0",
                            "a": "255"
                        }
                    },
                    {
                        "Material": "Fresnel",
                        "Color": {
                            "r": "206",
                            "g": "189",
                            "b": "169",
                            "a": "255"
                        }
                    }
                ],
                "Occluded": ""
            },
            "BacktrackGlow": {
                "Stencil": "1",
                "Blur": "0"
            },
            "Trajectory": "93",
            "Sightlines": "2"
        },
        "1": {
            "Name": "Misc",
            "Color": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            },
            "TagsOverrideColor": "true",
            "Targets": "8176",
            "Conditions": "1023",
            "Players": "0",
            "Buildings": "0",
            "Projectiles": "0",
            "ESP": "0",
            "Chams": {
                "Visible": [
                    {
                        "Material": "Original",
                        "Color": {
                            "r": "255",
                            "g": "255",
                            "b": "255",
                            "a": "255"
                        }
                    }
                ],
                "Occluded": ""
            },
            "Glow": {
                "Stencil": "1",
                "Blur": "1"
            },
            "OffscreenArrows": "false",
            "OffscreenArrowsOffset": "100",
            "OffscreenArrowsMaxDistance": "1000",
            "PickupTimer": "true",
            "Backtrack": "0",
            "BacktrackChams": {
                "Visible": "",
                "Occluded": ""
            },
            "BacktrackGlow": {
                "Stencil": "0",
                "Blur": "0"
            },
            "Trajectory": "0",
            "Sightlines": "2"
        },
        "2": {
            "Name": "hands",
            "Color": {
                "r": "119",
                "g": "255",
                "b": "151",
                "a": "255"
            },
            "TagsOverrideColor": "true",
            "Targets": "16384",
            "Conditions": "0",
            "Players": "0",
            "Buildings": "0",
            "Projectiles": "0",
            "ESP": "0",
            "Chams": {
                "Visible": [
                    {
                        "Material": "Original",
                        "Color": {
                            "r": "255",
                            "g": "255",
                            "b": "255",
                            "a": "255"
                        }
                    },
                    {
                        "Material": "Tint",
                        "Color": {
                            "r": "185",
                            "g": "82",
                            "b": "82",
                            "a": "255"
                        }
                    },
                    {
                        "Material": "Fresnel",
                        "Color": {
                            "r": "7",
                            "g": "3",
                            "b": "3",
                            "a": "255"
                        }
                    }
                ],
                "Occluded": ""
            },
            "Glow": {
                "Stencil": "0",
                "Blur": "0"
            },
            "OffscreenArrows": "false",
            "OffscreenArrowsOffset": "100",
            "OffscreenArrowsMaxDistance": "1000",
            "PickupTimer": "false",
            "Backtrack": "0",
            "BacktrackChams": {
                "Visible": "",
                "Occluded": ""
            },
            "BacktrackGlow": {
                "Stencil": "0",
                "Blur": "0"
            },
            "Trajectory": "0",
            "Sightlines": "2"
        },
        "3": {
            "Name": "nohands",
            "Color": {
                "r": "255",
                "g": "255",
                "b": "255",
                "a": "255"
            },
            "TagsOverrideColor": "true",
            "Targets": "32768",
            "Conditions": "0",
            "Players": "0",
            "Buildings": "0",
            "Projectiles": "0",
            "ESP": "0",
            "Chams": {
                "Visible": [
                    {
                        "Material": "Shaded",
                        "Color": {
                            "r": "185",
                            "g": "82",
                            "b": "82",
                            "a": "255"
                        }
                    },
                    {
                        "Material": "Fresnel",
                        "Color": {
                            "r": "15",
                            "g": "0",
                            "b": "4",
                            "a": "255"
                        }
                    }
                ],
                "Occluded": ""
            },
            "Glow": {
                "Stencil": "0",
                "Blur": "0"
            },
            "OffscreenArrows": "false",
            "OffscreenArrowsOffset": "100",
            "OffscreenArrowsMaxDistance": "1000",
            "PickupTimer": "false",
            "Backtrack": "0",
            "BacktrackChams": {
                "Visible": "",
                "Occluded": ""
            },
            "BacktrackGlow": {
                "Stencil": "0",
                "Blur": "0"
            },
            "Trajectory": "0",
            "Sightlines": "2"
        }
    }
}
)"
;