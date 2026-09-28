#pragma once

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef WARNING
#undef DELETE
#undef FALSE
#undef TRUE
#undef min
#undef max
#undef IN
#undef OUT
#define SYSERROR() GetLastError()
#else
#include <errno.h>
#define SYSERROR() errno
#endif

#include <set>
#include <SFML/Graphics.hpp>

#include <TGUI/TGUI.hpp>

#include "app_context.hpp"

#include "utils/utils.hpp"
#include "file/asset_manager.hpp"
#include "render/graphics.hpp"
#include "render/FastLabel.hpp"
#include "window/game_window.hpp"
#include "window/gui_utils.hpp"



static const std::string OUT_PATH = "./";

static const std::unordered_map<std::string, int> TEST_MAP_STR2INT = {
    {"EMPTY",    	0}, // Empty map
    {"MAZE",      	1}, // Pathfinding Test
	{"RANDOM ENTITIES",      2}, // Add random entities
	{"RANDOM", 		3}, 					// Everything's random!
	{"FIGHTING",  	4}, // Check if entities fight each other properly
	{"SYSTEM", 	  	5}, // Simple resource systems
	{"GAME SYSTEM",	6}, // System and builders setup
	{"ELECTRICITY",	7}, // System and builders setup}
	{"ELECTRICTY",  8}, // Electricity system
	{"SCENARIO",  9},
};

static int window_gameplay_test_map_str2int(const std::string& testStr)
{
	int testId = str_to_int(testStr);
	if (testId != -1)
		return testId;
	
	auto itr = TEST_MAP_STR2INT.find(testStr);
	if (itr != TEST_MAP_STR2INT.end())
		return itr->second;
	
	return -1;
}


std::vector<UpgradeTree *>
recursive_get_last_layer_tree(
	UpgradeTree *step,
	const std::vector<int> &upgrades);

// Defined in json_assets.cpp
Resources string_to_cost(const std::string &str, t_weights *weights);

void printBuildQuadTree(ChunkQuadTree<Tile *>::Node *tree);


enum class UiResources
{
	ELECTRICITY,
	PEOPLE,
	ORE,
	GEM,
	BODIES,
	COUNT
};

	struct ResLabelData
	{
		ResLabelData(std::string title = "") : title(title)
		{
		}

		std::string title;
		int lastValue = -1;
		FastLabel::Ptr ptr = nullptr;
	};

struct SaveGameContext
{
	PopupConfirmationData *popupSaveOverride = nullptr;
	PopupConfirmationData *popupLoadGame = nullptr;
	PopupConfirmationData *popupDeleteSave = nullptr;
	std::string scenarioName = "Unknown";
	bool enableSaving = false;
};

struct SelectionGameContext
{
	t_configmap::t_inner::iterator currentTree;
	t_id_config buildSelected{};
	BuildingBase *buildUpgradeSelected = nullptr;

	std::set<sf::Vector2i, IVecCompare> suggestionBuilds;
	std::set<sf::Vector2i, IVecCompare> preFocusBuilds;

	std::vector<UpgradeTree *> upgradeTree;
	std::vector<UpgradeTree *>::const_iterator upgradeTreeItr;
	int suggestionUpgrade = 0;

	// How many of the suggested building's can be afforded
	int affordableCount = -1;
};

struct InteractionGameContext
{
	bool zooming = false;
	bool isBtnPressed = false;

	bool enableDraw = false; // Draw mode, not static

	sf::Vector2i lastPressedTile;

	enum class DrawInsertStates
	{
		INSERT,
		REMOVE
	};

	// Then in building draw mode, remove or insert buildings?
	DrawInsertStates drawInsert = DrawInsertStates::INSERT;
	// Search for nearest entity to the cursor
	bool searchEntity = false;
	// Curser position when searching for entity
	FVec searchEntityPos;
};

using t_dinsert_state = InteractionGameContext::DrawInsertStates;

struct GuiGameContext
{
	sf::Font font;
	std::optional<sf::Text> text;
	CharSpriteSheet charSpriteSheet;

	std::vector<GameBody*> renderQueue;

	std::unordered_map<size_t, BuildingBase *> bInfoPtr;
	std::unordered_map<size_t, tgui::ChildWindow::Ptr> bInfoWin;

	std::unordered_map<size_t, EntityBody *> eInfoPtr;
	std::unordered_map<size_t, tgui::ChildWindow::Ptr> eInfoWin;

	std::unordered_map<UiResources, ResLabelData> labelResources = {
		{UiResources::ELECTRICITY, {"LabelResElctr"}},
		{UiResources::PEOPLE, {"LabelResPpl"}},
		{UiResources::ORE, {"LabelResOre"}},
		{UiResources::GEM, {"LabelResGem"}},
		{UiResources::BODIES, {"LabelResBodies"}}};
};

