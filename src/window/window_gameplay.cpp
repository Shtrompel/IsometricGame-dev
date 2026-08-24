#include "window/window_gameplay.hpp"

#include "game/game_data.hpp"
#include "utils/class/resources.hpp"
#include "window/game_window.hpp"
#include "window/gui_utils.hpp"
#include "window/window_manager.hpp"
#include "file/json_assets.hpp"

#include "game/power_network.hpp"
#include <cstdlib> // wcstombs_s
#include <utility>
#include <execution>

extern t_jsonpack file_read_jsonpack(const std::wstring &fileName, bool compact);
extern std::map<int, std::wstring> list_saved_games();

extern "C"
{
#include "libs/prng.h"
}

std::vector<UpgradeTree *>
recursive_get_last_layer_tree(UpgradeTree *step,
                              const std::vector<int> &upgrades)
{
  std::vector<UpgradeTree *> ret;

  if (!step)
    return ret;

  auto itr = std::find(upgrades.begin(), upgrades.end(), step->upgradeId);

  if (itr == upgrades.end())
  {
  }
  else
  {
    bool hasAny = false;

    auto &childs = step->children;
    for (auto itr = childs.begin(); itr != childs.end(); ++itr)
    {
      UpgradeTree *s = &*itr;

      auto itr2 = std::find(upgrades.begin(), upgrades.end(), s->upgradeId);
      if (itr2 != upgrades.end())
        hasAny = true;
      else
        continue;

      merge_vectors(ret, recursive_get_last_layer_tree(s, upgrades));
    }

    if (!hasAny)
      ret.push_back(step);
  }

  return ret;
}

_UNUSED void printBuildQuadTree(ChunkQuadTree<Tile *>::Node *tree)
{
  using T = ChunkQuadTree<Tile *>;

  T::printNode("Node", tree);
  for (auto &point : tree->points)
  {
    T::printPtrPoint("", point);
    printf("Point %d %d\n", point.pos.x, point.pos.y);
    BuildingBody *b = point.type->building;
    printf("Build type %d in %d %d\n", (int)b->base->buildType, b->tilePos.x,
           b->tilePos.y);
  }

  for (T::Node *node : tree->next)
    if (node)
      printBuildQuadTree(node);
}

void depth_sort(std::vector<GameBody *> &renderQueue,
                const t_bodies_ctr<GameBody *> &bodies, const int rot)
{
  constexpr auto calcDepth = [](const sf::Vector2f &v)
  { return v.x + v.y; };

  renderQueue.clear();

  for (auto &body : bodies)
  {
    if (!body->dead)
      renderQueue.push_back(body);
  }

  std::sort(renderQueue.begin(), renderQueue.end(),
            [rot, calcDepth](GameBody *e1, GameBody *e2)
            {
              // Assuming calcDepth is an inline or fast operation
              float a = calcDepth(vec_90_rotate(e1->pos, -rot));
              float b = calcDepth(vec_90_rotate(e2->pos, -rot));
              return a < b;
            });
}

WindowGameplay::WindowGameplay() {}

WindowGameplay::~WindowGameplay() { LOG("Unconstructing GameWindow"); }

bool WindowGameplay::init_window(sf::RenderWindow *window, sf::View *view)
{
  GameWindow::init_window(window, view);

  LOG("GameWindow initialized");
  renderer = RendererClass(window, view, &assets);
  gui.setTarget(*(dynamic_cast<sf::RenderTarget *>(window)));

  VariantFactory &factory = data.counterContext.variantFactory;

#define REGISTER(a, b) factory.register_variant<a, b>()

  REGISTER(SERIALIZABLE_NETWORK, PowerNetwork);
  REGISTER(SERIALIZABLE_BUILD_BODY, BuildingBody);
  REGISTER(SERIALIZABLE_BUILD_BASE, BuildingBase);
  REGISTER(SERIALIZABLE_ENTITY, EntityCitizen);
  REGISTER(SERIALIZABLE_CITIZEN, EntityCitizen);
  REGISTER(SERIALIZABLE_ENEMY, EntityEnemy);
  REGISTER(SERIALIZABLE_CHUNKS, Chunks);
  REGISTER(SERIALIZABLE_DATA, GameData);
  REGISTER(SERIALIZABLE_BULLET, BulletBody);
  REGISTER(SERIALIZABLE_CONSTRUCTION, Constructions);

  LOG("%d %d", (int)factory.variant_type_to_id<Chunks>(),
      (int)SERIALIZABLE_CHUNKS);

  return true;
}

bool WindowGameplay::load()
{

  // Load resources
  using namespace nlohmann;

  const std::string FILES_NAMES[] = {
      "enums", "consts",
      "entities", "buildings", "resources",
      "textures", "bullets"};
  const size_t FILES_NAMES_COUNT = sizeof(FILES_NAMES) / sizeof(*FILES_NAMES);
  std::map<std::string, nlohmann::json> jsonMap;

  for (size_t i = 0u; i < FILES_NAMES_COUNT; ++i)
  {
    std::string path;
    path = std::string("assets/json/") + FILES_NAMES[i] + ".json";
    if (!json_file_load(jsonMap[FILES_NAMES[i]], path))
      return false;
  }

  try
  {
    for (auto itr = jsonMap["consts"].begin();
         itr != jsonMap["consts"].end();
         ++itr)
    {
      const std::string &key = str_trim(str_uppercase(itr.key()));
      const json &val = itr.value();
      size_t index;
      try
      {
        index = CONSTS_MAP.at(key);
      }
      catch (const std::out_of_range &e)
      {
        WARNING(
            "Constant of string: \"%s\" was not found in CONSTS_MAP: %s",
            key.c_str(), e.what());
        continue;
      }

      if (val.is_number_float())
        data.constFloating[index] = val.get<float>();
      else if (val.is_number_integer())
        data.constNumeric[index] = val.get<int>();
      else if (val.is_boolean())
        data.constBoolean[index] = val.get<bool>();
      else
        continue;
    }
  }
  catch (nlohmann::detail::type_error &e)
  {
    LOG_ERROR("Error while reading from consts.json: %s", e.what());
    return false;
  }

  // Set flags after loading the boolean constants
  flags.godMode = data.constBoolean[(int)ConstantBoolean::GOD_MODE];
  flags.enableConstruction =
      data.constBoolean[(int)ConstantBoolean::ENABLE_CONSTRUCTIONS];

  try
  {
    for (auto &texInfo : jsonMap["resources"]["resources"])
    {
      auto &weights = data.resourceContext.weights;
      if (!texInfo.contains("name") && 0)
        continue;
      int weight = 1, id = -1;
      std::string name = "";
      name = texInfo["name"].get<std::string>();
      if (texInfo.contains("weight"))
        weight = texInfo["weight"].get<int>();
      if (texInfo.contains("id"))
        id = texInfo["id"].get<int>();

      if (id == -1)
      {
        id = (int)weights.next_key();
        weights.add(id, str_uppercase(name), weight);
      }
      else
      {
        id = (int)weights.push_and_move(id,
                                        str_uppercase(name), weight);
      }
    }
  }
  catch (nlohmann::detail::type_error &e)
  {
    LOG_ERROR("Can't load resources from data.json because of a json parsing error: %s",
              e.what());
    return false;
  }
  LOG("Successfuply loaded resources from data.json");

  // Generate null/missing texture
  assets.generate_null_texture();

  // Load texture data
  for (auto &texInfo : jsonMap["textures"]["textures"])
  {
    if (!texInfo.contains("name"))
      continue;

    IVec count = {1, 1};
    IVec divisions = {1, 1};
    if (texInfo.contains("count"))
    {
      auto nums = str_split(texInfo["count"], ",");
      if (nums.size() >= 2)
        count = {
            str_to_int(nums[0]),
            str_to_int(nums[1])};
    }
    if (texInfo.contains("divisions"))
    {
      auto nums = str_split(texInfo["divisions"], ",");
      if (nums.size() >= 2)
        divisions = {
            str_to_int(nums[0]),
            str_to_int(nums[1])};
    }
    else
    {
      divisions = count;
    }

    try
    {
      std::string subpath = "";
      if (texInfo.contains("path") && !texInfo["path"].is_null())
      {
        subpath = texInfo["path"].get<std::string>();
      }

      std::string name = texInfo["name"].get<std::string>();

      // Note the added subpath.c_str() argument here
      assets.load_texture(name.c_str(), subpath.c_str(), count, divisions);
    }
    catch (nlohmann::detail::type_error &e)
    {
      WARNING("Can't load texture because of a json parsing error: %s", e.what());
    }
  }
  LOG("Successfuply load textures");

  std::map<std::string, size_t> mapSerTree;

  if (!load_enum_trees(
          jsonMap["enums"]["enums"],
          data.mapEnumTree))
  {
    ASSERT_ERROR(false, "Can't load upgrade tree.");
    return false;
  }
  LOG("Successfuply load enums");

  if (!load_serializable(
          jsonMap["enums"]["serializables"],
          mapSerTree))
  {
    ASSERT_ERROR(false, "Can't load serializables.");
    return false;
  }
  LOG("Successfuply load serializables");

  if (!load_upgrade_trees(
          data.bodyConfigMap,
          assets,
          data.mapEnumTree,
          mapSerTree,
          &data.resourceContext.weights,
          jsonMap["buildings"]))
  {
    ASSERT_ERROR(false, "Can't load upgrade tree.");
    return false;
  }
  LOG("Successfuply load buildings");

  if (!load_entity_stats(
          data.bodyConfigMap,
          assets,
          data.mapEnumTree,
          mapSerTree,
          jsonMap["entities"]))
  {
    ASSERT_ERROR(false, "Can't load entity stats.");
    return false;
  }
  LOG("Successfuply load entities");

  if (!load_bullets_stats(
          data.bodyConfigMap,
          assets,
          data.mapEnumTree,
          jsonMap["bullets"]))
  {
    ASSERT_ERROR(false, "Can't load bullets stats.");
    return false;
  }
  LOG("Successfully load bullets");

  // for (auto& x : data.bodyConfigMap.at(ENUM_BULLET_TYPE))
  //{
  //	DEBUG("%s", x.second.at(0)->to_string().c_str());
  // }

  renderer.spriteGrass =
      assets.load_sprites("tile", {0, 0}, {0, 0});
  renderer.spriteTileSelected =
      assets.load_sprites("tile", {1, 0}, {1, 0});
  renderer.spriteEntity =
      assets.load_sprites("entity", {0, 0}, {1, 0});
  renderer.spriteEnemy =
      assets.load_sprites("enemy", {0, 0}, {1, 0});
  renderer.spriteConstruction =
      assets.load_sprites("other", {0, 0}, {0, 0});
  renderer.spriteIconDelete =
      assets.load_sprites("other", {1, 0}, {1, 0});
  renderer.spriteIconBuild =
      assets.load_sprites("other", {2, 0}, {2, 0});
  renderer.spriteTmpBullet =
      assets.load_sprites("bullet", {0, 0}, {7, 0});

  if (!guiCtx.font.openFromFile("assets/OpenSans-Regular.ttf"))
  {
    LOG_ERROR("Missing file OpenSans-Regular.ttf");
  }
  else
    LOG("File OpenSans-Regular.ttf loaded aucceafully");

  {
    sf::Font font2;
    if (!font2.openFromFile("assets/fonts/bankgothic-regular.ttf"))
    {
      LOG_ERROR("Unable to load font bankgothic-regular.ttf");
      assert(0);
    }

    sf::Text text{font2};
    text.setCharacterSize(30);
    text.setFillColor(sf::Color::Black);

    guiCtx.charSpriteSheet.init(font2, text);
  }

  guiCtx.font = guiCtx.font;

  return true;
}

