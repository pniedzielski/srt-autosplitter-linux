#include <array>          // std::array<T,N>
#include <codecvt>        // std::codecvt_utf8_utf16
#include <chrono>         // std::chrono::steady_clock, namespace std::chrono_literals;
#include <cstddef>        // std::byte
#include <iostream>       // std::cout, std::cerr
#include <locale>         // std::wstring_convert
#include <string>         // std::string
#include <string_view>    // std::u16string_view
#include <thread>         // std::this_thread
#include <unordered_map>  // std::unordered_map<K,V>
#include <unordered_set>  // std::unordered_set<K>

#include "env.hpp"
#include "fake_timer.hpp"
#include "pointer_path.hpp"
#include "settings.hpp"

using namespace std::literals;

struct state {
  // Set to 0 when loading, set to 1 otherwise (foundable as a 4-byte
  // searching for 256)
  pointer_path<std::byte> is_not_loading{ 0x03415F30, 0xF8, 0x4A8, 0xE19 };

  // Set to 0 in game, 1 if in menu, 15 if in graphics submenu
  pointer_path<std::byte> in_menu{ 0x034160D0, 0x20, 0x218, 0x60 };

  // Set to 0 in title screen and main menu, set to 1 everywhere else
  pointer_path<std::byte> in_game{ 0x03415F30, 0xF0, 0x378, 0x564 };

  // Counts Ripto's 3rd phase health (init at 8 from the very
  // beginning of Ripto 1 fight, can be frozen at 0 to end the fight)
  pointer_path<std::byte> health_ripto3{ 0x03415F30, 0x110, 0x50, 0x140, 0x8, 0x1D0, 0x134 };

  // Counts Sorceress's 2nd phase health (init at 10 from the very
  // beginning of Sorc 1 fight, can be frozen at 0 to end the fight)
  //  pointer_path<std::byte> health_sorceress2{ 0x0 };    // TODO

  // ID of the map the player is being in
  pointer_path<std::array<char16_t, 256>> map{ 0x03415F30, 0x138, 0xB0, 0xB0, 0x598, 0x210, 0xB8, 0x148, 0x190, 0x0 };

  void update() {
    is_not_loading.update();
    in_menu.update();
    in_game.update();
    health_ripto3.update();
    map.update();
  }
};

auto current = state{};
auto old     = state{};

auto already_triggered_splits  = std::unordered_set<std::string_view>{};
auto last_level_exit_timestamp = std::chrono::steady_clock::duration::zero();

