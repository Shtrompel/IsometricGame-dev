#include "file/json_assets.hpp"


static Resources string_to_cost(const std::string& str, t_weights* weights)
{
	assert(weights);
	std::vector<std::string> strings = str_split(str, ",");
	Resources ret;
	for (size_t i = 0; i < strings.size(); ++i)
	{
		auto spt = str_split(strings[i], ":");
		if (spt.empty())
			continue;
		else if (spt.size() == 1)
		{
			try
			{
				ret.resCount[i] = std::stoi(str_trim(spt[0]));
			}
			catch (std::invalid_argument const& ex)
			{
				WARNING("Error parsing int in data.json: %s",
					ex.what());
				continue;
			}
		}
		else if (spt.size() >= 2)
		{
			int v;
			std::string s;
			bool success;
			s = str_uppercase(str_trim(spt[0]));
			try
			{
				v = std::stoi(spt[1]);
			}
			catch (std::invalid_argument const& ex)
			{
				WARNING("Error parsing int in data.json: %s",
					ex.what());
				continue;
			}
			int id = (int)weights->get_id(s, &success);
			if (!success)
			{
				WARNING("Error parsing int in data.json: Unknwon resource name %s",
					s.c_str());
				continue;
			}
			ret.resCount[id] = v;
		}
	}
	return ret;
}


#define SET_STR(j, var, name) \
	if (j.contains(name))     \
	strcpy(var, j[name].get<std::string>().c_str())

#define SET_INT(j, var, name) \
	if (j.contains(name))     \
	var = j[name].get<int>()

#define SET_NUM(j, var, name) \
	if (j.contains(name))     \
	var = j[name].get<float>()

#define SET_COST(j, var, weights, name) \
	if (j.contains(name))               \
		var = string_to_cost(j[name].get<std::string>(), weights);
/*
#define SET_COST_POWER(j, varA, varB, name) \
	if (j.contains(name))                   \
		string_set_resources(varA, varB, j[name].get<std::string>());
*/
#define SET_IVEC(j, var, name) \
	if (j.contains(name))      \
		var = string_to_ivec(j[name].get<std::string>());

[[deprecated]] static void set_vector2i(sf::Vector2i& start, sf::Vector2i& end, const std::string& str)
{
	std::vector<std::string> strings = str_split(str, "-");
	for (size_t i = 0u; i < strings.size(); ++i)
	{
		std::vector<std::string> nums = str_split(strings[i], ",");
		int x = str_to_int(nums[0]);
		int y = str_to_int(nums[1]);
		((i == 0) ? start : end) = { x, y };
	}
}

 bool load_enum_trees(
	nlohmann::json& jsonFile,
	t_enum_tree& enums)
{
	using namespace nlohmann;
	for (auto itrGroup = jsonFile.cbegin(); itrGroup != jsonFile.cend(); ++itrGroup)
	{
		const json jsonGroup = *itrGroup;
		int id = -1;
		char title[32] = "";
		SET_INT(jsonGroup, id, "id");
		SET_STR(jsonGroup, title, "title");
		
		enums.add_group(id, str_uppercase(title));
		if (id == -1)
			continue;
		bool success;
		auto& group = enums.get_group(id, &success);
		if (!success)
		{
			WARNING("Could not found enum group %d", (int)id);
			continue;
		}
		assert(success);
		auto& jsonVals = jsonGroup["values"];
		size_t counter = 0;

		for (auto itrVal = jsonVals.cbegin();
			itrVal != jsonVals.cend();
			++itrVal)
		{
			const json jsonVal = *itrVal;
			group.add(counter++,
				str_uppercase(jsonVal.get<std::string>()),
				t_tiletree{ VEC_LIMIT, POINT_LIMIT });
		}
	}

	return true;
}

 inline std::string format_json_keyname(const std::string& str0)
 {
	 std::string str = str0;
	 std::transform(str.begin(), str.end(), str.begin(), [](char c) { return toupper(c); });
	 str = std::regex_replace(str, std::regex(" "), "");
	 return str;
 }

 static t_alignment alignment_json_to_int(
	 const nlohmann::json& jsonFile,
	 t_enum_tree& enumTree)
 {
	 if (jsonFile.contains("alignment"))
	 {
		 const nlohmann::json& jsonAlgn = jsonFile.at("alignment");
		 if (jsonAlgn.is_string())
		 {
			 std::string algnStr;
			 jsonFile.at("alignment").get_to(algnStr);

			 // DEBUG("%s %s", info.name, algnStr.c_str());
			 algnStr = format_json_keyname(algnStr);
			 if (enumTree.get_group(ENUM_ALIGNMENT).has(algnStr))
			 {
				 auto& group = enumTree.get_group(ENUM_ALIGNMENT);
				 return (int)group.get_id(algnStr);
			 }
			 else
				 WARNING("Use \"%s\" don't match any value in enum list", algnStr.c_str());
		 }
		 else if (jsonAlgn.is_number_unsigned())
		 {
			 return jsonFile.value("alignment", 0);
		 }
		 else
		 {
			 WARNING("Use \"%s\" unknown alignment type", jsonAlgn.dump().c_str());
		 }
	 }
	 else
	 {
		 WARNING("Missing alignment in json");
	 }

	 return 0;
 }