bool WindowGameplay::run_test_generator(int testId)
{
  Chunks *chunksPtr = &chunks;
  const auto generateChunks = [&chunksPtr](int radius)
  {
    // Initialize grid chunks
    int areaWidth = radius * 2 + 1;
    for (int i = 0; i < areaWidth * areaWidth; ++i)
    {
      Grid *grid = new Grid(CHUNK_W, CHUNK_H, i % areaWidth - areaWidth / 2,
                            i / areaWidth - areaWidth / 2);
      chunksPtr->add(grid);
    }
  };

  const auto autoAddResources = [this](int ores = 20, int gems = 6, int radius = 32)
  {
    BuildingBase *b = nullptr;
    Logger::set_priority(0);

    for (int i = 0; i < ores + gems; ++i)
    {
      sf::Vector2i pos;
      pos.x = (int)(radius * (prng_get_double() * 2 - 1));
      pos.y = (int)(radius * (prng_get_double() * 2 - 1));

      int maxCount = 50, minCount = 10;
      BuildingType buildType = BuildingType::RAW_ORE;
      int resourceId = Resources::ORE;

      if (i >= ores)
      {
        minCount = 3;
        maxCount = 15;
        resourceId = Resources::GEMS;
        buildType = BuildingType::RAW_GEMS;
      }

      b = data.add_building(pos, buildType, 0);
      if (!b)
        continue;

      b->rStorage[resourceId] = (int)math_floor(
          prng_get_double() * (maxCount - minCount + 1) + minCount);
    }
    Logger::set_priority(99);
  };

  LOG("\tTest start");

  this->saveCtx.enableSaving = true;

  // Map test IDs directly to their execution lambdas
  std::unordered_map<int, std::function<void(void)>> tests;

  // --- 0: TEST_MAZE ---

  tests[0] = [this, &generateChunks]() {

  };

  tests[1] = [this, &generateChunks]()
  {
    generateChunks(3);
    sf::Vector2i end = {7, 17};
    sf::Image mazeImg;
    assert(mazeImg.loadFromFile("assets/maze.png"));
    Logger::set_priority(0);
    BuildingBase *b;
    LOOP(i, mazeImg.getSize().x)
    {
      LOOP(j, mazeImg.getSize().y)
      {
        sf::Color c = mazeImg.getPixel({(unsigned)i, (unsigned)j});
        sf::Vector2i p = sf::Vector2i{(int)i, (int)j};
        switch (c.toInteger())
        {
        case 0x000000FF:
          data.add_building(p, BuildingType::WALL, false);
          break;
        case 0x0000FFFF:
          b = data.add_building(p, BuildingType::HOME, false);
          if (b)
          {
            b->entityLimit = 12;
            b->actionTimer->set_length(0.1f);
          }
          break;
        case 0xFF0000FF:
          b = data.add_building(p, BuildingType::GENERATOR, false);
          if (b)
            b->entityLimit = 12;
          break;
        case 0xFFFFFFFF:
          end = p;
          if (data.add_building(p, BuildingType::ROAD, false))
          {
          }
          break;
        default:
          printf("%s %d %x\n", FILE_NAME, __LINE__, c.toInteger());
          break;
        }
      }
    }
    Logger::set_priority(10);
  };

  // --- 2: RANDOM ENTITIES ---
  // Drops a chaotic mix of citizens and brutes in an open field
  tests[2] = [this, &generateChunks]()
  {
    generateChunks(3);

    for (int i = 0; i < 40; ++i)
    {
      float x = (float)(prng_get_double() * 24.f - 12.f);
      float y = (float)(prng_get_double() * 24.f - 12.f);

      // 25% chance to spawn an enemy, 75% for a citizen
      if (i % 4 == 0)
      {
        auto sEnemy = data.get_entity_stats(ENUM_ENEMY_TYPE, (t_id)EnemyType::BRUTE);
        data.add_entity_enemy({x, y}, sEnemy);
      }
      else
      {
        auto sCitizen = data.get_entity_stats(ENUM_CITIZEN_JOB, (t_id)CitizenJob::NONE);
        data.add_entity_citizen({x, y}, sCitizen);
      }
    }
    focus_on({0.f, 0.f});
  };

  // --- 3: RANDOM ---
  // Fully chaotic map with random resources, buildings, and entities
  tests[3] = [this, &generateChunks, &autoAddResources]()
  {
    generateChunks(4);
    autoAddResources(40, 15, 20); // High resource density

    for (int i = 0; i < 25; ++i)
    {
      int x = (int)(prng_get_double() * 24 - 12);
      int y = (int)(prng_get_double() * 24 - 12);

      // Pick a random building type, avoiding EMPTY (0) and LAST
      int rawType = (int)(prng_get_double() * ((int)BuildingType::LAST - 1)) + 1;
      BuildingType type = static_cast<BuildingType>(rawType);

      data.add_building({x, y}, type, false);
    }

    for (int i = 0; i < 20; ++i)
    {
      float x = (float)(prng_get_double() * 24.f - 12.f);
      float y = (float)(prng_get_double() * 24.f - 12.f);
      auto sCitizen = data.get_entity_stats(ENUM_CITIZEN_JOB, (t_id)CitizenJob::NONE);
      data.add_entity_citizen({x, y}, sCitizen);
    }
    focus_on({0.f, 0.f});
  };

  // --- 4: FIGHTING ---
  // Sets up a functional military outpost besieged by an enemy portal
  tests[4] = [this, &generateChunks]()
  {
    generateChunks(3);

    // Friendly Outpost
    data.add_building({-3, 0}, BuildingType::HOME, false);
    data.add_building({-1, 0}, BuildingType::ARMORY, false); // Trains soldiers
    data.add_building({-1, -2}, BuildingType::TOWER, false); // Watcher support
    data.add_building({-1, 2}, BuildingType::TOWER, false);

    // The Threat
    BuildingBase *spawn = data.add_building({8, 0}, BuildingType::ENEMY_SPAWN, false);
    if (spawn)
    {
      spawn->entityLimit = 8;
    }

    // Pre-spawn some enemies to instigate immediate conflict
    for (int i = -2; i <= 2; ++i)
    {
      auto sEnemy = data.get_entity_stats(ENUM_ENEMY_TYPE, (t_id)EnemyType::BRUTE);
      data.add_entity_enemy({7.f, (float)i}, sEnemy);
    }

    focus_on({2.f, 0.f});
  };

  // --- 5: SYSTEM ---
  // A complete, self-sustaining resource loop (Mine -> Storage -> Logistics)
  tests[5] = [this, &generateChunks, &autoAddResources]()
  {
    generateChunks(3);
    autoAddResources(20, 5, 12);

    data.add_building({0, 0}, BuildingType::MINERS_POST, false);
    data.add_building({-2, 0}, BuildingType::HOME, false); // Miners

    data.add_building({2, 0}, BuildingType::STORAGE, false);

    data.add_building({0, 3}, BuildingType::LOGISTICS_CENTER, false);
    data.add_building({-2, 3}, BuildingType::HOME, false); // Couriers

    focus_on({0.f, 1.f});
  };

  // --- 6: GAME SYSTEM ---
  // Bootstraps a logistics loop alongside a Construction Department
  tests[6] = [this, &generateChunks, &autoAddResources]()
  {
    generateChunks(3);
    autoAddResources(20, 5, 10);

    // Basic resource loop
    data.add_building({0, 0}, BuildingType::MINERS_POST, false);
    data.add_building({0, -2}, BuildingType::HOME, false);
    data.add_building({2, 0}, BuildingType::STORAGE, false);

    // Construction setup
    data.add_building({4, 0}, BuildingType::CONSTRUCTION_DEPARTMENT, false);
    data.add_building({4, -2}, BuildingType::HOME, false); // Builders

    focus_on({2.f, -1.f});
  };

  // --- 7: ELECTRICITY ---
  // Tests power generation, storage routing, and consumption via upgrades
  tests[7] = [this, &generateChunks]()
  {
    generateChunks(2);

    // Power generation
    data.add_building({0, 0}, BuildingType::GENERATOR, false);
    data.add_building({-12, 0}, BuildingType::HOME, false); // Electricians

    // Power routing and storage
    data.add_building({2, 0}, BuildingType::CAPACITOR, false);

    // Consumer (A home manually forced into the 'Well Off' upgrade to draw power)
    BuildingBase *poweredHome = data.add_building({4, 0}, BuildingType::HOME, false);
    if (poweredHome && poweredHome->get_upgrades().size() > 1)
    {
      // Index 1 corresponds to "Well Off Shack" which demands powerIn

      UpgradeTree *ut = poweredHome->get_upgrades()[1];
      DEBUG("%s", ut->to_string(&data, false).c_str());

      upgrade_building(poweredHome, poweredHome->get_upgrades()[1]);
    }

    focus_on({2.f, 0.f});
  };

  // Run the specified test
  if (tests.count(testId))
  {
    tests[testId]();
  }
  else
  {
    LOG_ERROR("Test ID %d is invalid or not implemented.", testId);
  }

  LOG("\tTest end");
  return true;
}

struct SaveOverridePopupData : public PopupConfirmationData
{
  WindowGameplay *gameWindow;
  bool isSave;
  std::wstring fileName;
  std::function<void(const std::wstring &)> onLoad;
  std::function<void(const std::wstring &)> onDelete;
  std::function<void(const std::wstring &)> onSave;

  SaveOverridePopupData(WindowGameplay *gw, bool save, const std::wstring &fName,
                        std::function<void(const std::wstring &)> loadCb,
                        std::function<void(const std::wstring &)> deleteCb,
                        std::function<void(const std::wstring &)> saveCb)
      : PopupConfirmationData("Override file?", "Are you sure you want to override this save? Old save will be lost."),
        gameWindow(gw), isSave(save), fileName(fName), onLoad(loadCb), onDelete(deleteCb), onSave(saveCb)
  {
  }

  void onConfirmation() override
  {
    t_jsonpack jsonPack = gameWindow->jsonpack_from_game();
    gameWindow->file_write_jsonpack(fileName, jsonPack, gameWindow->flags.compactJson);
    guiutils_save_files(gameWindow->gui, isSave, gameWindow->flags.compactJson, true, onLoad, onDelete, onSave);
  }
};

struct LoadGamePopupData : public PopupConfirmationData
{
  WindowGameplay *gameWindow;
  std::wstring fileName;
  LoadGamePopupData(WindowGameplay *gw, const std::wstring &fName)
      : PopupConfirmationData("Load game?", "Are you sure you want to load this save? Current progress will be lost."), gameWindow(gw), fileName(fName) {}
  void onConfirmation() override
  {
    t_jsonpack jsonPack = file_read_jsonpack(fileName, gameWindow->flags.compactJson);
    gameWindow->jsonpack_to_game(jsonPack);
  }
};

struct DeleteSavePopupData : public PopupConfirmationData
{
  WindowGameplay *gameWindow;
  bool isSave;
  std::wstring fileName;
  std::function<void(const std::wstring &)> onLoad;
  std::function<void(const std::wstring &)> onDelete;
  std::function<void(const std::wstring &)> onSave;

  DeleteSavePopupData(WindowGameplay *gw, bool save, const std::wstring &fName,
                      std::function<void(const std::wstring &)> loadCb,
                      std::function<void(const std::wstring &)> deleteCb,
                      std::function<void(const std::wstring &)> saveCb)
      : PopupConfirmationData("Delete file?", "Are you sure you want to delete this save? This save will be lost."),
        gameWindow(gw), isSave(save), fileName(fName), onLoad(loadCb), onDelete(deleteCb), onSave(saveCb) {}

  void onConfirmation() override
  {
    file_delete(fileName);
    guiutils_save_files(gameWindow->gui, isSave, gameWindow->flags.compactJson, true, onLoad, onDelete, onSave);
  }
};