// Maps info tuples contains :
//  1) internal map ID           (string)
//  2) English display name      (string)
auto maps = std::unordered_map<std::string_view, std::pair<std::u16string_view, std::string_view>>{
  // Spyro 1 maps
  { "s1_stone_hill"sv,              { u"/LS102_StoneHill/Maps/"sv,          "Stone Hill"sv       } },
  { "s1_dark_hollow"sv,             { u"/LS103_DarkHollow/Maps/"sv,         "Dark Hollow"sv      } },
  { "s1_town_square"sv,             { u"/LS104_Townsquare/Maps/"sv,         "Town Square"sv      } },
  { "s1_sunny_flight"sv,            { u"/LS105_Sunnyflight/Maps/"sv,        "Sunny Flight"sv     } },
  { "s1_toasty"sv,                  { u"/LS106_Toasty/Maps/"sv,             "Toasty"sv           } },
  { "s1_dry_canyon"sv,              { u"/LS108_DryCanyon/Maps/"sv,          "Dry Canyon"sv       } },
  { "s1_cliff_town"sv,              { u"/LS109_CliffTown/Maps/"sv,          "Cliff Town"sv       } },
  { "s1_ice_cavern"sv,              { u"/LS110_IceCavern/Maps/"sv,          "Ice Cavern"sv       } },
  { "s1_night_flight"sv,            { u"/LS111_NightFlight/Maps/"sv,        "Night Flight"sv     } },
  { "s1_doctor_shemp"sv,            { u"/LS112_DrShemp/Maps/"sv,            "Doctor Shemp"sv     } },
  { "s1_alpine_ridge"sv,            { u"/LS114_AlpineRidge/Maps/"sv,        "Alpine Ridge"sv     } },
  { "s1_high_caves"sv,              { u"/LS115_HighCaves/Maps/"sv,          "High Caves"sv       } },
  { "s1_wizard_peak"sv,             { u"/LS116_WizardPeak/Maps/"sv,         "Wizard Peak"sv      } },
  { "s1_crystal_flight"sv,          { u"/LS117_CrystalFlight/Maps/"sv,      "Crystal Flight"sv   } },
  { "s1_blowhard"sv,                { u"/LS118_Blowhard/Maps/"sv,           "Blowhard"sv         } },
  { "s1_terrace_village"sv,         { u"/LS120_TerraceVillage/Maps/"sv,     "Terrace Village"sv  } },
  { "s1_misty_bog"sv,               { u"/LS121_MistyBog/Maps/"sv,           "Misty Bog"sv        } },
  { "s1_tree_tops"sv,               { u"/LS122_TreeTops/Maps/"sv,           "Tree Tops"sv        } },
  { "s1_wild_flight"sv,             { u"/LS123_WildFlight/Maps/"sv,         "Wild Flight"sv      } },
  { "s1_metalhead"sv,               { u"/LS124_MetalHead/Maps/"sv,          "Metalhead"sv        } },
  { "s1_dark_passage"sv,            { u"/LS126_DarkPassage/Maps/"sv,        "Dark Passage"sv     } },
  { "s1_lofty_castle"sv,            { u"/LS127_LoftyCastle/Maps/"sv,        "Lofty Castle"sv     } },
  { "s1_haunted_towers"sv,          { u"/LS128_HauntedTowers/Maps/"sv,      "Haunted Towers"sv   } },
  { "s1_icy_flight"sv,              { u"/LS129_IcyFlight/Maps/"sv,          "Icy Flight"sv       } },
  { "s1_jacques"sv,                 { u"/LS130_Jacques/Maps/"sv,            "Jacques"sv          } },
  { "s1_gnorc_cove"sv,              { u"/LS132_GnorcCove/Maps/"sv,          "Gnorc Cove"sv       } },
  { "s1_twilight_harbor"sv,         { u"/LS133_TwlightHarbour/Maps/"sv,     "Twilight Harbor"sv  } },
  { "s1_gnasty_gnorc"sv,            { u"/LS134_GnastyGnorc/Maps/"sv,        "Gnasty Gnorc"sv     } },
  { "s1_gnastys_loot"sv,            { u"/LS135_GnastyLoot/Maps/"sv,         "Gnasty's Loot"sv    } },

  // Spyro 2 maps
  { "s2_glimmer"sv,                 { u"/LS202_Glimmer/Maps/"sv,            "Glimmer"sv          } },
  { "s2_idol_springs"sv,            { u"/LS203_IdolSprings/Maps/"sv,        "Idol Springs"sv     } },
  { "s2_colossus"sv,                { u"/LS204_Colossus/Maps/"sv,           "Colossus"sv         } },
  { "s2_hurricos"sv,                { u"/LS205_Hurricos/Maps/"sv,           "Hurricos"sv         } },
  { "s2_sunny_beach"sv,             { u"/LS206_SunnyBeach/Maps/"sv,         "Sunny Beach"sv      } },
  { "s2_aquaria_towers"sv,          { u"/LS207_AquariaTowers/Maps/"sv,      "Aquaria Towers"sv   } },
  { "s2_crushs_dungeon"sv,          { u"/LS208_CrushsDungeon/Maps/"sv,      "Crush's Dungeon"sv  } },
  { "s2_ocean_speedway"sv,          { u"/LS209_OceanSpeedway/Maps/"sv,      "Ocean Speedway"sv   } },
  { "s2_crystal_glacier"sv,         { u"/LS211_CrystalGlacier/Maps/"sv,     "Crystal Glacier"sv  } },
  { "s2_skelos_badlands"sv,         { u"/LS212_SkelosBadlands/Maps/"sv,     "Skelos Badlands"sv  } },
  { "s2_zephyr"sv,                  { u"/LS213_Zephyr/Maps/"sv,             "Zephyr"sv           } },
  { "s2_breeze_harbor"sv,           { u"/LS214_BreezeHarbor/Maps/"sv,       "Breeze Harbor"sv    } },
  { "s2_scorch"sv,                  { u"/LS215_Scorch/Maps/"sv,             "Scorch"sv           } },
  { "s2_fracture_hills"sv,          { u"/LS216_FractureHills/Maps/"sv,      "Fracture Hills"sv   } },
  { "s2_magma_cone"sv,              { u"/LS217_MagmaCone/Maps/"sv,          "Magma Cone"sv       } },
  { "s2_shady_oasis"sv,             { u"/LS218_ShadyOasis/Maps/"sv,         "Shady Oasis"sv      } },
  { "s2_gulps_overlook"sv,          { u"/LS219_GulpsOverlook/Maps/"sv,      "Gulp's Overlook"sv  } },
  { "s2_icy_speedway"sv,            { u"/LS220_IcySpeedway/Maps/"sv,        "Icy Speedway"sv     } },
  { "s2_metro_speedway"sv,          { u"/LS221_MetroSpeedway/Maps/"sv,      "Metro Speedway"sv   } },
  { "s2_mystic_marsh"sv,            { u"/LS223_MysticMarsh/Maps/"sv,        "Mystic Marsh"sv     } },
  { "s2_cloud_temples"sv,           { u"/LS224_CloudTemples/Maps/"sv,       "Cloud Temples"sv    } },
  { "s2_metropolis"sv,              { u"/LS225_Metropolis/Maps/"sv,         "Metropolis"sv       } },
  { "s2_robotica_farms"sv,          { u"/LS226_RoboticaFarms/Maps/"sv,      "Robotica Farms"sv   } },
  { "s2_riptos_arena"sv,            { u"/LS227_RiptosArena/Maps/"sv,        "Ripto's Arena"sv    } },
  { "s2_canyon_speedway"sv,         { u"/LS228_CanyonSpeedway/Maps/"sv,     "Canyon Speedway"sv  } },
  { "s2_dragon_shores"sv,           { u"/LS229_DragonShores/Maps/"sv,       "Dragon Shores"sv    } },

  // Spyro 3 maps
  { "s3_sunny_villa"sv,             { u"/LS302_SunnyVilla/Maps/"sv,         "Sunny Villa"sv          } },
  { "s3_cloud_spires"sv,            { u"/LS303_CloudSpires/Maps/"sv,        "Cloud Spires"sv         } },
  { "s3_molten_crater"sv,           { u"/LS304_MoltenCrater/Maps/"sv,       "Molten Crater"sv        } },
  { "s3_seashell_shore"sv,          { u"/LS305_SeashellShore/Maps/"sv,      "Seashell Shore"sv       } },
  { "s3_sheilas_alp"sv,             { u"/LS306_SheilasAlp/Maps/"sv,         "Sheila's Alp"sv         } },
  { "s3_mushroom_speedway"sv,       { u"/LS307_MushroomSpeedway/Maps/"sv,   "Mushroom Speedway"sv    } },
  { "s3_buzzs_dungeon"sv,           { u"/LS308_BuzzsDungeon/Maps/"sv,       "Buzz's Dungeon"sv       } },
  { "s3_crawdad_farms"sv,           { u"/LS309_CrawdadFarm/Maps/"sv,        "Crawdad Farm"sv         } },
  { "s3_icy_peak"sv,                { u"/LS311_IcyPeak/Maps/"sv,            "Icy Peak"sv             } },
  { "s3_enchanted_towers"sv,        { u"/LS312_EnchantedTowers/Maps/"sv,    "Enchanted Towers"sv     } },
  { "s3_spooky_swamp"sv,            { u"/LS313_SpookySwamp/Maps/"sv,        "Spooky Swamp"sv         } },
  { "s3_bamboo_terrace"sv,          { u"/LS314_BambooTerrace/Maps/"sv,      "Bamboo Terrace"sv       } },
  { "s3_sgt_byrds_base"sv,          { u"/LS315_SgtByrdsBase/Maps/"sv,       "Sgt. Byrd's Base"sv     } },
  { "s3_country_speedway"sv,        { u"/LS316_CountrySpeedway/Maps/"sv,    "Country Speedway"sv     } },
  { "s3_spikes_arena"sv,            { u"/LS317_SpikesArena/Maps/"sv,        "Spike's Arena"sv        } },
  { "s3_spider_town"sv,             { u"/LS318_SpiderTown/Maps/"sv,         "Spider Town"sv          } },
  { "s3_lost_fleet"sv,              { u"/LS320_LostFleet/Maps/"sv,          "Lost Fleet"sv           } },
  { "s3_frozen_altars"sv,           { u"/LS321_FrozenAltars/Maps/"sv,       "Frozen Altars"sv        } },
  { "s3_fireworks_factory"sv,       { u"/LS322_FireworksFactory/Maps/"sv,   "Fireworks Factory"sv    } },
  { "s3_charmed_ridge"sv,           { u"/LS323_CharmedRidge/Maps/"sv,       "Charmed Ridge"sv        } },
  { "s3_bentleys_outpost"sv,        { u"/LS324_BentleysOutpost/Maps/"sv,    "Bentleys Outpost"sv     } },
  { "s3_honey_speedway"sv,          { u"/LS325_HoneySpeedway/Maps/"sv,      "Honey Speedway"sv       } },
  { "s3_scorchs_pit"sv,             { u"/LS326_ScorchsPit/Maps/"sv,         "Scorchs Pit"sv          } },
  { "s3_starfish_reef"sv,           { u"/LS327_StarfishReef/Maps/"sv,       "Starfish Reef"sv        } },
  { "s3_crystal_islands"sv,         { u"/LS329_CrystalIslands/Maps/"sv,     "Crystal Islands"sv      } },
  { "s3_desert_ruins"sv,            { u"/LS330_DesertRuins/Maps/"sv,        "Desert Ruins"sv         } },
  { "s3_haunted_tomb"sv,            { u"/LS331_HauntedTomb/Maps/"sv,        "Haunted Tomb"sv         } },
  { "s3_dino_mines"sv,              { u"/LS332_DinoMines/Maps/"sv,          "Dino Mines"sv           } },
  { "s3_agent_9s_lab"sv,            { u"/LS333_Agent9sLab/Maps/"sv,         "Agent 9's Lab"sv        } },
  { "s3_harbor_speedway"sv,         { u"/LS334_HarborSpeedway/Maps/"sv,     "Harbor Speedway"sv      } },
  { "s3_sorceresss_lair"sv,         { u"/LS335_SorceresssLair/Maps/"sv,     "Sorceress's Lair"sv     } },
  { "s3_bugbot_factory"sv,          { u"/LS336_BugbotFactory/Maps/"sv,      "Bugbot Factory"sv       } },
  { "s3_super_bonus"sv,             { u"/LS337_SuperBonusRound/Maps/"sv,    "Super Bonus Round"sv    } }
};