bool load_serializable(
	nlohmann::json& jsonFile,
	std::map<std::string, size_t>& ser)
{
	using namespace nlohmann;
	for (auto itrVal = jsonFile.cbegin(); itrVal != jsonFile.cend(); ++itrVal)
	{
		const json jsonVal = *itrVal;
		if (itrVal.value().is_number_unsigned())
			ser[itrVal.key()] = itrVal.value().get<size_t>();
	}

	return true;
}

/**
 * Insert a new image to assets, and get it's id for use
 */
static std::unordered_map<IVec, t_sprite>
sprite_info_to_id(
	AssetManager& assets,
	const IVec& start,
	const IVec& end,
	const char* img,
	const IVec& size = { 1, 1 })
{
	static const auto upgradeFrames =
		[](
			const sf::Vector2i& v0,
			const sf::Vector2i& v1) -> bool {
				return v0.x == -1 || v0.y == -1 || v1.x == -1 || v1.y == -1;
		};

	std::unordered_map<IVec, t_sprite> ret;

	if (!strcmp(img, ""))
		return ret;

	if (size == sf::Vector2i{ 1, 1 })
	{
		int mainSprite = assets.load_sprites(
			img,
			start,
			end);
		if (mainSprite == -1 && !upgradeFrames(start, end))
			LOG(
				"Texture \"%s\" with frames %s %s wil be initialized after upgrading",
				img,
				VEC_CSTR(start),
				VEC_CSTR(end));

		ret[IVec{ 0, 0 }] = mainSprite;
		return ret;
	}
	else
	{
		// Frame count is only for the gridline of the source image
		// meaning it will contain empty sprites.
		IVec frameCount;
		frameCount = assets.get_texture_from_name(
			img)
			.frameCount;
		int mainSprite = assets.load_sprites(
			img,
			start,
			end,
			frameCount);

		if (mainSprite == -1 && !upgradeFrames(start, end))
			LOG("Texture \"%s\" with frames %s %s wil be initialized after upgrading",
				img,
				VEC_CSTR(start),
				VEC_CSTR(end));

		assert(mainSprite > 0);
		for (int y = 0; y < size.y; ++y)
		{
			for (int x = 0; x < size.x; ++x)
			{
				sf::Vector2i pos = sf::Vector2i{ x, y };
				int v = assets.split_sprites(
					mainSprite,
					start,
					end,
					pos,
					size);
				if (mainSprite == DEBUG_SELECT_BUILDING_SPRITE)
				{
					DEBUG("Tile Pos: %s, Sprite: %d", VEC_CSTR(pos), v);
				}

				assert(assets.get_sprite(v));

				ret[pos] = v;
			}
		}

		return ret;
	}
}

/**
 * Convert a string representation of a sprite to sprite data
 */
static std::unordered_map<IVec, t_sprite>
sprite_repr_to_info(
	t_sprite* spriteOut,
	IVec* startOut,
	IVec* endOut,
	char* imgOut,
	AssetManager& assets,
	std::string strImage, // Do a copy
	std::string strSource,
	const IVec& size = { 1, 1 })
{
	std::unordered_map<IVec, t_sprite> ret;
	if (strSource.size() == 0)
		return ret;
	strSource = str_trim(strSource);
	size_t follower = 0;
	auto v = str_split(strSource, " ");
	if (v.size() > 1 && v[0].find(',') == std::string::npos)
		strImage = v[follower++];

	std::string frames = iterator_to_string_linear(
		std::next(v.begin(), follower),
		v.end(),
		"");
	IVec start, end;
	std::vector<std::string> strings = str_split(frames, "-");
	for (size_t i = 0u; i < strings.size(); ++i)
	{
		std::vector<std::string> nums = str_split(strings[i], ",");
		int x = str_to_int(nums[0]);
		int y = str_to_int(nums[1]);

		((i == 0) ? start : end) = { x, y };
	}

	ret = sprite_info_to_id(
		assets,
		start,
		end,
		strImage.c_str(),
		size);

	if (spriteOut && ret.size())
		*spriteOut = (*ret.begin()).second;
	if (startOut)
		*startOut = start;
	if (endOut)
		*endOut = end;
	if (imgOut)
		strcpy(imgOut, strImage.c_str());

	return ret;
}

/*
static t_spriteaxis sprite_axis_from_name(const std::string& raw)
{
    std::string s = format_json_keyname(raw);
    if (s == "ANIM") 
		return SPRITE_LAYOUT_ANIM;
    if (s == "FULLNESS") 
		return SPRITE_LAYOUT_FULLNESS;
    if (s == "OCCUPANT") 
		return SPRITE_LAYOUT_OCCUPENT;
	
	// Check if formatted like UPGRADEx where x is a non negative integer
	const char* COMPARE_STR = "UPGRADE";
	int xVal = 0;
	int i = 0;
	for (i = 0; i < raw.size(); ++i)
	{
		if (i < 8)
		{
			if (COMPARE_STR[i] == raw[i])
				continue;
			
			xVal = -1;
			break;
		}

		if (isdigit(raw[i]))
		{
			xVal *= 10;
			xVal += (int)(raw[i] - '0');
			continue;
		}

		xVal = -1;
		break;
	}

	if (xVal != -1)
	{
		return SPRITE_LAYOUT_UPGRADE0 + xVal;
	}

    return -1;
}
*/
// Maps the axis names used in buildings.json's "sprite_axes_x"/"sprite_axes_y"
// strings to the SPRITE_LAYOUT_* constants.
static bool sprite_axis_from_str(const std::string& str0, t_spriteaxis& out)
{
	std::string str = format_json_keyname(str0);
	if (str == "ANIM") { out = SPRITE_LAYOUT_ANIM; return true; }
	if (str == "FULLNESS") { out = SPRITE_LAYOUT_FULLNESS; return true; }
	if (str == "OCCUPENT") { out = SPRITE_LAYOUT_OCCUPENT; return true; }
	if (str == "UPGRADE0") { out = SPRITE_LAYOUT_UPGRADE0; return true; }
	if (str == "UPGRADE1") { out = SPRITE_LAYOUT_UPGRADE1; return true; }
	if (str == "UPGRADE2") { out = SPRITE_LAYOUT_UPGRADE2; return true; }
	WARNING("Unknown sprite axis name \"%s\"", str0.c_str());
	return false;
}