bool WindowGameplay::init()
{

  LOG("\tInit start");

  // TODO
  // guiCtx.text.setFont(guiCtx.font);
  // guiCtx.text.setCharacterSize(16);
  // guiCtx.text.setFillColor(sf::Color::Black);

  LOG("Gui start");

  std::string guiFormPath =
      std::filesystem::current_path().string() + "/assets/gui/FormGame.txt";
  try
  {
    gui.loadWidgetsFromFile(guiFormPath);
  }
  catch (std::exception const &e)
  {
    LOG_ERROR("Unable to call loadWidgetsFromFile. "
              "Error: %s",
              e.what());
    return false;
  }

  float scaleFactor = managerParent->get_settings().uiScale;
  gui.setRelativeView(tgui::FloatRect{0.f, 0.f, 1.f / scaleFactor, 1.f / scaleFactor});

  // float scaleFactor = 2.0f; // 200% scaling
  // gui.setRelativeView({0.f, 0.f, 1.f / scaleFactor, 1.f / scaleFactor});

  get_widget<tgui::Button>("ButtonBuild")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            // gameWindow->gui_change_group("GroupBuild");
            tgui::ChildWindow::Ptr w1;
            w1 = gameWindow->gui.get<tgui::ChildWindow>("WindowConstructions");
            assert(w1);
            w1->setVisible(!w1->isVisible());
            gameWindow->gameMode = GameMode::BUILD;
          },
          this);
  get_widget<tgui::Button>("ButtonRemove")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->gui_change_group("GroupBuild");
            gameWindow->gameMode = GameMode::DELETE;
          },
          this);
  get_widget<tgui::Button>("ButtonUpgrade")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->gui_change_group("GroupBuild");
            gameWindow->gameMode = GameMode::UPGRADE;
          },
          this);
  get_widget<tgui::Button>("ButtonBuildCancel")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            // for (const tgui::Widget::Ptr& widget :
            // gameWindow->gui.getWidgets())
            {
              gameWindow->gui_change_group("GroupMain");
              gameWindow->gameMode = GameMode::VIEW;
              gameWindow->selectCtx.suggestionBuilds.clear();
              gameWindow->inputCtx.enableDraw = false;
              gameWindow->gui_hide_upgrade_info();
            }
          },
          this);

  auto openSaves = [](WindowGameplay *gameWindow, bool isSave)
  {
    auto onLoad = [gameWindow](const std::wstring &fileName)
    {
      guiutils_popup_confirmation_window(gameWindow->gui, new LoadGamePopupData(gameWindow, fileName));
    };

    auto onSavePtr = std::make_shared<std::function<void(const std::wstring &)>>();
    auto onDeletePtr = std::make_shared<std::function<void(const std::wstring &)>>();

    *onSavePtr = [gameWindow, isSave, onLoad, onSavePtr, onDeletePtr](const std::wstring &fileName)
    {
      auto savedGames = list_saved_games();
      bool exists = false;
      for (auto &pair : savedGames)
      {
        if (pair.second == fileName)
          exists = true;
      }

      if (exists)
      {
        guiutils_popup_confirmation_window(gameWindow->gui, new SaveOverridePopupData(gameWindow, isSave, fileName, onLoad, *onDeletePtr, *onSavePtr));
      }
      else
      {
        t_jsonpack jsonPack = gameWindow->jsonpack_from_game();
        gameWindow->file_write_jsonpack(fileName, jsonPack, gameWindow->flags.compactJson);
        guiutils_save_files(gameWindow->gui, isSave, gameWindow->flags.compactJson, true, onLoad, *onDeletePtr, *onSavePtr);
      }
    };

    *onDeletePtr = [gameWindow, isSave, onLoad, onSavePtr, onDeletePtr](const std::wstring &fileName)
    {
      guiutils_popup_confirmation_window(gameWindow->gui, new DeleteSavePopupData(gameWindow, isSave, fileName, onLoad, *onDeletePtr, *onSavePtr));
    };

    guiutils_save_files(gameWindow->gui, isSave, gameWindow->flags.compactJson, true, onLoad, *onDeletePtr, *onSavePtr);
  };

  get_widget<tgui::Button>("ButtonMenuSaves")
      ->onPress(
          [openSaves](WindowGameplay *gameWindow)
          {
            openSaves(gameWindow, true);
          },
          this);

  get_widget<tgui::Button>("ButtonContinue")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->get_widget<tgui::ChildWindow>("WindowMenu")
                ->setVisible(false);
          },
          this);
  get_widget<tgui::Button>("ButtonMenu")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            std::shared_ptr<tgui::Widget> childWindow = nullptr;
            childWindow =
                gameWindow->get_widget<tgui::ChildWindow>("WindowMenu");
            childWindow->setVisible(!childWindow->isVisible());
          },
          this);

  get_widget<tgui::ChildWindow>("WindowMenu")
      ->onClosing(
          [](WindowGameplay *gameWindow, bool *abort)
          {
            gameWindow->get_widget<tgui::ChildWindow>("WindowMenu")
                ->setVisible(false);
            *abort = true;
          },
          this);

  get_widget<tgui::Button>("ButtonSettings")
      ->onPress(
          [](WindowGameplay *gameWindow, WindowManager *manager)
          {
            guiutils_settings(manager, gameWindow->gui);
          },
          this, managerParent);

  get_widget<tgui::Button>("ButtonExit")
      ->onPress(
          [](WindowGameplay *gameWindow, WindowManager *manager)
          {
            GameWindow *window = manager->get_window_from_id("MENU");
            if (window)
              manager->change_window(window);
          },
          this, managerParent);
  get_widget<tgui::Button>("ButtonRotate")
      ->onPress([](WindowGameplay *gameWindow)
                { gameWindow->rotate_world(1); },
                this);
  get_widget<tgui::Button>("ButtonUpgrade")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->gameMode = GameMode::UPGRADE;
          },
          this);
  get_widget<tgui::Button>("ButtonConstructionBack")
      ->onClick(
          [](tgui::Gui &gui)
          {
            tgui::ChildWindow::Ptr w1, w2;
            w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
            w2 = gui.get<tgui::ChildWindow>("WindowConstruction");
            if (w1 && w2)
            {
              w1->setVisible(true);
              w2->setVisible(false);
            }
            else
              assert(0);
          },
          std::ref(gui));
  gui.get<tgui::ChildWindow>("WindowConstructions")
      ->onClosing(
          [](tgui::Gui &gui, bool *abort)
          {
            tgui::ChildWindow::Ptr w1;
            w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
            w1->setVisible(false);
            *abort = true;
          },
          std::ref(gui));
  gui.get<tgui::ChildWindow>("WindowConstruction")
      ->onClosing(
          [](tgui::Gui &gui, bool *abort)
          {
            tgui::ChildWindow::Ptr w1;
            w1 = gui.get<tgui::ChildWindow>("WindowConstruction");
            w1->setVisible(false);
            *abort = true;
          },
          std::ref(gui));

  get_widget<tgui::Button>("ButtonPaint")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->inputCtx.enableDraw = !gameWindow->inputCtx.enableDraw;
          },
          this);
  get_widget<tgui::Button>("ButtonGridCancel")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->gui_disable_group("GroupGridBuild");
            gameWindow->selectCtx.suggestionBuilds.clear();
            gameWindow->gui_hide_upgrade_info();
          },
          this);

  get_widget<tgui::Button>("ButtonGridApply")
      ->onPress(
          [](WindowGameplay *gameWindow)
          {
            gameWindow->gui_disable_group("GroupGridBuild");

            SelectionGameContext *selection = &gameWindow->selectCtx;

            switch (gameWindow->gameMode)
            {
            case GameMode::BUILD:
            {
              UpgradeTree *tree = dynamic_cast<UpgradeTree *>(
                  gameWindow->selectCtx.buildSelected[0]);
              assert(tree);
              for (auto &tp : gameWindow->selectCtx.suggestionBuilds)
                gameWindow->data.queue_add_build(
                    tp, tree, gameWindow->flags.enableConstruction,
                    !gameWindow->flags.godMode);
              gameWindow->selectCtx.suggestionBuilds.clear();
              break;
            }
            case GameMode::DELETE:
            {
              std::vector<BuildingBase *> builds;
              for (auto &tp : gameWindow->selectCtx.suggestionBuilds)
              {
                BuildingBody *body =
                    gameWindow->data.chunks->get_build(tp.x, tp.y);
                if (body && std::find(builds.begin(), builds.end(),
                                      body->base) == builds.end())
                {
                  builds.push_back(body->base);
                  body->base->flagDelete = true;
                }
              }
              selection->suggestionBuilds.clear();
            }
            break;
            case GameMode::UPGRADE:
            {
              if (selection->upgradeTree.empty())
                break;
              std::vector<BuildingBase *> builds;
              UpgradeTree *tree = *selection->upgradeTreeItr;
              assert(tree);
              for (auto &tp : selection->suggestionBuilds)
              {
                BuildingBody *body =
                    gameWindow->data.chunks->get_build(tp.x, tp.y);
                if (body && std::find(builds.begin(), builds.end(),
                                      body->base) == builds.end())
                {
                  builds.push_back(body->base);
                  gameWindow->upgrade_building(body->base, tree);
                }
              }
              selection->suggestionBuilds.clear();
            }
            break;
            default:
              selection->suggestionBuilds.clear();
              break;
            }
          },
          this);

  gui_init_resource_tab();
  gui_update_constrs_window();

  LOG("Gui end");

  unsigned randSeed = 0;
  prng_seed_bytes((const unsigned char *)&randSeed, sizeof(randSeed));

  list_saved_games();

  LOG("\tInit end");

  return true;
}

void WindowGameplay::close()
{
  LOG("\tClose start");
  LOG("\tClose end");
}