struct GameGameFlags
{
	bool godMode = false;
	bool enableConstruction = true;
	bool compactJson = false;
	bool paused = false;
};

void depth_sort(
	std::vector<GameBody*>& renderQueue,
	const t_bodies_ctr<GameBody *> &bodies,
	const int rot);

struct WindowGameplay : GameWindow
{

	// Core Systems
	AssetManager assets = AssetManager("assets");
	Chunks chunks = Chunks(CHUNK_W, CHUNK_H);
	GameData data = GameData(&chunks);
	RendererClass renderer;
	GameTimerManager<t_seconds> timerManager;
	GameMode gameMode = GameMode::VIEW;

	// Grouped Contexts
	SaveGameContext saveCtx;
	SelectionGameContext selectCtx;
	InteractionGameContext inputCtx;
	GuiGameContext guiCtx;
	GameGameFlags flags;

	EntityBody *debugFollowEntity = nullptr;

	FastLabel::Ptr labelTimelineCurrent = nullptr;
	FastLabel::Ptr labelTimelineNext = nullptr;

	inline BuildingBase *suggestion_build()
	{
		if (selectCtx.suggestionBuilds.empty())
			return nullptr;
		auto &pos = *selectCtx.suggestionBuilds.begin();
		BuildingBase *base = nullptr;
		base = data.chunks->get_build(pos.x, pos.y)->base;
		return base;
	}

	WindowGameplay();

	~WindowGameplay();

	bool
	init_window(sf::RenderWindow *window, sf::View *view) override;

	bool run_test_generator(int testId);

	bool
	load() override;

	bool
	init() override;

	void close() override;

	void update(float delta) override;

	void
	render() override;

	void
	mouse_start(const sf::Vector2i &mousePos) override;

	void mouse_dragged(
		const sf::Vector2i &mousePos,
		const sf::Vector2i &pmousePos) override;

	void
	mouse_released(const sf::Vector2i &mousePos) override;

	void mouse_pressed(const sf::Vector2i &mousePos) override;

	void
	mouse_zoom_dragged(
		const sf::Vector2i &mousePos,
		const sf::Vector2i &pmousePos) override;

	void mouse_focus_start(const sf::Vector2i &a) override;

	void mouse_focus(const sf::Vector2i &a, const sf::Vector2i &b) override;

	void mouse_focus_end(
		const sf::Vector2i &a,
		const sf::Vector2i &b) override;

	void evict_overlapping_suggestions(
		const sf::Vector2i &pos,
		const IVec &size);

	void keyboard_pressed(sf::Keyboard::Key key) override;

	void keyboard_released(sf::Keyboard::Key key) override;

	void on_focus() override;

	void on_unfocus() override;

	void mouse_zoom(
		const sf::Vector2i &iCenterPos,
		int dist) override;

	void focus_on(const sf::Vector2f &fpos);

	void rotate_world(int dif);

	// Load/Save
	
	bool file_write_jsonpack(
		const std::wstring &fileName,
		const t_jsonpack &,
		bool compact);

	bool jsonpack_to_game(const t_jsonpack &);
	
	bool load_scenario(const std::wstring &name);

	t_jsonpack jsonpack_from_game();

	// Gui

	void popup_confirmation_window(PopupConfirmationData *);

	void gui_init_resource_tab();

	void gui_update_resources_tab(UiResources resources, int value);

	bool gui_change_group(const std::string &groupName);

	bool gui_enable_group(const std::string &groupName);

	bool gui_disable_group(const std::string &groupName);

	bool gui_hide_window(const std::string& windowName);

	bool gui_update_upgrade_info(
		UpgradeTree *info,
		BuildingBase *build);

	bool gui_hide_upgrade_info();

	bool gui_show_upgrade_info();

	bool gui_show_upgrade_info(BuildingBase *build);

	bool gui_update_constr_build(t_id_config *idConfig, tgui::Texture tex);

	bool gui_update_constrs_window();

	void gui_open_saved_games(bool isSave);

	void filter_suggestions();

	void filter_upgrade_buildings(const IVec &latest);

	bool has_build_info(BuildingBase *infoBuild);

	void show_build_info(BuildingBase *infoBuild, bool update = false);

	bool has_entity_info(EntityBody *entity);

	void show_entity_info(EntityBody *entity, bool update = false);

	void grid_change_confirmation(IVec latest);

	void update_upgrade_info();

	// Get the new frames of the upgraded building
	void upgrade_building_get_frames(
		BuildingBase *build,
		UpgradeTree *step,
		IVec &frame0, IVec &frame1);

	// Wrapper for BuildingBody::upgrade() to apply sprite assets
	void upgrade_building(
		BuildingBase *build,
		UpgradeTree *step);
};