/**
 * Parses a "{AXIS:size, AXIS2:size2}" string - same brace-list convention
 * as "uses"/"properties"/"nums"/"bools" elsewhere in this file - into an
 * ordered axis list plus each axis' size. Order matters: the first entry
 * is the fastest-varying (stride 1) axis along that dimension, matching
 * SpriteSheetSystem::calculate_strides. The product of the sizes listed
 * becomes that dimension's grid size.
 */
static bool parse_sprite_axes_str(
	const std::string& strIn,
	std::vector<t_spriteaxis>& axesOut,
	std::unordered_map<t_spriteaxis, int>& sizesOut,
	int& gridSizeOut)
{
	gridSizeOut = 1;

	if (strIn.empty())
		return true;

	std::string str = str_trim(strIn);
	if (str.front() == '{' && str.back() == '}')
		str = str.substr(1, str.size() - 2);

	if (str.empty() || str_lowercase(str) == STR_LOWER_NONE)
		return true;

	auto entries = str_split(str, ",");
	for (auto& entry : entries)
	{
		auto strs = str_split(entry, ":");
		if (strs.size() != 2)
		{
			LOG_ERROR(
				"Bad sprite axis \"%s\" formatting, there should be only one colon.",
				entry.c_str());
			return false;
		}

		t_spriteaxis axis;
		if (!sprite_axis_from_str(str_trim(strs[0]), axis))
			return false;

		int size;
		try
		{
			size = std::stoi(str_trim(strs[1]));
		}
		catch (const std::invalid_argument&)
		{
			LOG_ERROR("Bad sprite axis size in \"%s\"", entry.c_str());
			return false;
		}

		axesOut.push_back(axis);
		sizesOut[axis] = size;
		gridSizeOut *= size;

		DEBUG("[SPRITE] parsed axis %d size %d (running grid size %d)",
			(int)axis, size, gridSizeOut);
	}

	return true;
}

/**
 * Parses the optional "sprite_axes_x" / "sprite_axes_y" strings, e.g.:
 * "sprite_axes_x": "{UPGRADE1:3}",
 * "sprite_axes_y": "{UPGRADE0:3}"
 * into a SpriteSheetSystem. Grid width/height are derived as the product
 * of the axis sizes listed in each string, so the sheet dimensions always
 * match the declared axes - no separate grid_width/grid_height needed.
 */
static bool load_sprite_layout(
	const nlohmann::json& jsonBuilding,
	SpriteSheetSystem& layoutOut)
{
	if (!jsonBuilding.contains("sprite_axes_x") &&
		!jsonBuilding.contains("sprite_axes_y"))
		return false;

	std::vector<t_spriteaxis> xAxes, yAxes;
	std::unordered_map<t_spriteaxis, int> axisSizes;
	int gridWidth = 1, gridHeight = 1;

	if (jsonBuilding.contains("sprite_axes_x") &&
		!parse_sprite_axes_str(
			jsonBuilding["sprite_axes_x"].get<std::string>(),
			xAxes, axisSizes, gridWidth))
		return false;

	if (jsonBuilding.contains("sprite_axes_y") &&
		!parse_sprite_axes_str(
			jsonBuilding["sprite_axes_y"].get<std::string>(),
			yAxes, axisSizes, gridHeight))
		return false;

	if (xAxes.empty() && yAxes.empty())
		return false;

	layoutOut = SpriteSheetSystem(xAxes, yAxes, axisSizes, gridWidth, gridHeight);
	
	DEBUG("[SPRITE] load_sprite_layout: grid %dx%d, xAxes=%zu yAxes=%zu",
		gridWidth, gridHeight, xAxes.size(), yAxes.size());

	return true;
}

static std::unordered_map<IVec, t_sprite>
sprite_repr_to_info_entity(
	t_sprite* spriteOut,
	TextureFrames* framesOut,
	char* imgOut,
	AssetManager& assets,
	std::string strImage, // Do a copy
	std::string strSource)
{
	return sprite_repr_to_info(
		spriteOut,
		&framesOut->start,
		&framesOut->end,
		imgOut,
		assets,
		strImage, // Do a copy
		strSource);
}


static std::unordered_map<IVec, t_sprite>
sprite_repr_to_info(
	t_sprite* spriteOut,
	AssetManager& assets,
	std::string strImage, // Do a copy
	std::string strSource,
	const IVec& size = { 1, 1 })
{
	return sprite_repr_to_info(
		spriteOut, nullptr, nullptr, nullptr,
		assets, strImage, strSource, size);
}