void WindowGameplay::update(float delta)
{
  // TODO account add_build
  // bool gridChange = false;

  t_weights &weights = data.resourceContext.weights;

  t_seconds start = data.get_time();
  static t_seconds max = 0.0f;

  // Remove buildings that are marked for deletion
  auto &buildings = data.bodiesContext.buildingBases;
  AppendingChangesContext &changesContext = data.changesContext;
  ResourceContext &resourceContext = data.resourceContext;

  // Add all building that pending to be added
  while (changesContext.buildingQueue.size())
  {
    BuildingQueueData bqd;
    bqd = std::move(changesContext.buildingQueue.front());
    DEBUG("Changes Contex: %s %s %s", bqd.step->name, bqd.isConstruction ? "True" : "False", bqd.useResources ? "True" : "False");
    data.add_building(std::move(bqd));
    changesContext.buildingQueue.pop_front();
  }

  // Calculate total power
  resourceContext.powerTotal = 0;
  for (auto &network : resourceContext.networks)
  {
    network->powerValue = math_min(network->powerStore, network->powerOut);
    network->storeCount = network->powerValue;
    resourceContext.powerTotal += network->storeCount;
  }

  resourceContext.resources = Resources::res_empty(&weights);
  resourceContext.power = 0;

  auto itr = buildings.begin();
  while (itr != buildings.end())
  {
    bool removed = false;
    BuildingBase *b = (*itr);
    assert(b);
    b->update();

    // Calculate power networks power values
    if (b->props.bool_is(PropertyBool::POWER_NETWORK) && b->network)
    {
      if (b->network)
      {
        auto &network = *b->network;
        if (b->powerIn)
        {
          network.powerValue -= b->powerIn;
          b->sufficient = network.powerValue >= 0;
        }

        if (b->powerStore)
        {
          b->powerValue = math_min(network.storeCount, b->powerStore);
          network.storeCount -= b->powerValue;
          b->powerValue = math_max(0, b->powerValue);

          resourceContext.power += b->powerValue;
        }
      }
      else
      {
        // Building is not connected to a network, meaning it's depowered
        b->sufficient = false;
      }
    }

    // Update info window
    if (b->updateInfo)
    {
      b->updateInfo = false;
      if (has_build_info(b))
      {
        show_build_info(b, true);
      }
    }

    // Calculate all storage
    if (b->is_any_storage() && !b->props.bool_is(PropertyBool::HARVESTABLE))
    {
      resourceContext.resources += b->rStorage;
    }

    // Delete buildings that are flagged to be deleted
    // Must be last
    if (b->flagDelete)
    {
      // Close building UI if open
      if (has_build_info(b))
      {
        show_build_info(b, false);
      }
      if (selectCtx.buildUpgradeSelected == b)
      {
        gui_hide_upgrade_info();
        selectCtx.buildUpgradeSelected = nullptr;
      }
      if (data.infoBuild == b)
      {
        data.infoBuild = nullptr;
      }

      itr = data.delete_building(b);
      removed = true;
    }

    gui_update_resources_tab(UiResources::BODIES,
                             resourceContext.resources[Resources::BODIES]);
    gui_update_resources_tab(UiResources::ORE,
                             resourceContext.resources[Resources::ORE]);
    gui_update_resources_tab(UiResources::GEM,
                             resourceContext.resources[Resources::GEMS]);
    gui_update_resources_tab(UiResources::ELECTRICITY,
                             resourceContext.powerTotal);
    gui_update_resources_tab(UiResources::PEOPLE,
                             (int)data.bodiesContext.entityCitizens.size());

    // If not removed, go the the next building
    if (!removed)
      itr++;
  }

  size_t removedEntities = 0u;
  for (auto itr = data.bodiesContext.bodies.begin();
       itr != data.bodiesContext.bodies.end();)
  {
    GameBody *body = *itr;
    assert(body);

    if (body->dead)
    {
      for (auto &x : data.changesContext.bodyQueue)
      {
        if (x.target == body)
          x.target = nullptr;
      }

      if (body->type == BodyType::ENTITY)
      {
        EntityBody *eb = dynamic_cast<EntityBody *>(body);

        // Close entity UI
        if (has_entity_info(eb))
        {
          show_entity_info(eb, false);
        }
        if (data.infoEntity == eb)
        {
          data.infoEntity = nullptr;
        }

        data.delete_entity(eb);
      }

      data.delete_game_body_generic(body);
      itr = data.bodiesContext.bodies.erase(itr);

      continue;
    }
    else
      ++itr;

    // Movement
    body->update(delta);

    if (body->type == BodyType::ENTITY)
    {
      EntityBody *entity = dynamic_cast<EntityBody *>(body);
      if (entity->updateInfo)
      {
        entity->updateInfo = false;
        if (has_entity_info(entity))
        {
          show_entity_info(entity, true);
        }
      }
    }
  }

  while (!data.changesContext.entityQueue.empty())
  {
    BuildingBase *build = data.changesContext.entityQueue.front();
    data.add_entity_citizen(build);
    data.changesContext.entityQueue.pop_front();
  }

  while (!data.changesContext.bodyQueue.empty())
  {
    const BodyQueueData bodyData = data.changesContext.bodyQueue.back();
    data.changesContext.bodyQueue.pop_back();

    const SpawnInfo &spawn = bodyData.spawn;

    bool isEntity = false;
    // Is the spawn info spawns an entity
    if (bodyData.spawn.id.group == ENUM_CITIZEN_JOB ||
        bodyData.spawn.id.group == ENUM_ENEMY_TYPE ||
        (bodyData.spawn.id.group == ENUM_BODY_TYPE &&
         bodyData.spawn.id.id == (t_id)BodyType::ENTITY))
    {
      isEntity = true;
    }

    if (isEntity && bodyData.home)
    {
      if (bodyData.home->storedEntities.size() >= bodyData.home->entityLimit)
        continue;
    }

    if (bodyData.home &&
        bodyData.home->entities.size() >= bodyData.home->entityLimit)
    {
      continue;
    }

    GameBody *body;
    body = data.add_game_body(spawn.serializableID, spawn.id.group, spawn.id.id,
                              bodyData.pos);

    assert(body);
    if (!body)
    {
      WARNING("Body insertion failed! %s - %s", spawn.id.to_string().c_str(),
              (int)spawn.serializableID);
    }

    if (isEntity)
    {
      assert(body->type == BodyType::ENTITY);
      EntityBody *entity = dynamic_cast<EntityBody *>(body);
      assert(entity);

      if (entity->entityType == EntityType::CITIZEN)
      {
        EntityCitizen *citizen = dynamic_cast<EntityCitizen *>(entity);
        citizen->home = bodyData.home;
      }

      if (bodyData.home)
      {
        bodyData.home->entities.push_back(entity);
        bodyData.home->updateInfo = true;
      }
    }

    if (bodyData.target)
    {
      assert(body != dynamic_cast<GameBody *>(bodyData.target));
      body->set_target(bodyData.target);
    }

    body->vel = bodyData.vel;
    body->alignment = bodyData.alignment;
  }

  data.changesContext.addedTiles.clear();
  data.changesContext.removedTiles.clear();
  float dif = data.get_time() - start;
  max = std::max(dif, max);

  ++data.frameCount;
}

void WindowGameplay::render()
{
  depth_sort(guiCtx.renderQueue, data.bodiesContext.bodies,
             renderer.orientation.rotation);

  renderer.render_grid(&chunks);

  // Render influence area of selected buildings
  renderer.render_influence_area(guiCtx.bInfoPtr);

  GameBody *selectedBody = nullptr;

  if ((selectedBody = renderer.render_objects(guiCtx.renderQueue,
                                              // data.bodiesContext.bodies,
                                              inputCtx.searchEntity,
                                              inputCtx.searchEntityPos)))
  {
    switch (selectedBody->type)
    {
    case BodyType::ENTITY:
    {
      EntityBody *entity = dynamic_cast<EntityBody *>(selectedBody);
      show_entity_info(entity);
      break;
    }
    case BodyType::BUILDING:
    {
      BuildingBody *b = dynamic_cast<BuildingBody *>(selectedBody);
      this->data.infoBuild = b->base;
      show_build_info(b->base);
      break;
    }
    default:
      AAAA;
    }
  }

  inputCtx.searchEntity = false;

  // If a building aws selected
  if (selectCtx.suggestionBuilds.size())
  {
    UpgradeTree *tree = nullptr;
    if (selectCtx.buildSelected.size())
      tree = dynamic_cast<UpgradeTree *>(selectCtx.buildSelected[0]);
    renderer.render_suggestion(tree, &chunks, gameMode,
                               selectCtx.suggestionBuilds);
  }

  tgui::String str = tgui::String::fromNumber(fps);
  str += '\n';
  IVec mouseTilePos = screen_pos_to_tile_pos(mousePos, renderer.orientation);
  str += vec_str(mouseTilePos);

  get_widget<tgui::Label>("LabelFramerate")->setText(str);

  gui.draw();
}

void WindowGameplay::mouse_start(const sf::Vector2i &mousePos)
{
  IVec m = screen_pos_to_tile_pos(mousePos, renderer.orientation);
  inputCtx.drawInsert = t_dinsert_state::INSERT;

  auto &suggestion = selectCtx.suggestionBuilds;

  // Remove / add buildings to the suggestionBuilds list
  if (inputCtx.enableDraw &&
      chunks.is_free(m.x, m.y) != (gameMode == GameMode::DELETE))
  {
    inputCtx.drawInsert =
        suggestion.count(m) ? t_dinsert_state::REMOVE : t_dinsert_state::INSERT;
    if (inputCtx.drawInsert == t_dinsert_state::REMOVE)
      suggestion.erase(m);
    else
      suggestion.insert(m);
  }
}

void WindowGameplay::mouse_dragged(const sf::Vector2i &mousePos,
                                   const sf::Vector2i &pmousePos)
{
  auto &suggestion = selectCtx.suggestionBuilds;

  auto m0 = screen_pos_to_tile_pos(pmousePos, renderer.orientation);
  auto m1 = screen_pos_to_tile_pos(mousePos, renderer.orientation);
  if (inputCtx.enableDraw && m0 != m1 &&
      chunks.is_free(m1.x, m1.y) != (gameMode == GameMode::DELETE))
  {
    bool has = suggestion.count(m1);
    if (has && inputCtx.drawInsert == t_dinsert_state::REMOVE)
    {
      suggestion.erase(m1);
    }
    else if (!has && inputCtx.drawInsert == t_dinsert_state::INSERT)
    {
      suggestion.insert(m1);
    }

    grid_change_confirmation(m0);
    return;
  }

  if (!inputCtx.enableDraw)
    mouse_zoom_dragged(mousePos, pmousePos);
}

void WindowGameplay::mouse_released(const sf::Vector2i &mousePos) {}

void WindowGameplay::mouse_pressed(const sf::Vector2i &mousePos)
{
  auto &suggestion = selectCtx.suggestionBuilds;

  sf::Vector2i p = screen_pos_to_tile_pos(mousePos, renderer.orientation);

  if (inputCtx.isBtnPressed)
  {
    inputCtx.isBtnPressed = false;
    return;
  }

  if (gameMode == GameMode::VIEW)
  {
    inputCtx.searchEntity = true;
    inputCtx.searchEntityPos = (FVec)mousePos;
  }

  switch (gameMode)
  {
  case GameMode::DELETE:
  {
    if (!data.chunks->has_tile(p.x, p.y))
      break;
    BuildingBody *body = data.chunks->get_tile(p.x, p.y).building;
    if (body && !body->base->props.bool_is(PropertyBool::UNREMOVABLE))
    {
      if (suggestion.count(p))
        for (auto &b : body->base->get_bodies())
          suggestion.erase(b->tilePos);
      else
        for (auto &b : body->base->get_bodies())
          suggestion.insert(b->tilePos);
    }
  }
  break;
  case GameMode::BUILD:

    if (selectCtx.buildSelected.empty())
      break;

    if (suggestion.count(p))
      suggestion.erase(p);
    else
      suggestion.insert(p);
    // If pressed on  a building, show it's data.
    break;
  case GameMode::UPGRADE:
  {
    BuildingBody *body = data.chunks->get_tile(p.x, p.y).building;
    if (body)
    {
      if (body->base->props.bool_is(PropertyBool::UNREMOVABLE))
        break;
      
      this->data.infoBuild = body->base;
      auto pos = body->tilePos;
      if (suggestion.count(pos))
      {
        suggestion.erase(pos);
        gui_hide_upgrade_info();
      }
      else
      {
        suggestion.insert(pos);
        gui_show_upgrade_info(body->base);
      }
    }
  }
  break;
  default:
    break;
  }

  switch (gameMode)
  {
  case GameMode::DELETE:
  case GameMode::BUILD:
  case GameMode::UPGRADE:
    grid_change_confirmation(p);
    break;
  default:
    break;
  }
}

void WindowGameplay::mouse_zoom_dragged(const sf::Vector2i &mousePos,
                                        const sf::Vector2i &pmousePos)
{
  auto dif = mousePos - pmousePos;
  renderer.orientation.worldPos +=
      dif; // vec_90_rotate(dif, gridDraw.rotation);
}

void WindowGameplay::mouse_focus_start(const sf::Vector2i &a)
{
  this->selectCtx.preFocusBuilds = selectCtx.suggestionBuilds;
}

void WindowGameplay::mouse_focus(const sf::Vector2i &a, const sf::Vector2i &b)
{
  auto &selection = selectCtx.suggestionBuilds;

  // Reset the suggestions to the state before the drag started
  selection = selectCtx.preFocusBuilds;

  sf::Vector2i p1 = screen_pos_to_tile_pos(a, renderer.orientation);
  sf::Vector2i p2 = screen_pos_to_tile_pos(b, renderer.orientation);

  IVec treeSize = {1, 1};
  if (selectCtx.buildSelected.size())
  {
    UpgradeTree *treePtr =
        dynamic_cast<UpgradeTree *>(this->selectCtx.buildSelected[0]);
    assert(treePtr);
    UpgradeTree &tree = *treePtr;
    treeSize = tree.size;
  }

  auto xRange = Range<>(math_min(p1.x, p2.x), math_max(p1.x, p2.x), treeSize.x);
  auto yRange = Range<>(math_min(p1.y, p2.y), math_max(p1.y, p2.y), treeSize.y);
  // For nicer rendering
  xRange.reverse();
  yRange.reverse();

  auto axis = vec_90_rotate_axis(renderer.orientation.rotation);
  if (axis.x == -1)
    yRange.reverse();
  if (axis.y == -1)
    xRange.reverse();
  BuildingBase *mainBuild = nullptr;
  for (auto x : xRange)
  {
    for (auto y : yRange)
    {
      auto pos = sf::Vector2i{x, y};
      if (!data.chunks->has_tile(pos.x, pos.y))
        continue;

      switch (gameMode)
      {
      case GameMode::BUILD:
        selection.insert(sf::Vector2i{x, y});
        break;
      case GameMode::DELETE:
      {
        selection.insert(sf::Vector2i{x, y});
        BuildingBody *body = data.chunks->get_tile(x, y).building;
        if (!body)
          continue;

        for (auto &b : body->base->get_bodies())
        {
          const auto &p = b->tilePos;
          if (!(p1.x <= p.x && p.x <= p2.x && p1.y <= p.y && p.y <= p2.y))
            selection.insert(p);
        }
        break;
      }
      case GameMode::UPGRADE:
      {
        // Get body
        BuildingBody *body = data.chunks->get_tile(pos.x, pos.y).building;
        // If there is no body
        if (!body)
        {
          // Empty tile so you can keep it
          selection.insert(pos);
          continue;
        }
        BuildingBase *base = body->base;
        if (base->upgrades.empty())
          continue;
        if (base->alignment != ALIGNMENT_FRIENDLY)
          continue;

        if (mainBuild)
        {
          // Different building, remove
          if (!mainBuild->tree_similar(base))
            continue;
        }
        else
        {
          mainBuild = base;
        }
        for (auto &b : base->get_bodies())
          selection.insert(b->tilePos);
        break;
      }
      break;
      default:
        break;
      }
    }
  }
}