// This dictionary defines which autosplits require a specific
// transition to be triggered, and to which map the transition must
// lead.  This is especially useful for boss fights, where leaving the
// level without completing it must not split (only happens in level
// storage contexts)
auto specific_map_transitions = std::unordered_map<std::string_view, std::u16string_view>{
  { "s2_crushs_dungeon"sv, u"/LS210_AutumnPlains_Home/Maps/"sv },
  { "s2_gulps_overlook"sv, u"/LS222_WinterTundra_Home/Maps/"sv },
  { "s2_riptos_arena"sv,   u"/LS229_DragonShores/Maps/"sv      }
};

auto settings = settings_map{};

auto timer = fake_timer{};

bool start() {
  if (current.in_game() == std::byte{ 1 } && old.in_game() == std::byte{ 0 }) {
    already_triggered_splits.clear();
    timer.start();
    return true;
  }
  return false;
}

bool reset() {
  return settings["reset"s] && current.in_game() == std::byte{ 0 };
}

bool split() {
  auto old_map     = std::u16string_view{ old.map().data()     };
  auto current_map = std::u16string_view{ current.map().data() };

  // For each map...
  for (auto&& entry : maps) {
    auto split_code = entry.first;
    auto map_id     = entry.second.first;

    // An autosplit can only happen if we were in the currently
    // processed map, and if we aren't in it anymore.  If that's not
    // the case, no need to continue.
    if (old_map            != map_id ||
        current_map        == map_id ||
        current_map.size() == 0)
      continue;

    auto is_fast_exit = (timer.loadless() - last_level_exit_timestamp < 15s);
    last_level_exit_timestamp = timer.loadless();

    if (settings["ignore_fast_exits"] && is_fast_exit)
      break;

    // This autosplit needs to be verified if it's always enabled, or
    // if it's enabled for first exit check and it has not yet been
    // triggered.
    if (settings[std::string{ split_code } + "_everytime"s] ||
        (settings[std::string{ split_code } + "_first"s] &&
         already_triggered_splits.find(split_code) == end(already_triggered_splits))) {
      auto should_autosplit = true;

      // If a specific map transition is required, we autosplit only
      // if we go from map A to map B
      if (specific_map_transitions.find(split_code) != end(specific_map_transitions))
        should_autosplit = (current_map == specific_map_transitions[split_code]);

      if (should_autosplit) {
        already_triggered_splits.insert(split_code);
        return true;
      }
    }

    break;
  }

  if (current_map == maps["s2_riptos_arena"sv].first) {
    // "Enter Ripto's Arena" specific handling
    if (settings["s2_enter_ripto"s] && old_map != current_map)
      return true;

    // "Ripto (on last blow)" specific handling
    if (settings["s2_kill_ripto"s]                 &&
        old.health_ripto3()     == std::byte{ 1 }  &&
        current.health_ripto3() == std::byte{ 0 })
      return true;
  }

  return false;
}