/*
�	Id (integer):
�	alignment (Number):
�	hp (integer)
�	type  (text/integer): Enum id of entity type from enums.json (or hardcoded enum).
�	inventory_size (integer):
�	transfer_size
�	target (text/integer) (defualt: BodyType::ALL):
�	blacklist (text/integrr) (default: ��)
�	max_speed (number):
�	max_force (number):
�	attack (number)
�	speed (number):
�	sprite (text)
�	sprites (text)
�	sprites_action
�	sfx_idle
�	sfx_action
�	sfx_hit
�	sfx_death
*/

static t_idpair type_enum_to_id(
	const std::string& str,
	t_enum_tree& mapEnumTree)
{
	bool success = false;
	t_idpair pair = mapEnumTree.get_id(str, &success);
	if (!success)
	{
		WARNING(
			"On enum str to id can't find type: %s",
			str.c_str());
	}
	return pair;
}


static void spawn_json_to_id(
	size_t& serializableIdOut,
	t_idpair& idOut,
	const nlohmann::json& jsonFile,
	t_enum_tree& enumTree,
	std::map<std::string, size_t>& serMap)
{
	std::string spawnStr = jsonFile.at("spawn").get<std::string>();
	const auto strs = str_split(spawnStr, ",");

	auto serItr = serMap.find(str_trim(strs[0]));
	if (serItr == serMap.end())
	{
		WARNING(
			"Spawn contains unknown SERIALIZABLE type \"%s\", check for enums.json for values.",
			str_trim(strs[0]).c_str());
	}
	else
	{
		serializableIdOut = serMap[str_trim(strs[0])];
		idOut = type_enum_to_id(
			str_trim(strs[1]),
			enumTree);
	}
}

static t_idpair enumtree_add_to_id(
	t_group groupId,
	const std::string& keyName,
	t_enum_tree& mapEnumTree)
{
	bool success = false;
	auto& group = mapEnumTree.get_group(groupId, &success);
	if (!success)
	{
		WARNING(
			"Missing group \"%d\" in mapEnumTree.",
			(int)groupId);
		return IDPAIR_NONE;
	}

	t_idpair pair{};
	group.append(keyName, EMPTY_TILETREE);
	pair.id = (int)group.get_id(keyName);
	pair.group = groupId;

	return pair;
}

// Get all properties and uses
template <typename t_bool, typename t_num>
bool decode_props(
	PropertySet<t_bool, t_num>& props,
	t_enum_tree& enumTree,
	const size_t enumPropertyBool,
	const size_t enumPropertyNum,
	const std::string& strBoolsIn = "",
	const std::string& strNumsIn = "")
{
	if (strBoolsIn.size())
	{
		std::string strBools = str_trim(strBoolsIn);
		if (strBools.front() == '{' && strBools.back() == '}')
			strBools = strBools.substr(1, strBools.size() - 2);
		std::vector<std::string> boolsVec = str_split(strBools, ",");

		for (auto& str0 : boolsVec)
		{
			std::string str = format_json_keyname(str0);
			if (enumTree.get_group(enumPropertyBool).has(str))
			{
				props.bool_set(
					(t_bool)
					enumTree.get_group(enumPropertyBool)
					.get_id(str),
					true);
			}
			else
			{
				WARNING("Use \"%s\" don't match any value in enum list", str.c_str());
				return false;
			}
		}
	}

	if (strNumsIn.size())
	{
		std::string strNums = str_trim(strNumsIn);
		if (strNums.front() == '{' && strNums.back() == '}')
			strNums = strNums.substr(1, strNums.size() - 2);
		auto numsVec = str_split(strNums, ",");

		for (auto itr = numsVec.cbegin(); itr != numsVec.cend(); ++itr)
		{
			std::string str = *itr;
			auto strs = str_split(str, ":");

			if (str_lowercase(str) == STR_LOWER_NONE)
			{
				continue;
			}

			if (strs.size() != 2)
			{
				LOG_ERROR("Bad property \"%s\" formatting, there's should be only one colon.",
					str.c_str());
				return false;
			}

			bool b1, b2;
			t_id e = enumTree.get_group(enumPropertyNum, &b1)
				.get_id(str_uppercase(str_trim(strs[0])), &b2);
			if (!b1 && !b2)
			{
				LOG_ERROR("Unknown or unimpemented string to enum value: \"%s\"",
					strs[0].c_str());
				return false;
			}

			double val;
			try
			{
				val = std::strtod(strs[1].c_str(), nullptr);
			}
			catch (const std::invalid_argument& e)
			{
				LOG_ERROR("Can't process double value  \"%f\" in property \"%s\": %s",
					strs[1].c_str(),
					strs[0].c_str(),
					e.what());
				return false;
			}
			catch (const std::out_of_range& e)
			{
				LOG_ERROR("Can't process double value  \"%f\" in property \"%s\": %s",
					strs[1].c_str(),
					strs[0].c_str(),
					e.what());
				return false;
			}
			// ("%s - %d, %d: %f", strs[0].c_str(), (int)e.group, (int)e.id, (float)val);
			props.num_set((t_num)e, val);
		}
	}

	return true;
};