void WindowGameplay::mouse_focus_end(const sf::Vector2i &a,
                                     const sf::Vector2i &b)
{
  filter_suggestions();
  grid_change_confirmation(b);
}

void WindowGameplay::keyboard_pressed(sf::Keyboard::Key key)
{
  if (key == sf::Keyboard::Key::H)
  {
    renderer.halfWalls = true;
  }
}

void WindowGameplay::keyboard_released(sf::Keyboard::Key key)
{
  if (key == sf::Keyboard::Key::H)
  {
    renderer.halfWalls = false;
  }
}

void WindowGameplay::on_focus()
{
  data.timerManager.stop_all(data.get_time());
}

void WindowGameplay::on_unfocus()
{
  data.timerManager.resume_all(data.get_time());
}

bool WindowGameplay::file_write_jsonpack(const std::wstring &fileName,
                                         const t_jsonpack &jsonPack,
                                         bool compact)
{
  nlohmann::json jsonData;
  nlohmann::json jsonInfo;

  for (auto &jsonSingle : jsonPack)
  {
    if (jsonSingle.first == "info") {
      jsonInfo = jsonSingle.second;
    } else {
      jsonData[jsonSingle.first] = jsonSingle.second;
    }
  }

  std::error_code ec;
  std::filesystem::create_directories("saves", ec);

  std::wstring pathData = L"saves/" + fileName + L".json";
  std::ofstream fileOutData{std::filesystem::path(pathData)};

  if (!fileOutData || fileOutData.bad())
  {

    LOG_ERROR("Could not open data file at path \"%s\" for writing. Error: %s",
              pathData.c_str(), std::strerror(SYSERROR()));
    return false;
  }

  if (compact)
  {
    std::vector<std::uint8_t> v = json::to_cbor(jsonData);
    fileOutData.write(reinterpret_cast<char *>(v.data()), v.size());
  }
  else
  {
    fileOutData << jsonData.dump(4);
  }
  fileOutData.close();

  // Write the info file
  std::wstring pathInfo = L"saves/" + fileName + L"_info.json";
  std::ofstream fileOutInfo{std::filesystem::path(pathInfo)};

  if (!fileOutInfo || fileOutInfo.bad())
  {
    LOG_ERROR("Could not open info file at path \"%ls\" for writing.", 
      pathInfo.c_str());
    return false;
  }

  if (compact)
  {
    std::vector<std::uint8_t> v = nlohmann::json::to_cbor(jsonInfo);
    fileOutInfo.write(reinterpret_cast<char *>(v.data()), v.size());
  }
  else
  {
    fileOutInfo << jsonInfo.dump(4);
  }
  fileOutInfo.close();

  return true;
}

bool WindowGameplay::jsonpack_to_game(const t_jsonpack &jsonPack)
{
  DEBUG("Cleaning world:");
  this->data.clean_world();
  data.counterContext.variantFactory.list_types();

  ClassIdCounter &counter = this->data.counterContext.objectIdCounter;

  auto jsonToVariants = [&counter](const json &jsonFile, SerializeMap &map,
                                   VariantFactory &variantFactory,
                                   std::string keyName)
  {
    if (jsonFile.is_null())
      return false;

    if (!jsonFile.is_object())
      return false;

    // Every type of object always is in the same row
    // Meaning a json file that stores SERIALIZABLE_ENEMY will
    // Have only nulls up to this point.
    for (auto itType = jsonFile.begin(); itType != jsonFile.end(); ++itType)
    {
      size_t typeId = std::stoull(itType.key());
      const json &jsonObjs = itType.value();

      if (jsonObjs.is_null())
        continue;

      for (auto itObj = jsonObjs.begin(); itObj != jsonObjs.end(); ++itObj)
      {
        size_t objId = std::stoull(itObj.key());
        const json &jsonObj = itObj.value();

        Variant *t = variantFactory.create(map, typeId, objId);
        assert(t);

        counter.counterMap[typeId] =
            std::max(counter.counterMap[typeId], objId + 1);

        t->from_json(jsonObj);
      }
    }
    return true;
  };

  bool COMPRESSED = false;
  std::vector<json> jsons;

  std::string jsonFiles[] = {
      "data", "build_bodies", "build_bases",
      "entities", "networks", "bullets"};
  // Containts all serializable variants
  SerializeMap map;
  map.add(SERIALIZABLE_DATA, 0, dynamic_cast<Variant *>(&this->data));

  DEBUG("Loading other:");
  if (jsonPack.count("other"))
  {
    json jsonOther = jsonPack.at("other");
    renderer.orientation = jsonOther[0];
    const json jsonCons = jsonOther[1];
    map.add(SERIALIZABLE_CONSTRUCTION, 0, &data.constructions);

    data.constructions.from_json(jsonCons);
  }

  DEBUG("Loading chunks:");
  if (jsonPack.count("chunks"))
  {
    json jsonChunks = jsonPack.at("chunks");
    jsonChunks = jsonChunks = jsonChunks[std::to_string(SERIALIZABLE_CHUNKS)]
                                        [std::to_string(chunks.objectId)];
    map.add(SERIALIZABLE_CHUNKS, 0, &chunks);
    chunks.from_json(jsonChunks);
  }

  for (size_t i = 0; i < (sizeof(jsonFiles) / sizeof(*jsonFiles)); ++i)
  {
    DEBUG("Loading %s", jsonFiles[i].c_str());
    if (!jsonPack.count(jsonFiles[i]))
      continue;

    auto jsonFileItr = jsonPack.find(jsonFiles[i]);

    json jsonFile = jsonFileItr->second;
    if (jsonFile.is_null())
      continue;

    assert(jsonToVariants(jsonFile, map, data.counterContext.variantFactory,
                          jsonFileItr->first));
  }

  // Publish
  DEBUG("Publishing:");
  for (auto &y : map.data)
  {
    for (auto &x1 : y.second)
    {
      auto &x = x1.second;
      x->serialize_publish(map);
    }
  }

  // Debugging
  // for (auto &x : map) printf("{%d %d},\t", (int)x->objectType,
  // (int)x->objectId);

  // Initialize
  DEBUG("initializing:");
  for (auto &y : map.data)
  {
    for (auto &x1 : y.second)
    {
      auto &x = x1.second;
      {
        x->serialize_initialize(map);
      }
    }
  }

  DEBUG("Checking state:");
  if (!data.validate_state())
  {
    data.resolve_invalid_state();
  }

  return true;
}

template <class T>
static json &util_getter(json &file, const T &b)
{
  return file[std::to_string(b->objectType)][std::to_string(b->objectId)];
}

#undef OLD_JSON_PACK_FROM_GAME

#ifdef OLD_JSON_PACK_FROM_GAME

t_jsonpack WindowGameplay::jsonpack_from_game()
{
  t_jsonpack out{};

  const bool COMPRESSED = false;

  json
      jsonBuildBodies,
      jsonBuildBases, jsonNetworks,
      jsonEntities, jsonBullets, jsonChunks, jsonData, jsonOther;

  DEBUG("Loading in jsonpack");

  DEBUG("Building bodies:");
  for (auto &b : data.bodiesContext.buildings)
    util_getter(jsonBuildBodies, b) = b->to_json();
  out["build_bodies"] = jsonBuildBodies;

  DEBUG("Building bases:");
  for (auto &b : data.bodiesContext.buildingBases)
    util_getter(jsonBuildBases, b) = b->to_json();
  out["build_bases"] = jsonBuildBases;

  DEBUG("Resource networks:");
  for (auto &n : data.resourceContext.networks)
    util_getter(jsonNetworks, n) = n->to_json();
  out["networks"] = jsonNetworks;

  DEBUG("Enity bodies:");
  for (auto &b : data.bodiesContext.bodies)
  {
    if (b->type == BodyType::ENTITY)
    {
      util_getter(jsonEntities, b) = b->to_json();
    }

    if (b->type == BodyType::BULLET)
    {
      util_getter(jsonBullets, b) = b->to_json();
    }
  }

  out["entities"] = jsonEntities;
  out["bullets"] = jsonBullets;

  DEBUG("Chunks:");
  jsonChunks[std::to_string(chunks.objectType)]
            [std::to_string(chunks.objectId)] = chunks.to_json();
  out["chunks"] = jsonChunks;

  // Constructions *c = data.constructions;
  // jsonData[c->objectType][c->objectId] = c->to_json();
  DEBUG("Data:");
  out["data"] = jsonData;

  DEBUG("Other:");
  jsonOther[0] = renderer.orientation;
  jsonOther[1] = data.constructions.to_json();
  out["other"] = jsonOther;

  json jsonInfo;
  {
    DEBUG("Citizens size:");
    jsonInfo["citizens"] = data.bodiesContext.entityCitizens.size();
    DEBUG("Scenario:");
    jsonInfo["scenario"] = saveCtx.scenarioName;

    time_t curr_time;
    tm *curr_tm;
    char date_string[100];

    std::time(&curr_time);
    curr_tm = localtime(&curr_time);

    strftime(date_string, 50, "%T %B %d, %Y", curr_tm);

    DEBUG("Date:");
    jsonInfo["date"] = date_string;
    jsonInfo["dateInt"] = curr_time;
  }

  DEBUG("Info:");
  out["info"] = jsonInfo;

  return out;
}
#else

// Gemini's Pro implemenation, uses threading for performance
t_jsonpack WindowGameplay::jsonpack_from_game()
{
  t_jsonpack out{};
  const bool COMPRESSED = false;

  json jsonBuildBodies = json::array();
  json jsonBuildBases = json::array();
  json jsonEntities = json::array();
  json jsonBullets = json::array();
  json jsonNetworks = json::array();
  json jsonChunks, jsonData, jsonOther;

  // --- 1. Flatten Pointers ---
  std::vector<BuildingBody*> flatBuildBodies;
  for (auto& b : data.bodiesContext.buildings) {
      if (b.is_valid()) flatBuildBodies.push_back(b.get());
  }

  std::vector<BuildingBase*> flatBuildBases;
  for (auto& b : data.bodiesContext.buildingBases) {
      if (b) flatBuildBases.push_back(b);
  }

  std::vector<EntityBody*> flatEntities;
  std::vector<BulletBody*> flatBullets;
  for (auto& b : data.bodiesContext.bodies) {
      if (!b) continue;
      if (b->type == BodyType::ENTITY) flatEntities.push_back(dynamic_cast<EntityBody*>(b));
      else if (b->type == BodyType::BULLET) flatBullets.push_back(dynamic_cast<BulletBody*>(b));
  }

  // --- 2. Pre-allocate Output Vectors ---
  std::vector<json> parBuildBodies(flatBuildBodies.size());
  std::vector<json> parBuildBases(flatBuildBases.size());
  std::vector<json> parEntities(flatEntities.size());
  std::vector<json> parBullets(flatBullets.size());

  // --- 3. Execute Parallel Transformations ---
  std::transform(std::execution::par_unseq, 
                 flatBuildBodies.begin(), flatBuildBodies.end(), 
                 parBuildBodies.begin(), 
                 [](BuildingBody* b) { return b->to_json(); });

  std::transform(std::execution::par_unseq, 
                 flatBuildBases.begin(), flatBuildBases.end(), 
                 parBuildBases.begin(), 
                 [](BuildingBase* b) { return b->to_json(); });

  std::transform(std::execution::par_unseq, 
                 flatEntities.begin(), flatEntities.end(), 
                 parEntities.begin(), 
                 [](EntityBody* b) { return b->to_json(); });

  std::transform(std::execution::par_unseq, 
                 flatBullets.begin(), flatBullets.end(), 
                 parBullets.begin(), 
                 [](BulletBody* b) { return b->to_json(); });

  // --- 4. Assign Results ---
  out["build_bodies"] = parBuildBodies;
  out["build_bases"] = parBuildBases;
  out["entities"] = parEntities;
  out["bullets"] = parBullets;

  // ... Existing sequential code for networks, chunks, constructions, and info ...
  
  for (auto &n : data.resourceContext.networks)
    jsonNetworks.push_back(n->to_json());
  out["networks"] = jsonNetworks;

  jsonChunks[std::to_string(chunks.objectType)][std::to_string(chunks.objectId)] = chunks.to_json();
  out["chunks"] = jsonChunks;

  out["data"] = jsonData;

  jsonOther[0] = renderer.orientation;
  jsonOther[1] = data.constructions.to_json();
  out["other"] = jsonOther;

  json jsonInfo;
  {
    jsonInfo["citizens"] = data.bodiesContext.entityCitizens.size();
    jsonInfo["scenario"] = saveCtx.scenarioName;

    time_t curr_time;
    tm *curr_tm;
    char date_string[100];
    std::time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(date_string, 50, "%T %B %d, %Y", curr_tm);

    jsonInfo["date"] = date_string;
    jsonInfo["dateInt"] = curr_time;
  }
  out["info"] = jsonInfo;

  return out;
}
#endif