bool is_loading() {
  // Game must be loading something to pause the timer
  if (current.is_not_loading() != std::byte{ 0 })
    return false;

  // Timer must never be paused on title screen
  if (current.in_game() == std::byte{ 0 })
    return false;

  // Timer must not be paused when inside menu to prevent abusing
  // this by buffering a loading and pausing the game at the exact
  // same frame.  We also check that the run didn't just start,
  // because the "in_menu" state is active during the fade to black
  // after game is selected (which would cause 0.9s to elapse on run
  // start whereas something is indeed loading).
  if (current.in_menu() > std::byte{ 0 } && timer.rta() >= 3s)
    return false;

  return true;
}


void send_start() {
  std::cout << "start\ninitgametime\n" << std::flush;
}

void send_reset() {
  std::cout << "reset\n" << std::flush;
}

void send_split() {
  std::cout << "split\n" << std::flush;
}

void send_pause() {
  std::cout << "pausegametime\n" << std::flush;
}

void send_resume() {
  std::cout << "resumegametime\n" << std::flush;
}


int main(int argc, char** argv) {
  using namespace std::chrono_literals;

  std::cout << "> Spyro Reignited Trilogy Autosplitter for Linux\n";

  set_pid();

  auto was_loading = false;

  // Initialize settings.
  settings.add("reset"s,             false, "Reset timer on title screen"s);
  settings.add("ignore_fast_exits"s, true,  "Ignore fast exits (time spent in level < 15s)"s);

  settings.add("s1"s, true, "Spyro the Dragon"s);
    settings.add("s1_first"s,       true, "Level exits (first time)"s, "s1"s);
    settings.add("s1_everytime"s,   true, "Level exits (every time)"s, "s1"s);
//    settings.add("s1_kill_gnasty"s, true, "Gnasty Gnorc (on kill)"s,   "s1"s);

  settings.add("s2"s, true, "Spyro 2: Ripto's Rage!"s);
    settings.add("s2_first"s,       true,  "Level exits (first time)"s,            "s2"s);
    settings.add("s2_everytime"s,   true,  "Level exits (every time)"s,            "s2"s);
    settings.add("s2_enter_ripto"s, false, "Enter Ripto's Arena"s,                 "s2"s);
    settings.add("s2_kill_ripto"s,  true,  "Ripto (on last blow) [EXPERIMENTAL]"s, "s2"s);

  settings.add("s3"s, true, "Spyro: Year of the Dragon"s);
    settings.add("s3_first"s,          true, "Level exits (first time)"s, "s3"s);
    settings.add("s3_everytime"s,      true, "Level exits (every time)"s, "s3"s);
//    settings.add("s3_kill_sorceress"s, true, "Sorceress (on last blow)"s, "s3"s);

  for (auto&& entry : maps) {
    auto split_code  = std::string{ entry.first };
    auto map_name    = std::string{ entry.second.second };
    auto game_prefix = split_code.substr(0, 2);

    settings.add(split_code + "_first"s,     true,  map_name, game_prefix + "_first"s);
    settings.add(split_code + "_everytime"s, false, map_name, game_prefix + "_everytime"s);
  }

  while (true) {
    old = current;
    current.update();

    auto map_name_utf16 = std::u16string_view{ current.map().data(), current.map().size() };
    auto map_name = std::wstring_convert<
      std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(map_name_utf16.data());

    // std::cout << "> is_not_loading: " << static_cast<int>(current.is_not_loading()) << '\n';
    // std::cout << "> in_menu:        " << static_cast<int>(current.in_menu())        << '\n';
    // std::cout << "> in_game:        " << static_cast<int>(current.in_game())        << '\n';
    // std::cout << "> health_ripto3:  " << static_cast<int>(current.health_ripto3())  << '\n';
    // std::cout << "> map:            " << map_name                                   << '\n';

    auto now_loading = is_loading();

    if (start())                      { send_start();  was_loading = false; }
    if (reset())                        send_reset();
    if (split())                        send_split();
    if (now_loading  && !was_loading) { send_pause();   timer.pause();      }
    if (!now_loading && was_loading)  { send_resume();  timer.resume();     }

    was_loading = now_loading;

    std::this_thread::sleep_for(33ms);
  }

  return 0;
}