static bool process_generic_stat(
	t_idpair& typePairOut,
	int& idOut,
	t_enum_tree& enumTree,
	const nlohmann::json& jStat,
	const t_group group)
{

	t_idpair typePair{};
	int id = jStat.value("id", -1);
	
	// Find / generate typePair
	if (jStat.contains("key_name"))
	{
		std::string keyName = "";
		keyName = jStat["key_name"]
			.get<std::string>()
			.c_str();
		keyName = str_uppercase(keyName);

		typePair = enumtree_add_to_id(group, keyName, enumTree);
		if (typePair == IDPAIR_NONE)
		{
			LOG_ERROR(
				"t_idpair typePair equals to IDPAIR_NONE. Unable to create stats with key_name %s",
				keyName.c_str());
			assert(0);
			return false;
		}
	}
	else if (jStat.contains("type_str"))
	{
		std::string typeStr = jStat.value("type_str", "BodyType::ALL");
		typePair = type_enum_to_id(
			typeStr,
			enumTree);
		if (typePair == IDPAIR_NONE)
		{
			LOG_ERROR(
				"t_idpair typePair equals to IDPAIR_NONE. Unable to create stats from type_str %s",
				typeStr.c_str());
			assert(0);
			return false;
		}
	}
	else if (jStat.contains("type_id"))
	{
		size_t typeId = jStat.value<size_t>("type_id", 0);
		typePair = {group, typeId};

		if (typePair == IDPAIR_NONE)
		{
			LOG_ERROR(
				"t_idpair typePair equals to IDPAIR_NONE. Unable to create stats from type_id %s",
				std::to_string(typeId).c_str());
			assert(0);
			return false;
		}
	}
	else
	{
		// Stat exists, but can't be accessed via string or id pair
	}

	typePairOut = typePair;
	idOut = id;

	return true;
}

template <class T>
static bool publish_generic_stat(
	t_idpair& typePair,
	int id,
	t_configmap& bodyConfigMap,
	T& stats)
{
	GameBodyConfig* config = dynamic_cast<GameBodyConfig*>(new T(stats));

	if (bodyConfigMap.count(
		typePair.group,
		typePair.id))
	{
		WARNING(
			"Two stats have the same id pair %s, overriding the old one.",
			typePair.to_string().c_str());
		return false;
	}

	bodyConfigMap.insert(
		typePair.group,
		typePair.id,
		{});
	bodyConfigMap.at(typePair.group, typePair.id).insert(
		std::make_pair((t_id)0, config));
	return true;
}

static EntityStats process_entity_stat(
	AssetManager& assets,
	t_enum_tree& enumTree,
	std::map<std::string, size_t>& serMap,
	nlohmann::json& jStats,
	nlohmann::json& jStat,
	size_t loopCounter = 0)
{
	EntityStats stats;

	if (loopCounter >= SMALL_INFINITY)
		return stats;

	t_idpair typePair{};
	int id;
	assert(process_generic_stat(typePair, id, enumTree, jStat, ENUM_ENTITY_TYPE));

	if (jStat.count("parent"))
	{
		int parentId = jStat["parent"].get<int>();
		if (id == parentId)
		{
			WARNING("entity.json parent attribute equals to its own id");
		}
		else if ((size_t)parentId < jStat.size())
		{
			stats = process_entity_stat(
				assets,
				enumTree,
				serMap,
				jStats,
				jStats.at(parentId),
				loopCounter + 1);
		}
		else
		{
			WARNING("entity.json parent attribute leads to an infinie loop");
		}
	}

	// Assign the type and id, if the stats are copied from another stat don't
	// copy it's type id
	if (loopCounter == 0 && typePair != IDPAIR_NONE)
	{
		stats.id = id;
		stats.type = typePair;
		// DEBUG("Set \"type_str\" to %d -> %s", stats.id, typePair.to_string().c_str());
	}
	if (jStat.count("alignment"))
		stats.alignment = alignment_json_to_int(jStat, enumTree);
	if (jStat.count("hp"))
		stats.hp = jStat.value("hp", 0);
	if (jStat.count("radius"))
		stats.radius = jStat.value("radius", 0.0f);

	if (jStat.count("inventory_size"))
		stats.inventorySize = jStat.value("inventory_size", 0);
	if (jStat.count("transfer_size"))
		stats.transferSize = jStat.value("transfer_size", 0);

	if (jStat.count("cooldown_action"))
		stats.cooldownAction = jStat.value("cooldown_action", 0.0f);
	if (jStat.count("cooldown_path"))
		stats.cooldownPath = jStat.value("cooldown_path", 0.0f);

	if (jStat.count("target"))
		stats.target = type_enum_to_id(
			jStat.value("target", "BodyType::NONE"),
			enumTree);

	stats.maxSpeed = jStat.value("max_speed", stats.maxSpeed);
	stats.maxForce = jStat.value("max_force", stats.maxForce);

	// stats.attack = jStat.value("attack", stats.attack);

	if (jStat.count("aggressive"))
		stats.aggressive = jStat.value("aggressive", false);

	if (jStat.count("is_light_source"))
		stats.isLightSource = jStat.value("is_light_source", false);

	std::string img = jStat.value("image", "entity");
	if (jStat.count("frames_walk"))
		sprite_repr_to_info(
			&stats.spriteWalk,
			assets,
			img.c_str(),
			jStat.value("frames_walk", "entity 0,0-0,0"));
	if (jStat.count("frames_action"))
		sprite_repr_to_info(
			&stats.spriteAction,
			assets,
			img.c_str(),
			jStat.value("frames_action", "0,0-0,0"));
	if (jStat.count("frames_hit"))
		sprite_repr_to_info(
			&stats.spriteHit,
			assets,
			img.c_str(),
			jStat.value("frames_hit", "0,0-0,0"));

	if (jStat.contains("spawn"))
	{
		size_t serializableID = 0;
		t_idpair idPair = IDPAIR_NONE;
		spawn_json_to_id(serializableID, idPair, jStat, enumTree, serMap);
		stats.spawnID = serializableID;
		stats.spawnType = idPair;
	}

	std::string propsBoolsStr = "";
	std::string propsNumStr = "";
	if (jStat.contains("bools"))
	{
		try
		{
			propsBoolsStr = jStat.at("bools").get<std::string>();
		}
		catch (const nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
		}
	}
	if (jStat.contains("nums"))
	{
		try
		{
			propsNumStr = jStat["nums"].get<std::string>();
		}
		catch (const nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
		}
	}

	decode_props(
		stats.props,
		enumTree,
		ENUM_ENTITY_PROPERTY_BOOL,
		ENUM_ENTITY_PROPERTY_NUM,
		propsBoolsStr,
		propsNumStr);

	return stats;
}


 bool load_entity_stats(
	t_configmap& bodyConfigMap,
	AssetManager& assets,
	t_enum_tree& enumTree,
	 std::map<std::string, size_t>& serMap,
	nlohmann::json& j)
{

	if (!j.contains("entities"))
	{
		LOG_ERROR("\"entities\" not found in entities.json");
		return false;
	}

	nlohmann::json j0 = j.at("entities");

	try
	{
		for (auto& jStat : j0)
		{

			EntityStats stats;
			stats = process_entity_stat(
				assets, enumTree, serMap, j0, jStat);
			if (stats.type == IDPAIR_NONE)
				continue;

			if (!publish_generic_stat(stats.type, stats.id, bodyConfigMap, stats))
			{
				WARNING("Unable to publish stat %d %s", (int)stats.id, stats.type.to_string().c_str());
				continue;
			}
		}
	}
	catch (nlohmann::detail::type_error& e)
	{
		WARNING("entities json file parsing error %s", e.what());
		return false;
	}

	return true;
}