void WindowGameplay::popup_confirmation_window(PopupConfirmationData *data)
{
  tgui::ChildWindow::Ptr childWindow = tgui::ChildWindow::create("popupWindow");

  childWindow->setSize({"30%", "30%"});
  childWindow->setPosition({"50%", "50%"});
  childWindow->setTitle(data->title);
  gui.add(childWindow);

  tgui::Button::Ptr btnOkay = tgui::Button::create("btnPopupOkay");
  btnOkay->setSize({"25.05%", "16.7%"});
  btnOkay->setPosition({"5%", "73.3%"});
  btnOkay->setText(data->okStr);
  btnOkay->onClick(
      [](PopupConfirmationData *data, tgui::ChildWindow::Ptr childWindow)
      {
        data->onConfirmation();
        childWindow->close();
      },
      data, childWindow);
  childWindow->add(btnOkay);

  tgui::Button::Ptr btnCancel = tgui::Button::create("btnPopupOkay");
  btnCancel->setSize({"25.05%", "16.7%"});
  btnCancel->setPosition({"69.95%", "73.3%"});
  btnCancel->setText(data->cancelStr);
  btnCancel->onClick(
      [](PopupConfirmationData *data, tgui::ChildWindow::Ptr childWindow)
      {
        data->onCancel();
        childWindow->close();
      },
      data, childWindow);
  childWindow->add(btnCancel);

  tgui::Label::Ptr labelDescription = tgui::Label::create("btnPopupLabel");
  labelDescription->setSize({"90.2%", "60.29%"});
  labelDescription->setPosition({"5%", "7.22%"});
  labelDescription->setText(data->description);
  childWindow->add(labelDescription);

  data->postInit();
}

void WindowGameplay::gui_init_resource_tab()
{
  for (auto &x : guiCtx.labelResources)
  {
    ResLabelData &data = x.second;
    data.ptr = FastLabel::create();
    data.ptr->setCharSpriteSheet(&guiCtx.charSpriteSheet);
    data.ptr->renderWindow = window;
    data.ptr->set(get_widget<tgui::Label>(data.title));
    data.ptr->setText("Test");
    gui.add(data.ptr);
  }
}

void WindowGameplay::gui_update_resources_tab(UiResources resources,
                                              int value)
{
  ResLabelData &data = guiCtx.labelResources[resources];
  if (data.lastValue != value)
    data.ptr->setText(std::to_string(value));
  data.lastValue = value;
}

bool WindowGameplay::gui_change_group(const std::string &groupName)
{
  for (const tgui::Widget::Ptr &widget : gui.getWidgets())
  {
    assert(widget);
    try
    {
      std::string str;
      str = widget->getUserData<tgui::String>().toStdString();
      if (str == "GroupMainCore" || str == groupName)
        widget->setVisible(true);
      else
        widget->setVisible(false);
    }
    catch (std::bad_cast &)
    {
      continue;
    }
  }
  return true;
}

bool WindowGameplay::gui_enable_group(const std::string &groupName)
{
  for (const tgui::Widget::Ptr &widget : gui.getWidgets())
  {
    assert(widget);
    try
    {
      std::string str;
      str = widget->getUserData<tgui::String>().toStdString();
      if (str == groupName)
        widget->setVisible(true);
    }
    catch (std::bad_cast &)
    {
      continue;
    }
  }
  return true;
}

bool WindowGameplay::gui_disable_group(const std::string &groupName)
{
  for (const tgui::Widget::Ptr &widget : gui.getWidgets())
  {
    assert(widget);
    try
    {
      std::string str;
      str = widget->getUserData<tgui::String>().toStdString();
      if (str == groupName)
        widget->setVisible(false);
    }
    catch (std::bad_cast &)
    {
      continue;
    }
  }
  return true;
}

bool WindowGameplay::gui_update_upgrade_info(UpgradeTree *info,
                                             BuildingBase *build)

{
  tgui::ChildWindow::Ptr newWindow;
  newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");

  std::string name = string_format(build->name, info->name);
  str_replace(name, "_", " ");
  get_widget<tgui::Label>(newWindow, "LabelUpgradeTitle")->setText(name);
  get_widget<tgui::Label>(newWindow, "LabelUpgradeCost")
      ->setText("Cost: " + (info->build.empty()
                                ? data.resources_to_str(info->build)
                                : "None"));

  // Layout stuff

  tgui::VerticalLayout::Ptr layout;
  layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutUpgradeInfo");

  layout->removeAllWidgets();

  if (info->spriteHolders.size())
  {
    IVec start, end;
    upgrade_building_get_frames(build, info, start, end);
    int s = assets.load_sprites(build->tree->image, start, end);

    if (s != -1)
    {
      sf::Sprite *sprite = assets.get_sprite(s)->get(0);

      // Use getTextureRect() to get the actual sprite coordinates on the sheet
      sf::IntRect ir = sprite->getTextureRect();
      tgui::UIntRect rect = {(unsigned)ir.position.x, (unsigned)ir.position.y,
                             (unsigned)ir.size.x, (unsigned)ir.size.y};

      tgui::Texture tex;
      sf::Image image = sprite->getTexture().copyToImage();
      tex.loadFromPixelData({image.getSize().x, image.getSize().y}, image.getPixelsPtr(), rect);

      get_widget<tgui::Picture>(newWindow, "PictureUpgradeWindow")
          ->getRenderer()
          ->setTexture(tex);
    }
  }

  if (strcmp(info->description, ""))
  {
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Description: " + tgui::String{info->description});
    layout->add(label);
  }

  if (info->hp != NULL_INT && info->hp > 0)
  {
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("HP: " + tgui::String::fromNumber(info->hp));
    layout->add(label);
  }

  if (!info->storageCap.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr labelRes = tgui::Label::create();
    labelRes->setText("Storage cap: " +
                      data.resources_to_str(info->storageCap));
    layout->add(labelRes);
  }

  if (info->weightCap != NULL_INT)
  {
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Weight cap: " + tgui::String::fromNumber(info->weightCap));
    layout->add(label);
  }

  if (info->entityLimit > 0)
  {
    std::string strEntityType = "Workers";
    if (info->props.bool_is(PropertyBool::HOME))
      strEntityType = "Residents";
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText(strEntityType +
                   " limit: " + tgui::String::fromNumber(info->entityLimit));
    layout->add(label);
  }

  if (!info->input.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Resource requirement: " +
                   data.resources_to_str(info->input));
    layout->add(label);
  }

  if (!info->output.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Resource output: " + data.resources_to_str(info->output));
    layout->add(label);
  }

  if (info->powerIn > 0)
  {
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Power input: " + tgui::String::fromNumber(info->powerIn));
    layout->add(label);
  }

  if (info->powerOut > 0)
  {
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Power output: " + tgui::String::fromNumber(info->powerOut));
    layout->add(label);
  }

  if (info->powerStore > 0)
  {
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Power storage: " +
                   tgui::String::fromNumber(info->powerStore));
    layout->add(label);
  }

  return true;
}

bool WindowGameplay::gui_hide_upgrade_info()

{
  tgui::ChildWindow::Ptr window;
  window = get_widget<tgui::ChildWindow>("WindowUpgrade");
  window->setVisible(false);

  return true;
}

bool WindowGameplay::gui_show_upgrade_info()
{
  return this->gui_show_upgrade_info(selectCtx.buildUpgradeSelected);
}

bool WindowGameplay::gui_show_upgrade_info(BuildingBase *build)

{
  this->selectCtx.buildUpgradeSelected = build;

  tgui::ChildWindow::Ptr window;
  window = get_widget<tgui::ChildWindow>("WindowUpgrade");

  window->setVisible(true);

  selectCtx.upgradeTree = build->get_upgrades();
  if (selectCtx.upgradeTree.size())
  {
    selectCtx.upgradeTreeItr = selectCtx.upgradeTree.begin();
    gui_update_upgrade_info(*selectCtx.upgradeTreeItr, build);
  }
  else
  {
    tgui::ChildWindow::Ptr newWindow;
    newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");
    tgui::VerticalLayout::Ptr layout;
    layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutUpgradeInfo");

    layout->removeAllWidgets();

    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("No upgrades available");
    layout->add(label);

    get_widget<tgui::Label>(newWindow, "LabelUpgradeTitle")
        ->setText(build->get_format_name());
  }

  auto btnPrev = get_widget<tgui::Button>("ButtonUpgradePrev");
  btnPrev->onClick.disconnectAll();
  btnPrev->onClick(
      [](WindowGameplay *window, BuildingBase *build)
      {
        SelectionGameContext *selection = &window->selectCtx;

        if (selection->upgradeTree.empty())
          return;
        if (selection->upgradeTreeItr == selection->upgradeTree.begin())
        {
          selection->upgradeTreeItr =
              std::prev(selection->upgradeTree.end());
        }
        else
          selection->upgradeTreeItr = std::prev(selection->upgradeTreeItr);
        window->gui_update_upgrade_info(*selection->upgradeTreeItr, build);
      },
      this, build);

  auto btnNext = get_widget<tgui::Button>("ButtonUpgradeNext");
  btnNext->onClick.disconnectAll();
  btnNext->onClick(
      [](WindowGameplay *window, BuildingBase *build)
      {
        SelectionGameContext *selection = &window->selectCtx;
        if (selection->upgradeTree.empty())
          return;
        ++selection->upgradeTreeItr;
        if (selection->upgradeTreeItr == selection->upgradeTree.end())
        {
          selection->upgradeTreeItr = selection->upgradeTree.begin();
        }
        window->gui_update_upgrade_info(*selection->upgradeTreeItr, build);
      },
      this, build);

  return true;
}

bool WindowGameplay::gui_update_constr_build(t_id_config *idConfig,
                                             tgui::Texture tex)

{
  UpgradeTree *config = dynamic_cast<UpgradeTree *>(idConfig->at(0));

  const tgui::ChildWindow::Ptr &window =
      gui.get<tgui::ChildWindow>("WindowConstruction");
  if (!window)
    return false;

  tgui::String strTitle, strCost, strWork, strDesc;
  strTitle = BuildingBase::get_format_str(config->name);
  strCost = "Cost: " + data.resources_to_str(config->build);
  strWork = "Work: " + tgui::String::fromNumber(config->buildWork);
  strDesc = config->description;

  get_widget<tgui::Label>("LabelConstrTitle")->setText(strTitle);
  get_widget<tgui::Label>("LabelConstrCost")->setText(strCost);
  get_widget<tgui::Label>("LabelConstrWork")->setText(strWork);
  get_widget<tgui::Label>("LabelConstrDescr")->setText(strDesc);

  get_widget<tgui::Picture>(window, "PictureConstrWindow")
      ->getRenderer()
      ->setTexture(tex);

  LOG("%s", config->to_string().c_str());

  std::shared_ptr<tgui::Button> confirmBtn;
  confirmBtn = get_widget<tgui::Button>(window, "ButtonConstructionConfirm");

  confirmBtn->onClick.disconnectAll();

  confirmBtn->onClick(
      [](WindowGameplay *gameWindow, t_id_config *config)
      {
        LOG("buildSelected has set");
        gameWindow->selectCtx.buildSelected = *config;
        gameWindow->gameMode = GameMode::BUILD;
        gameWindow->gui_change_group("GroupBuild");
      },
      this, idConfig);

  return true;
}

