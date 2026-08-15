#pragma once


#include "game/game_data.hpp"
#include "asset_manager.hpp"

#include "utils/utils.hpp"



#include <regex> // std::regex

#include "utils/class/logger.hpp"
#include "utils/math.hpp"


/**
  * Every game object must be derive from a "stast" type object 
  * that will define the object and it's behaviour. all stats are
  * defined in json files and each function needs the folloing variables
  * to work:
  * 
  * t_configmap& - The map that contains all stats objects
  * AssetManager& - Game assets and sprites
  * t_enum_tree& - A list of all stats types and ids, each linked
  *				   to an empty tree that may be used for pathfinding.
  * nlohmann::json& - The original json file that needs to be read.
  * 
  * 
  * Non essential parameters:
  * 
  * t_weights* - Each reource have a weight that is used to find out
  * how many of that resource can be stored in a particular space.
  * std::map<std::string, size_t>* - If an object needs to have
  * spawning info of another object, this map can convert this info
  * to it's serializable id needed for the spawning process.
  */

	bool load_upgrade_trees(
		t_configmap&,
		AssetManager&,
		t_enum_tree& enumTree,
		std::map<std::string, size_t>& serMap,
		t_weights*,
		nlohmann::json&);

	bool load_enum_trees(
		nlohmann::json&,
		t_enum_tree&);

	bool load_serializable(
		nlohmann::json&,
		std::map<std::string, size_t>&);

	bool load_entity_stats(
		t_configmap&,
		AssetManager&,
		t_enum_tree& enumTree,
		std::map<std::string, size_t>& serMap,
		nlohmann::json&);

	bool load_bullets_stats(
		t_configmap&,
		AssetManager&,
		t_enum_tree& enumTree,
		nlohmann::json&);