BulletStats process_bullets_stat(
	AssetManager& assets,
	t_enum_tree& enumTree,
	nlohmann::json& jStats,
	nlohmann::json& jStat,
	size_t loopCounter = 0)
{

	BulletStats stats;

	if (loopCounter >= SMALL_INFINITY)
		return stats;

	t_idpair typePair{};
	int id;
	assert(process_generic_stat(typePair, id, enumTree, jStat, ENUM_ENTITY_TYPE));

	if (jStat.count("parent"))
	{
		int parentId = jStat["parent"].get<int>();
		if (id == parentId)
		{
		}
		else if ((size_t)parentId < jStat.size())
		{
			stats = process_bullets_stat(
				assets,
				enumTree,
				jStats,
				jStats.at(parentId),
				loopCounter + 1);
		}
		else
		{
			WARNING("bullet json file parent attribute leads to an infinie loop!");
		}
	}

	if (loopCounter == 0)
	{
		stats.id = id;
		stats.type = typePair;
	}

	if (jStat.count("alignment"))
		stats.alignment = alignment_json_to_int(jStat, enumTree);
	if (jStat.count("damage"))
		stats.damage = jStat.value("damage", 0.0f);
	if (jStat.count("max_speed"))
		stats.maxSpeed = jStat.value("max_speed", 0.0f);
	if (jStat.count("max_force"))
		stats.maxForce = jStat.value("max_force", 0.0f);
	//if (jStat.count("attack"))
	//	stats.attack = jStat.value("attack", 1.0f);
	if (jStat.count("life_span"))
		stats.lifeSpan = jStat.value("life_span", 0.0f);
	if (jStat.count("max_dist"))
		stats.maxDist = jStat.value("max_dist", 0.0f);
	if (jStat.count("radius"))
		stats.radius = jStat.value("radius", 0.0f);
	if (jStat.count("is_light_source"))
		stats.isLightSource = jStat.value("is_light_source", false);

	sprite_repr_to_info(
		&stats.sprite,
		assets,
		"",
		jStat.value("frames", "bullet 0,0 - 7,0"));

	std::string propsBoolsStr = "";
	std::string propsNumStr = "";
	if (jStat.contains("bools"))
	{
		try
		{
			propsBoolsStr = jStat.at("bools").get<std::string>();
		}
		catch (const nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
		}
	}

	if (jStat.contains("nums"))
	{
		try
		{
			propsNumStr = jStat["nums"].get<std::string>();
		}
		catch (const nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
		}
	}

	decode_props(
		stats.props,
		enumTree,
		ENUM_BULLET_PROPERTY_BOOL,
		ENUM_BULLET_PROPERTY_NUM,
		propsBoolsStr,
		propsNumStr);

	return stats;
}


 bool load_bullets_stats(
	t_configmap& bodyConfigMap,
	AssetManager& assets,
	t_enum_tree& enumTree,
	nlohmann::json& j)
{
	if (!j.contains("bullets"))
	{
		LOG_ERROR("\"bullets\" not found in json file");
		return false;
	}

	nlohmann::json j0 = j.at("bullets");

	try
	{
		for (auto& jStat : j0)
		{
			BulletStats stats;
			stats = process_bullets_stat(
				assets, enumTree, j0, jStat);
			if (stats.type == IDPAIR_NONE)
				continue;

			if (!publish_generic_stat(stats.type, stats.id, bodyConfigMap, stats))
			{
				WARNING(
					"Unable to publish stat %d %s", 
					(int)stats.id, stats.type.to_string().c_str());
				continue;
			}
		}
	}
	catch (nlohmann::detail::type_error& e)
	{
		WARNING("bullets json file parsing error %s", e.what());
		return false;
	}

	return true;
}