bool WindowGameplay::gui_update_constrs_window()

{
  // For WindowConstructions

  const tgui::ScrollablePanel::Ptr &scroll =
      gui.get<tgui::ScrollablePanel>("ScrollableConstructions");
  const tgui::HorizontalWrap::Ptr &wrap =
      gui.get<tgui::HorizontalWrap>("HorizontalConstructions");
  tgui::Panel::Ptr panelClone =
      tgui::Panel::copy(gui.get<tgui::Panel>("PanelConstruction"));

  if (!scroll || !wrap || !panelClone)
    return false;

  wrap->removeAllWidgets();

  tgui::Vector2f wrapSize = wrap->getSize();
  float xPos = 0;
  float yCount = 0;
  auto &m = data.bodyConfigMap.at(ENUM_BUILDING_TYPE);
  for (auto &x : m)
  {
    UpgradeTree *config = dynamic_cast<UpgradeTree *>(x.second.at(0));
    assert(config);

    if (config->props.bool_is(PropertyBool::HIDDEN))
      continue;

    tgui::Panel::Ptr newPanel = tgui::Panel::copy(panelClone);
    wrap->add(newPanel);

    tgui::Label::Ptr labelConstruction =
        newPanel->get<tgui::Label>("LabelConstruction");
    tgui::Picture::Ptr labelImage =
        newPanel->get<tgui::Picture>("PictureConstruction");
    if (!labelConstruction)
      continue;

    labelConstruction->setText(
        tgui::String(BuildingBase::get_format_str(config->name)));

    tgui::Texture tex;
    if (config->spriteIcon != 0)
    {
      WARNING("todo");
    }
    else if (config->spriteHolders.size())
    {
      t_sprite s = config->spriteHolders.at(IVec{0, 0});
      sf::Sprite *sprite = nullptr;
      if (renderer.assets->get_sprite(s))
      {
        sprite = renderer.assets->get_sprite(s)->get(0);

        tgui::UIntRect rect;
        sf::FloatRect fr = sprite->getLocalBounds();
        rect = {(unsigned)fr.position.x, (unsigned)fr.position.y,
                (unsigned)fr.size.x, (unsigned)fr.size.y};

        tgui::Texture tex;
        sf::Image image = sprite->getTexture().copyToImage();

        tex.loadFromPixelData(
            {image.getSize().x, image.getSize().y},
            image.getPixelsPtr(),
            rect);

        labelImage->getRenderer()->setTexture(tex);
      }
    }

    xPos += newPanel->getSize().x;
    if (xPos + newPanel->getSize().x > wrapSize.x)
    {
      xPos = 0;
      yCount += newPanel->getSize().y;
    }

    newPanel->onClick(
        [](tgui::Gui &gui, WindowGameplay *gameWindow, tgui::Texture tex,
           t_id_config *config)
        {
          tgui::ChildWindow::Ptr w1, w2;
          w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
          w2 = gui.get<tgui::ChildWindow>("WindowConstruction");
          if (w1 && w2)
          {
            w1->setVisible(false);
            w2->setVisible(true);
          }
          else
            assert(0);
          gameWindow->gui_update_constr_build(config, tex);
        },
        std::ref(gui), this, tex, &x.second);
  }
  if (xPos != 0)
    yCount += panelClone->getSize().y;

  wrap->setSize({wrap->getSize().x, yCount});

  scroll->setContentSize({0.f, 0.f});
  scroll->getHorizontalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
  scroll->getVerticalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Always);

  return 1;
}

void WindowGameplay::filter_suggestions()
{
  auto &suggestion = selectCtx.suggestionBuilds;
  for (auto itr = suggestion.begin(); itr != suggestion.end();)
  {
    bool skipDelete;
    if (gameMode == GameMode::BUILD)
    {
      skipDelete = data.chunks->get_tile(itr->x, itr->y).building;
    }
    else
    {
      BuildingBody *body = data.chunks->get_tile(itr->x, itr->y).building;
      skipDelete =
          !(body && !body->base->props.bool_is(PropertyBool::UNREMOVABLE));
    }
    if (skipDelete)
      itr = suggestion.erase(itr);
    else
      ++itr;
  }
}

void WindowGameplay::filter_upgrade_buildings(const IVec &latest)
{
  BuildingBody *beginBuild = data.chunks->get_build(latest.x, latest.y);
  assert(beginBuild);
  for (auto itr = selectCtx.suggestionBuilds.begin();
       itr != selectCtx.suggestionBuilds.end();)
  {
    auto &pos = *itr;
    BuildingBody *b = data.chunks->get_build(pos.x, pos.y);
    assert(b);
    if (!b->base->tree_similar(beginBuild->base))
      itr = selectCtx.suggestionBuilds.erase(itr);
    else
      ++itr;
  }
}

bool WindowGameplay::has_build_info(BuildingBase *infoBuild)
{
  return guiCtx.bInfoWin.count(infoBuild->objectId);
}

void WindowGameplay::show_build_info(BuildingBase *infoBuild, bool update)
{
  data.infoBuild = infoBuild;
  tgui::ChildWindow::Ptr newWindow;

  auto itr = guiCtx.bInfoWin.find(infoBuild->objectId);

  if (itr == guiCtx.bInfoWin.end())
  {

    newWindow =
        tgui::ChildWindow::copy(get_widget<tgui::ChildWindow>("WindowBuild"));
    newWindow->setVisible(true);
    newWindow->setResizable(true);
    gui.add(newWindow);

    newWindow->setPosition(guiCtx.bInfoPtr.size() * 20,
                           newWindow->getPosition().y);

    guiCtx.bInfoWin.insert({infoBuild->objectId, newWindow});
    guiCtx.bInfoPtr.insert({infoBuild->objectId, infoBuild});

    get_widget<tgui::Button>(newWindow, "ButtonBuildBack")
        ->onClick(
            [](WindowGameplay *window, BuildingBase *infoBuild,
               tgui::ChildWindow::Ptr childWindow, tgui::Gui *gui)
            {
              childWindow->setVisible(false);
              window->guiCtx.bInfoPtr.erase(infoBuild->objectId);
              window->guiCtx.bInfoWin.erase(infoBuild->objectId);
              gui->remove(childWindow);
            },
            this, infoBuild, newWindow, &gui);

    get_widget<tgui::Label>(newWindow, "LabelBuildTitle")
        ->setText(tgui::String(infoBuild->get_format_name()));
    // newWindow->setTitle(tgui::String(infoBuild->get_format_name()));
  }
  else
  {
    newWindow = guiCtx.bInfoWin.at(infoBuild->objectId);
    if (!update)
    {
      newWindow->setVisible(false);
      guiCtx.bInfoPtr.erase(infoBuild->objectId);
      guiCtx.bInfoWin.erase(infoBuild->objectId);
      gui.remove(newWindow);
      return;
    }
  }

  get_widget<tgui::Label>(newWindow, "LabelBuildHp")
      ->setText(tgui::String("HP: ") + tgui::String::fromNumber(infoBuild->hp));

  tgui::VerticalLayout::Ptr layout;
  layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutBuildInfo");

  layout->removeAllWidgets();

  auto makeLine = []()
  {
    tgui::Panel::Ptr panel;
    panel = tgui::Panel::create();

    tgui::SeparatorLine::Ptr lineSep;
    lineSep = tgui::SeparatorLine::create();
    lineSep->setPosition("10%, 50%");
    lineSep->setSize({"80%", "5"});
    lineSep->setWidgetName("coolLine");

    panel->add(lineSep);
    panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    return panel;
  };

  auto x = makeLine();
  layout->add(x);

  // Add how much of the building's storage is full
  if (infoBuild->weightCap != NULL_INT || !infoBuild->rStoreCap.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr labelRes = tgui::Label::create();
    labelRes->setText("Stored Resources: " +
                      data.resources_to_str(infoBuild->rStorage));
    layout->add(labelRes);

    int max = -1, val = 0;
    tgui::String weightVal, weightMax;
    if (infoBuild->weightCap != NULL_INT)
    {
      val = infoBuild->rStorage.weight(&data.resourceContext.weights);
      max = infoBuild->weightCap;
    }
    else if (!infoBuild->rStoreCap.empty())
    {
      val = infoBuild->rStorage.weight(&data.resourceContext.weights);
      max = infoBuild->rStoreCap.weight(&data.resourceContext.weights);
    }

    if (max != -1)
    {
      weightVal = tgui::String::fromNumber(val);
      weightMax = tgui::String::fromNumber(max);

      tgui::HorizontalLayout::Ptr layoutRes;
      layoutRes = tgui::HorizontalLayout::create();

      tgui::Label::Ptr labelResCapPer = tgui::Label::create();
      labelResCapPer->setText("Capacity: " + weightVal + " / " + weightMax);

      tgui::ProgressBar::Ptr pb;
      pb = tgui::ProgressBar::create();
      pb->setMinimum(0);
      pb->setMaximum(max);
      pb->setValue(val);

      layoutRes->add(labelResCapPer);
      layoutRes->add(pb);
      layout->add(layoutRes);

      layout->add(makeLine());
    }
  }

  if (infoBuild->entityLimit > 0)
  {
    std::string strEntityType = "entities";
    if (infoBuild->props.bool_is(PropertyBool::HOME))
      strEntityType = "residents";
    else if (infoBuild->props.bool_is(PropertyBool::WORKPLACE))
      strEntityType = "workers";
    else if (infoBuild->alignment == ALIGNMENT_ENEMY)
      strEntityType = "spawns";

    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Entity " + strEntityType + ": " +
                   tgui::String::fromNumber(infoBuild->entities.size()));
    layout->add(label);

    tgui::HorizontalLayout::Ptr layoutRes;
    layoutRes = tgui::HorizontalLayout::create();

    tgui::Label::Ptr labelResCapPer = tgui::Label::create();
    labelResCapPer->setText(
        tgui::String::fromNumber(infoBuild->entities.size()) + " / " +
        tgui::String::fromNumber(infoBuild->entityLimit));

    tgui::ProgressBar::Ptr pb;
    pb = tgui::ProgressBar::create();
    pb->setMinimum(0);
    pb->setMaximum(infoBuild->entityLimit);
    pb->setValue((unsigned)infoBuild->entities.size());

    layoutRes->add(labelResCapPer);
    layoutRes->add(pb);
    layout->add(layoutRes);

    layout->add(makeLine());
  }

  if (!infoBuild->rIn.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Resource requirement: " +
                   data.resources_to_str(infoBuild->rIn));
    layout->add(label);

    layout->add(makeLine());
  }

  if (!infoBuild->rOut.empty())
  {
    // Add how many resources in the building
    tgui::Label::Ptr label = tgui::Label::create();
    label->setText("Resource output: " +
                   data.resources_to_str(infoBuild->rOut));
    layout->add(label);

    layout->add(makeLine());
  }

  if (infoBuild->network)
  {
    if (infoBuild->powerIn > 0)
    {
      tgui::Label::Ptr label = tgui::Label::create();
      label->setText("Power input: " +
                     tgui::String::fromNumber(infoBuild->powerIn));
      layout->add(label);
    }

    if (infoBuild->powerOut > 0)
    {
      tgui::Label::Ptr label = tgui::Label::create();
      label->setText("Power output: " +
                     tgui::String::fromNumber(infoBuild->powerOut));
      layout->add(label);
    }

    if (infoBuild->powerStore > 0)
    {
      tgui::HorizontalLayout::Ptr layoutRes;
      layoutRes = tgui::HorizontalLayout::create();

      tgui::Label::Ptr label = tgui::Label::create();
      label->setText(
          "Power stored: " + tgui::String::fromNumber(infoBuild->powerValue) +
          " / " + tgui::String::fromNumber(infoBuild->powerStore));
      layout->add(label);

      tgui::ProgressBar::Ptr pb;
      pb = tgui::ProgressBar::create();
      pb->setMinimum(0);
      pb->setMaximum(infoBuild->powerStore);
      pb->setValue(infoBuild->powerValue);

      layoutRes->add(label);
      layoutRes->add(pb);
      layout->add(layoutRes);
    }

    layout->add(makeLine());
  }

  // todo
  auto str = infoBuild->to_string();
  if (infoBuild->buildType == (t_id)BuildingType::CONSTRUCTION)
  {
    assert(0);

    str = "In Construction:";
    ConstructionData *construct = nullptr;
    try
    {
      construct = &data.constructions.at(infoBuild);
    }
    catch (std::out_of_range &e)
    {
      LOG_ERROR("Invalid construction building: %s", e.what());
    }

    if (construct)
    {
      int wn = construct->work_needed();
      str += std::to_string(wn - construct->work_left()) + " : " +
             std::to_string(wn);
      str += "\n";
      DEBUG("str += str_unfold(construct->queueData.step->to_string_tree(&data, false));");
    }
    else
    {
      str += "\nMissing construction data";
    }
  }
}