static bool process_building_stats(
	UpgradeTree& info,
	t_configmap& bodyConfigMap,
	AssetManager& assets,
	t_enum_tree& enumTree, // For enum-string conversions
	std::map<std::string, size_t>& serMap,
	t_weights* weights,
	const nlohmann::json& jsonBuilding,
	const bool isUpgrade)
{

	int id;
	t_idpair typePair;
	process_generic_stat(
		typePair,
		id,
		enumTree,
		jsonBuilding,
		ENUM_BUILDING_TYPE
	);

	try
	{
		SET_STR(jsonBuilding, info.name, "build_name");
		SET_STR(jsonBuilding, info.description, "description");
		SET_INT(jsonBuilding, info.hp, "hp");
		SET_STR(jsonBuilding, info.image, "image");
		SET_NUM(jsonBuilding, info.delayCost, "delay_cost");
		SET_NUM(jsonBuilding, info.delayAction, "delay_action");

		SET_IVEC(jsonBuilding, info.size, "build_size");

		if (jsonBuilding.count("image_frames"))
		{
			info.spriteHolders = sprite_repr_to_info(
				nullptr,
				&info.framesStart,
				&info.framesEnd,
				info.image,
				assets,
				std::string(info.image),
				jsonBuilding["image_frames"].get<std::string>(),
				info.size);
		}

		info.hasSpriteLayout = load_sprite_layout(jsonBuilding, info.spriteLayout);

		std::string jobStr;
		if (jsonBuilding.contains("job"))
		{
			jsonBuilding.at("job").get_to(jobStr);
			jobStr = format_json_keyname(jobStr);
			if (enumTree.get_group(ENUM_CITIZEN_JOB).has(jobStr))
			{
				auto& group = enumTree.get_group(ENUM_CITIZEN_JOB);
				info.job = (CitizenJob)group.get_id(jobStr);
			}
			else
				WARNING("Use \"%s\" don't match any value in enum list", jobStr.c_str());
		}

		if (jsonBuilding.contains("alignment"))
			info.alignment = alignment_json_to_int(jsonBuilding, enumTree);

		if (jsonBuilding.contains("rdelay"))
		{
			WARNING("rdelay in %s is deprecated", info.name);
			return false;
		}

		info.storageCap = Resources::res_inf(weights);

		SET_INT(jsonBuilding, info.powerIn, "powerin");
		SET_INT(jsonBuilding, info.powerOut, "powerout");
		SET_INT(jsonBuilding, info.powerStore, "powerstore");

		SET_COST(jsonBuilding, info.build, weights, "rcost");
		SET_COST(jsonBuilding, info.input, weights, "rinput");
		SET_COST(jsonBuilding, info.output, weights, "routput");
		SET_COST(jsonBuilding, info.storageCap, weights, "rstore");

		SET_INT(jsonBuilding, info.weightCap, "weight_cap");
		SET_INT(jsonBuilding, info.entityLimit, "entity_limit");
		SET_NUM(jsonBuilding, info.effectRadius, "radius_effect");
		if (jsonBuilding.contains("is_light_source"))
			info.isLightSource = jsonBuilding.value("is_light_source", false);

		SET_INT(jsonBuilding, info.buildWork, "build_work");

		if (jsonBuilding.contains("spawn"))
		{
			std::string spawnStr = jsonBuilding.at("spawn").get<std::string>();
			const auto strs = str_split(spawnStr, ",");

			auto serItr = serMap.find(str_trim(strs[0]));
			if (serItr == serMap.end())
			{
				WARNING(
					"Spawn contains unknown SERIALIZABLE type \"%s\", check for enums.json for values.",
					str_trim(strs[0]).c_str());
			}
			else
			{
				info.spawn.serializableID = serMap[str_trim(strs[0])];
				info.spawn.id = type_enum_to_id(
					str_trim(strs[1]),
					enumTree);
			}
		}
	}
	catch (nlohmann::detail::type_error& e)
	{
		WARNING("data.json parsing error %s", e.what());
		return false;
	}

	std::string propsBoolsStr = "";
	std::string propsNumStr = "";
	std::string propsBoolsRemoveStr = "";
	if (jsonBuilding.contains("uses"))
	{
		try
		{
			propsBoolsStr = jsonBuilding.at("uses").get<std::string>();
		}
		catch (nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
			return false;
		}
	}
	if (jsonBuilding.contains("properties"))
	{
		try
		{
			propsNumStr = jsonBuilding["properties"].get<std::string>();
		}
		catch (nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
			return false;
		}
	}
	if (jsonBuilding.contains("uses_remove"))
	{
		try
		{
			propsBoolsRemoveStr = jsonBuilding.at("uses_remove").get<std::string>();
		}
		catch (nlohmann::detail::type_error &e)
		{
			WARNING("data.json parsing error %s", e.what());
			return false;
		}
	}

	if (typePair != IDPAIR_NONE)
	{
		info.id = id;
		info.type = typePair;
	}

	decode_props(
		info.props,
		enumTree,
		ENUM_PROPERTY_BOOL,
		ENUM_PROPERTY_NUM,
		propsBoolsStr,
		propsNumStr);
	
	decode_props(
		info.propsRemove,
		enumTree,
		ENUM_PROPERTY_BOOL,
		ENUM_PROPERTY_NUM,
		propsBoolsRemoveStr,
		"");

	if (info.powerIn || info.powerOut || info.powerStore)
		info.props.bool_set(PropertyBool::POWER_NETWORK, true);

	if (info.props.bool_is((t_id)PropertyBool::STORAGE) ||
		info.props.bool_is((t_id)PropertyBool::TMP_STORAGE))
		info.props.bool_set(PropertyBool::ANY_STORAGE, true);

	return true;
}

 bool load_upgrade_trees(
	t_configmap& bodyConfigMap,
	AssetManager& assets,
	t_enum_tree& enumTree,
	std::map<std::string, size_t>& serMap,
	t_weights* weights,
	nlohmann::json& jsonData)
{
	using namespace nlohmann;

	int _counter = 0;

	// Start processing the building

	if (!jsonData.contains("buildings"))
		return false;

	json jBuilds = jsonData["buildings"];
	std::string keyName = "";
	for (auto itr = jBuilds.cbegin(); itr != jBuilds.cend(); itr++)
	{
		const json jBuild = *itr;

		UpgradeTree info;
		/*
		try
		{
			if (jBuild.contains("key_name"))
			{
				keyName = jBuild["key_name"]
					.get<std::string>()
					.c_str();
				keyName = str_uppercase(keyName);

				t_enum_tree::Group& ge = enumTree
					.get_group(ENUM_BUILDING_TYPE);
				ge.append(keyName,
					t_tiletree{ VEC_LIMIT, POINT_LIMIT });
				info.id = (int)ge.get_id(keyName);
			}
			else
			{
				if (jBuild.contains("build_id"))
					info.id = jBuild["build_id"].get<int>();

				auto& group = enumTree
					.get_group(ENUM_BUILDING_TYPE);
				if (group.has(info.id))
					keyName = enumTree
					.get_group(ENUM_BUILDING_TYPE)
					.get_str(info.id);
			}

			if (keyName == "")
			{
				ERROR(
					"Missing key name for unrecognized building %d",
					info.id);
				return 0;
			}

			SET_IVEC(jBuild, info.size, "build_size");
		}
		catch (nlohmann::detail::type_error& e)
		{
			WARNING("data.json parsing error %s", e.what());
			return false;
		}*/

		if (!process_building_stats(
			info, bodyConfigMap, assets,
			enumTree, serMap, weights,
			jBuild, false))
		{
			LOG_ERROR("Upgrade parsing error");
			return false;
		}

		if (jBuild.contains("upgrades"))
		{
			json jUpgrades = jBuild["upgrades"];
			std::list<UpgradeTree> upgrades;
			// Add all the upgrades to a list
			for (auto itru = jUpgrades.cbegin();
				itru != jUpgrades.cend();
				itru++)
			{
				const json& jUpgrade = *itru;
				upgrades.push_back({});
				UpgradeTree& info = upgrades.back();
				if (!process_building_stats(
					info, bodyConfigMap, assets,
					enumTree, serMap, weights,
					jUpgrade, true))
				{
					LOG_ERROR("Upgrade parsing error");
					return false;
				}
				try
				{
					info.upgradeId = 
						jUpgrade.value("id_upgrade", info.upgradeId);
					info.upgradeParent = 
						jUpgrade.value("id_parent", info.upgradeParent);
					info.upgradePath = 
						jUpgrade.value("id_path", info.upgradePath);
				}
				catch (nlohmann::detail::type_error& e)
				{
					WARNING("data.json parsing error %s", e.what());
					return false;
				}
			}

			// Convert the list of upgrades to a tree
			// Ugly O(n^2), but the trees are usually
			// small so who cares.]
			while (!upgrades.empty())
			{
				UpgradeTree& t = upgrades.back();
				bool added = false;
				std::list<UpgradeTree*> s;
				s.push_back(&info);

				// Tree search
				while (!s.empty())
				{
					UpgradeTree* back = s.back();
					if (back->upgradeId == t.upgradeParent)
					{
						back->children.push_back(t);
						added = true;
						for (auto k = back->children.begin();
							k != back->children.end();
							k++)
							s.push_back(&*k);
						break;
					}
					s.pop_back();
					for (auto k = back->children.begin();
						k != back->children.end();
						k++)
						s.push_back(&*k);
				}

				if (!added)
				{
					if (_counter++ > SMALL_INFINITY)
					{
						LOG_ERROR("Infinite loop on loading upgrade tree");
						return false;
					}
					upgrades.push_front(t);
				}
				upgrades.pop_back();
			}
		}

		if (!publish_generic_stat(info.type, info.id, bodyConfigMap, info))
		{
			WARNING(
				"Unable to publish stat %d %s",
				(int)info.id, info.type.to_string().c_str());
			continue;
		}

		// keyName

	} // For every building

	return true;
}