bool WindowGameplay::has_entity_info(EntityBody *entity)
{
  return guiCtx.eInfoWin.count(entity->objectId);
}

void WindowGameplay::show_entity_info(EntityBody *entity, bool update)
{
  t_weights &weights = data.resourceContext.weights;

  data.infoEntity = entity;
  tgui::ChildWindow::Ptr newWindow;

  if (!guiCtx.eInfoWin.count(entity->objectId))
  {
    newWindow =
        tgui::ChildWindow::copy(get_widget<tgui::ChildWindow>("WindowEntity"));
    newWindow->setVisible(true);
    newWindow->setResizable(true);
    gui.add(newWindow);

    newWindow->setPosition(guiCtx.bInfoPtr.size() * 20,
                           newWindow->getPosition().y);

    guiCtx.eInfoWin.insert({entity->objectId, newWindow});
    guiCtx.eInfoPtr.insert({entity->objectId, entity});

    get_widget<tgui::Button>(newWindow, "ButtonEntityClose")
        ->onClick(
            [](WindowGameplay *window, EntityBody *entity,
               tgui::ChildWindow::Ptr childWindow, tgui::Gui *gui)
            {
              childWindow->setVisible(false);
              window->guiCtx.eInfoPtr.erase(entity->objectId);
              window->guiCtx.eInfoWin.erase(entity->objectId);
              gui->remove(childWindow);
            },
            this, entity, newWindow, &gui);
  }
  else
  {
    newWindow = guiCtx.eInfoWin.at(entity->objectId);
    if (!update)
    {
      newWindow->setVisible(false);
      gui.remove(newWindow);
      guiCtx.eInfoPtr.erase(entity->objectId);
      guiCtx.eInfoWin.erase(entity->objectId);
      return;
    }
  }

  auto makeLine = []()
  {
    tgui::Panel::Ptr panel = tgui::Panel::create();
    tgui::SeparatorLine::Ptr lineSep = tgui::SeparatorLine::create();
    lineSep->setPosition("10%, 50%");
    lineSep->setSize({"80%", "5"});
    lineSep->setWidgetName("coolLine");
    panel->add(lineSep);
    panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
    return panel;
  };

  tgui::VerticalLayout::Ptr layout;
  layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutEntity");
  layout->removeAllWidgets();

  // 1. Generic Entity Stats
  std::string nameStr;
  if (entity->entityType == EntityType::CITIZEN)
  {
    EntityCitizen *citizen = dynamic_cast<EntityCitizen *>(entity);
    nameStr = str_lowercase(data.enum_get_str(ENUM_CITIZEN_JOB, (t_id)citizen->job, true));
  }
  else if (entity->entityType == EntityType::ENEMY)
  {
    EntityEnemy *enemy = dynamic_cast<EntityEnemy *>(entity);
    nameStr = str_lowercase(data.enum_get_str(ENUM_ENEMY_TYPE, (t_id)enemy->enemyType, true));
  }
  nameStr += " - " + std::to_string((int)entity->objectId);
  get_widget<tgui::Label>(newWindow, "LabelEntityJob")->setText(tgui::String(nameStr));

  auto addLabel = [&](const std::string &text)
  {
    tgui::Label::Ptr lbl = tgui::Label::create();
    lbl->setText(text);
    layout->add(lbl);
  };

  addLabel("HP: " + std::to_string(entity->hp));
  addLabel("Attack: " + std::to_string(entity->attack));
  addLabel("Speed: " + std::to_string(entity->maxSpeed).substr(0, 4));

  layout->add(makeLine());

  // 2. Action Data
  std::string strAction = EntityBodyStr::ACTION_STRS[(size_t)entity->action];
  if (strAction.find("#destination") != std::string::npos && entity->pathData.valid())
  {
    BuildingBase *buildDest = entity->pathData.destination->get_building();
    std::string destStr = buildDest ? (buildDest->get_format_name() + " at " + vec_str(buildDest->tilePos))
                                    : ("position " + vec_str(entity->pathData.destPos));
    str_replace(strAction, "#destination", destStr);
  }
  addLabel(strAction);

  layout->add(makeLine());

  // 3. Citizen Specific (Inventory)
  if (entity->entityType == EntityType::CITIZEN)
  {
    EntityCitizen *citizen = dynamic_cast<EntityCitizen *>(entity);
    if (citizen->inventorySize != NULL_INT || !citizen->rInventoryCap.empty())
    {
      addLabel("Stored Resources: " + data.resources_to_str(citizen->rInventory));

      int max = -1, val = 0;
      if (citizen->inventorySize != NULL_INT)
      {
        val = citizen->rInventory.weight(&weights);
        max = citizen->inventorySize;
      }
      else if (!citizen->rInventoryCap.empty())
      {
        val = citizen->rInventory.weight(&weights);
        max = citizen->rInventoryCap.weight(&weights);
      }

      if (max != -1)
      {
        tgui::HorizontalLayout::Ptr layoutRes = tgui::HorizontalLayout::create();
        tgui::Label::Ptr labelResCapPer = tgui::Label::create();
        labelResCapPer->setText("Capacity: " + std::to_string(val) + " / " + std::to_string(max));

        tgui::ProgressBar::Ptr pb = tgui::ProgressBar::create();
        pb->setMinimum(0);
        pb->setMaximum(max);
        pb->setValue(val);

        layoutRes->add(labelResCapPer);
        layoutRes->add(pb);
        layout->add(layoutRes);
        layout->add(makeLine());
      }
    }
  }
}

void WindowGameplay::grid_change_confirmation(IVec latest)

{
  if (selectCtx.suggestionBuilds.empty())
  {
    gui_disable_group("GroupGridBuild");
    return;
  }
  // todo
  // guiButtons["grid cost"].active = 1;

  gui_enable_group("GroupGridBuild");

  switch (gameMode)
  {
  case GameMode::BUILD:
  {
    if (selectCtx.buildSelected.empty())
      break;
    UpgradeTree *tree;
    tree = dynamic_cast<UpgradeTree *>(selectCtx.buildSelected[0]);
    assert(tree);
    Resources costRes = tree->build;
    costRes = costRes * selectCtx.suggestionBuilds.size();
    std::string costStr =
        (costRes.empty()
             ? "Free"
             : costRes.to_string_weights(data.resourceContext.weights));
  }
  break;

  case GameMode::UPGRADE:
  {
    if (data.chunks->has_build(latest.x, latest.y))
      filter_upgrade_buildings(latest);

    tgui::ChildWindow::Ptr newWindow;
    newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");
    if (selectCtx.suggestionBuilds.empty())
      gui_hide_upgrade_info();
    else
      gui_show_upgrade_info();
  }
  break;
  default:
    break;
  }
}

void WindowGameplay::update_upgrade_info()
{
  BuildingBase *base = suggestion_build();
  if (!base)
    return;

  std::string str = "\t" + base->get_format_name() + "\n";

  auto ul = base->get_upgrades();
  if (ul.empty())
  {
    str += "No upgrades found";
    return;
  }

  selectCtx.suggestionUpgrade =
      math_min(selectCtx.suggestionUpgrade, (int)ul.size() - 1);
  selectCtx.suggestionUpgrade = math_max(selectCtx.suggestionUpgrade, 0);

  auto &u = ul.at(selectCtx.suggestionUpgrade);
  auto str2 = u->to_string(&data, false);
  str2 = str_unfold(str2);
  str +=
      "\tCost: " + (u->build * selectCtx.suggestionBuilds.size()).to_string() +
      "\n" + str2;
}

void WindowGameplay::upgrade_building_get_frames(BuildingBase *build,
                                                 UpgradeTree *step,
                                                 IVec &frame0, IVec &frame1)

{
  std::vector<UpgradeTree *> steps =
      recursive_get_last_layer_tree(build->tree, build->upgrades);

  frame0 = build->tree->framesStart;
  frame1 = build->tree->framesEnd;
  DEBUG("%s %s", VEC_CSTR(frame0), VEC_CSTR(frame1));

  for (UpgradeTree *upgrade : steps)
  {
    IVec frame2 = upgrade->framesStart;
    IVec frame3 = upgrade->framesEnd;
    frame0 = {frame2.x == -1 ? frame0.x : frame2.x,
              frame2.y == -1 ? frame0.y : frame2.y};
    frame1 = {frame3.x == -1 ? frame1.x : frame3.x,
              frame3.y == -1 ? frame1.y : frame3.y};
    DEBUG("%s %s", VEC_CSTR(frame2), VEC_CSTR(frame3));
  }
}

void WindowGameplay::upgrade_building(BuildingBase *build, UpgradeTree *step)

{
  assert(build);
  assert(step);

  // Upgrade the building
  build->load_upgrade_step(step);

  // Get the upgrade tree's leafs
  std::vector<UpgradeTree *> steps =
      recursive_get_last_layer_tree(build->tree, build->upgrades);

  IVec start, end;
  upgrade_building_get_frames(build, step, start, end);

  auto &info = *build->tree;

  IVec frameCount;
  frameCount = assets.get_texture_from_name(info.image).frameCount;
  int mainSprite = assets.load_sprites(info.image, start, end);

  DEBUG("%s %d %s %s", info.image, (int)mainSprite, VEC_CSTR(start),
        VEC_CSTR(end));

  if (mainSprite == -1)
    WARNING("Error on loading image: \"%s\". Args: %s %s", info.image,
            VEC_CSTR(start), VEC_CSTR(end));

  build->sprites[sf::Vector2i{0, 0}] = mainSprite;
  // build->update_sprites(renderer.orientation.rotation);

  // todo
  WARNING("upgrade texture unimplemented");
}

void WindowGameplay::mouse_zoom(const sf::Vector2i &iCenterPos, int dist)
{

  if (gui.getWidgetAtPos(
             tgui::Vector2f{(float)iCenterPos.x, (float)iCenterPos.y}, true)
          .get())
    return;

  auto &orientation = renderer.orientation;
  auto &zoomFactor = orientation.zoomFactor;
  auto &worldPos = orientation.worldPos;

  float prevS, currS;
  // The location of the mouse on screen
  sf::Vector2f centerPos =
      (sf::Vector2f)vec_90_rotate(iCenterPos, 0 * orientation.rotation);

  prevS = powf(2.0f, zoomFactor / 256.f);
  zoomFactor += dist;
  currS = powf(2.0f, zoomFactor / 256.f);
  orientation.scale = {currS, currS};

  sf::Vector2i nextWorldPos =
      (sf::Vector2i)(currS / prevS * ((sf::Vector2f)worldPos - centerPos) +
                     centerPos);

  worldPos = nextWorldPos;
}

void WindowGameplay::focus_on(const sf::Vector2f &fpos)
{
  auto &orientation = renderer.orientation;
  orientation.worldPos = (sf::Vector2i)view->getCenter();
  sf::Vector2f center =
      screen_pos_to_world_pos(sf::Vector2i{0, 0}, orientation);

  orientation.worldPos = {0, 0};
  orientation.worldPos =
      -world_pos_to_screen_pos<float>(fpos + center, orientation);
}

void WindowGameplay::rotate_world(int dif)
{
  auto &orientation = renderer.orientation;
  dif = math_mod(dif, 4);

  sf::Vector2f center =
      screen_pos_to_world_pos((sf::Vector2i)view->getCenter(), orientation);
  orientation.worldPos = {0, 0};
  orientation.rotation = math_mod(orientation.rotation + dif, 4);

  auto pos = world_pos_to_screen_pos(sf::Vector2f{10.f, 10.f}, orientation);
  DEBUG("%s", VEC_CSTR(pos));

  focus_on(center);
}
