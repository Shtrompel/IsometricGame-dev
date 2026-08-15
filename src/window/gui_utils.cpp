#include "window/gui_utils.hpp"

#include <filesystem>
#include <fstream>
#include <memory>

#include "utils/globals.hpp"
#include "utils/class/logger.hpp"
#include "window/game_window.hpp"
#include "window/window_manager.hpp"

#define QUICK_FIND(container, strId, TYPE)                                     \
  (std::find_if(container.begin(), container.end(),                            \
                [strId](const TYPE &a) { return a.title == strId; }))


bool file_delete(const std::wstring &fileName) {
  std::wstring wPath = L"saves/" + fileName + L".json";
  const wchar_t *cwPath = wPath.c_str();

  std::string cPath;
  size_t size;
  cPath.resize(wPath.length());

#ifdef _WIN32
  wcstombs_s(&size, &cPath[0], cPath.size() + 1, wPath.c_str(), wPath.size());
#else
  wcstombs(&cPath[0], wPath.c_str(), wPath.size());
#endif

  std::remove(cPath.c_str());

  return false;
}

void guiutils_popup_confirmation_window(tgui::Gui &gui,
                                        PopupConfirmationData *data) {
  tgui::ChildWindow::Ptr childWindow = tgui::ChildWindow::create("popupWindow");

  childWindow->setSize({"30%", "30%"});
  childWindow->setPosition({"50%", "50%"});
  childWindow->setTitle(data->title);
  gui.add(childWindow);

  tgui::Button::Ptr btnOkay = tgui::Button::create("btnPopupOkay");
  btnOkay->setSize({"25.05%", "16.7%"});
  btnOkay->setPosition({"25.05%", "73.3%"});
  btnOkay->setText(data->okStr);
  btnOkay->onClick(
      [](PopupConfirmationData *data, tgui::ChildWindow::Ptr childWindow) {
        data->onConfirmation();
        childWindow->close();
        delete data; // FIX: Prevent memory leak!
      },
      data, childWindow);
  childWindow->add(btnOkay);

  tgui::Button::Ptr btnCancel =
      tgui::Button::create("btnPopupCancel");
  btnCancel->setSize({"25.05%", "16.7%"});
  btnCancel->setPosition({"69.95%", "73.3%"});
  btnCancel->setText(data->cancelStr);
  btnCancel->onClick(
      [](PopupConfirmationData *data, tgui::ChildWindow::Ptr childWindow) {
        data->onCancel();
        childWindow->close();
        delete data; // FIX: Prevent memory leak!
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

static bool guiutils_offload_settings(tgui::ChildWindow::Ptr &gui,
                                      GameSettings &settings,
                                      const SettingsDefaults &defaults) {
  tgui::Slider::Ptr sliderUIVolume, sliderMusicVolume, sliderFxVolume;
  sliderUIVolume = gui->get<tgui::Slider>("SliderUIVolume");
  if (sliderUIVolume != nullptr)
    settings.volumeUI = (int)sliderUIVolume->getValue();
  else
    return false;
  sliderMusicVolume = gui->get<tgui::Slider>("SliderMusicVolume");
  if (sliderMusicVolume != nullptr)
    settings.volumeMusic = (int)sliderMusicVolume->getValue();
  else
    return false;
  sliderFxVolume = gui->get<tgui::Slider>("SliderFxVolume");
  if (sliderFxVolume != nullptr)
    settings.volumeGame = (int)sliderFxVolume->getValue();
  else
    return false;

  tgui::ComboBox::Ptr comboGraphicsQuality, comboResolution, comboSaving;

  comboGraphicsQuality = gui->get<tgui::ComboBox>("ComboGraphicsQuality");
  if (comboGraphicsQuality != nullptr) {
    std::string quality =
        comboGraphicsQuality->getSelectedItemId().toStdString();
    auto itr = QUICK_FIND(defaults.qualities, quality, SettingsQuality);
    if (itr != defaults.qualities.end())
      settings.quality = *itr;
  } else
    return false;
  comboResolution = gui->get<tgui::ComboBox>("ComboResolution");
  if (comboResolution != nullptr) {
    std::string resolution = comboResolution->getSelectedItemId().toStdString();
    auto itr = QUICK_FIND(defaults.resolutions, resolution, SettingsResolution);
    if (itr != defaults.resolutions.end())
      settings.resolution = *itr;
  } else
    return false;
  comboSaving = gui->get<tgui::ComboBox>("ComboSaving");
  if (comboSaving != nullptr) {
    std::string interval = comboSaving->getSelectedItemId().toStdString();
    auto itr =
        QUICK_FIND(defaults.intervals, interval, SettingsAutosaveInterval);
    if (itr != defaults.intervals.end())
      settings.interval = *itr;
  } else
    return false;

  tgui::CheckBox::Ptr checkFullscreen, checkVSync;
  checkFullscreen = gui->get<tgui::CheckBox>("CheckFullscreen");
  if (checkFullscreen != nullptr)
    settings.isFullscreen = checkFullscreen->isChecked();
  else
    return false;

  checkVSync = gui->get<tgui::CheckBox>("CheckVSync");
  if (checkVSync != nullptr)
    settings.enableVSync = checkVSync->isChecked();
  else
    return false;

  return true;
}

static bool guiutils_populate_settings(tgui::ChildWindow::Ptr &gui,
                                       const GameSettings &settings) {
  tgui::Slider::Ptr sliderUIVolume, sliderMusicVolume, sliderFxVolume;
  sliderUIVolume = gui->get<tgui::Slider>("SliderUIVolume");
  if (sliderUIVolume != nullptr)
    sliderUIVolume->setValue((float)settings.volumeUI);
  sliderMusicVolume = gui->get<tgui::Slider>("SliderMusicVolume");
  if (sliderMusicVolume != nullptr)
    sliderMusicVolume->setValue((float)settings.volumeMusic);
  sliderFxVolume = gui->get<tgui::Slider>("SliderFxVolume");
  if (sliderFxVolume != nullptr)
    sliderFxVolume->setValue((float)settings.volumeGame);

  tgui::ComboBox::Ptr comboGraphicsQuality, comboResolution, comboSaving;

  comboGraphicsQuality = gui->get<tgui::ComboBox>("ComboGraphicsQuality");
  if (comboGraphicsQuality != nullptr)
    comboGraphicsQuality->setSelectedItemById(settings.quality.title);
  comboResolution = gui->get<tgui::ComboBox>("ComboResolution");
  if (comboResolution != nullptr)
    comboResolution->setSelectedItemById(settings.resolution.title);
  comboSaving = gui->get<tgui::ComboBox>("ComboSaving");
  if (comboSaving != nullptr)
    comboSaving->setSelectedItemById(settings.interval.title);

  tgui::CheckBox::Ptr checkFullscreen, checkVSync;
  checkFullscreen = gui->get<tgui::CheckBox>("CheckFullscreen");
  if (checkFullscreen != nullptr)
    checkFullscreen->setChecked(settings.isFullscreen);

  checkVSync = gui->get<tgui::CheckBox>("CheckVSync");
  if (checkVSync != nullptr)
    checkVSync->setChecked(settings.enableVSync);

  return true;
}

bool guiutils_settings(WindowManager *manager, tgui::Gui &gui) {

  tgui::Gui guiTmp;

  std::string guiFormPath =
      std::filesystem::current_path().string() + "/assets/gui/FormSettings.txt";
  try {
    guiTmp.loadWidgetsFromFile(guiFormPath);
  } catch (std::exception const &e) {
    LOG_ERROR("Unable to call loadWidgetsFromFile. In %s."
              "Error: %s",
              FUNC_NAME, e.what());
    return false;
  }

  // If a window already exists, don't change it
  tgui::ChildWindow::Ptr w;
  if ((w = gui.get<tgui::ChildWindow>("WindowSettings")).get()) {
    w->setVisible(!w->isVisible());
    w->moveToFront();
    guiutils_populate_settings(w, manager->get_settings());
    return true;
  }

  tgui::ChildWindow::Ptr windowSettings, windowNew;
  windowSettings = guiTmp.get<tgui::ChildWindow>("WindowSettings");
  windowNew = tgui::ChildWindow::create("WindowSettings");

  windowNew->setResizable(windowSettings->isResizable());

  DEBUG("open settings");
  for (tgui::Widget::Ptr ptr : windowSettings->getWidgets()) {
    windowNew->add(ptr->clone(), ptr->getWidgetName());
  }

  // Populate variations
  {
    const SettingsDefaults &defaults = manager->get_default_settings();

    tgui::ComboBox::Ptr comboGraphicsQuality, comboResolution, comboSaving;
    comboGraphicsQuality =
        windowNew->get<tgui::ComboBox>("ComboGraphicsQuality");
    comboResolution = windowNew->get<tgui::ComboBox>("ComboResolution");
    comboSaving = windowNew->get<tgui::ComboBox>("ComboSaving");

    for (auto &x : defaults.resolutions) {
      comboResolution->addItem(x.item_name(), x.title);
    }

    for (auto &x : defaults.qualities)
      comboGraphicsQuality->addItem(x.title, x.title);

    for (auto &x : defaults.intervals)
      comboSaving->addItem(x.title, x.title);
  }

  windowNew->get<tgui::Button>("ButtonCancel")
      ->onClick([](tgui::ChildWindow::Ptr window) { window->close(); },
                windowNew);

  windowNew->get<tgui::Button>("ButtonApply")
      ->onClick(
          [](tgui::Gui &gui, tgui::ChildWindow::Ptr window,
             WindowManager *manager) {
            GameSettings settings = manager->get_settings();
            assert(guiutils_offload_settings(window, settings,
                                             manager->get_default_settings()));

            manager->set_settings_data(settings);
            manager->apply_settings();
          },
          std::ref(gui), windowNew, manager);

  windowNew->get<tgui::Button>("ButtonConfirm")
      ->onClick(
          [](tgui::ChildWindow::Ptr window, WindowManager *manager) {
            GameSettings settings = manager->get_settings();
            assert(guiutils_offload_settings(window, settings,
                                             manager->get_default_settings()));

            manager->set_settings_data(settings);
            manager->apply_settings();

            window->close();
          },
          windowNew, manager);

  guiutils_populate_settings(windowNew, manager->get_settings());

  gui.add(windowNew, "WindowSettings");

  return true;
}

std::map<int, std::wstring> list_saved_games() {
  std::map<int, std::wstring> out{};

  std::error_code ec;
  if (!std::filesystem::exists("./saves", ec)) {
    std::filesystem::create_directories("./saves", ec);
    return out; // Folder is brand new, so it's empty
  }

  for (const auto &entry : std::filesystem::directory_iterator("./saves")) {

    std::wstring widePath = entry.path().wstring();
    if (!entry.is_regular_file())
      continue;

    std::wstring filename = std::filesystem::path(widePath).stem().wstring();

    std::wstring fileName = L"";
    std::wstring fileNumber = L"";
    bool validFileName = true;
    int step = 0;
    for (size_t i = 0; i < filename.size(); ++i) {
      wchar_t c = filename.at(i);
      if (isdigit(c)) {
        if (i == 0) {
          validFileName = false;
        }
        if (step == 0)
          step = 1;
        fileNumber += c;
      } else {
        if (step != 0) {
          validFileName = false;
        }
        fileName += c;
      }
    }

    if (!validFileName)
      continue;

    if (fileName == L"" || fileName != L"save")
      continue;

    if (fileNumber == L"")
      continue;

    int num = (int)wcstol(fileNumber.c_str(), nullptr, 10);
    DEBUG("Listed Saved Game: %S %d", widePath.c_str(), (int)num);
    if (1 <= num && num <= 16) {
      out[num] = filename;
    }
  }

  return out;
}

t_jsonpack file_read_jsonpack(const std::wstring &fileName, bool compact) {
  t_jsonpack out{};

  std::ifstream fileIn;
  std::wstring path = L"saves/" + fileName + L".json";
  fileIn.open(std::filesystem::path(path));

  nlohmann::json j;

  try {
    fileIn.clear();
    fileIn.seekg(0, std::ios::beg);
    if (compact) {
      j = j.from_cbor(fileIn);
    } else {
      j = nlohmann::json::parse(fileIn);
    }
  } catch (nlohmann::json::exception &e) {
    fileIn.close();
    LOG_ERROR("Can't parse json file \"%ls\" Compact: \"%s\": %s", path.c_str(),
              (compact ? "True" : "False"), e.what());
    return t_jsonpack{};
  }

  fileIn.close();

  for (auto &jsonGroup : j.items()) {
    out[jsonGroup.key()] = jsonGroup.value();
  }

  return out;
}

bool guiutils_save_files(
    tgui::Gui &gui, 
    bool isSaving, bool compact, bool forceRefresh,
    std::function<void(const std::wstring &)> onLoadAction,
    std::function<void(const std::wstring &)> onDeleteAction,
    std::function<void(const std::wstring &)> onSaveAction) {
  // Toggle existing window on/off
  if (auto existingWindow = gui.get<tgui::ChildWindow>("WindowSaves")) {
    if (forceRefresh) {
      gui.remove(existingWindow);
    } else {
      existingWindow->setVisible(!existingWindow->isVisible());
      existingWindow->moveToFront();
      return false;
    }
  }

  auto window = tgui::ChildWindow::create();
  window->setWidgetName("WindowSaves");
  window->setPosition(260, 80);
  window->setSize(520, 450);
  window->setResizable(true);
  window->setTitleAlignment(tgui::HorizontalAlignment::Center);
  window->setTitleButtons(tgui::ChildWindow::TitleButton::Close);
  window->setTitleTextSize(13);

  if (!window) return false;

  auto scrollPanel = tgui::ScrollablePanel::create();
  scrollPanel->setWidgetName("ScrollableSave");
  scrollPanel->setSize("100%", "100%");
  window->add(scrollPanel);

  auto layout = tgui::VerticalLayout::create();
  layout->setWidgetName("LayoutSaves");
  layout->setSize("100%", "100%");
  scrollPanel->add(layout);

  auto savedGames = list_saved_games();
  float panelHeight = 150.f; 
  float totalHeight = 0.f;
  bool addedNewSave = false;

  for (int i = 1; i <= 16; ++i) {
    auto itrFind = savedGames.find(i);
    bool isExistingSave = (itrFind != savedGames.end());

    // If there is no save here, only show it if we are saving and haven't added a "New Save" slot yet.
    if (!isExistingSave) {
      if (isSaving && !addedNewSave) {
        addedNewSave = true;
      } else {
        continue;
      }
    }

    std::wstring fileName = isExistingSave ? itrFind->second : (L"save" + std::to_wstring(i));

    auto savePanel = tgui::Panel::create();
    savePanel->setSize("100%", panelHeight);

    if (!savePanel) return false;

    std::string timeText = "Last Save: ";
    std::string scenarioText = "Scenario: ";
    std::string popText = "Population: ";

    if (isExistingSave) {
      t_jsonpack jsonPack = file_read_jsonpack(fileName, compact);
      nlohmann::json jsonData;
      if (jsonPack.count("other"))
        jsonData = jsonPack.at("other");

      if (jsonData.count("date")) timeText += jsonData.at("date").get<std::string>();
      if (jsonData.count("scenario")) scenarioText += jsonData.at("scenario").get<std::string>();
      if (jsonData.count("population")) popText += jsonData.at("population").get<std::string>();
    } else {
      timeText += "N/A";
      scenarioText += "New Save";
      popText += "0";
    }

    auto pic = tgui::Picture::create("assets/gui/resources/DefaultPicture.png");
    auto labelName = tgui::Label::create(isExistingSave ? (L"Save " + std::to_wstring(i)) : L"New Save Slot");
    auto labelTime = tgui::Label::create(timeText);
    auto labelScenario = tgui::Label::create(scenarioText);
    auto labelPop = tgui::Label::create(popText);
    auto btnLoad = tgui::Button::create("Load");
    auto btnSave = tgui::Button::create(isExistingSave ? "Override" : "Save");
    auto btnDelete = tgui::Button::create("Delete");

    // Layout configuration
    pic->setPosition("1.93%", "12.83%");
    pic->setSize("19.31%", "64.14%");
    pic->setIgnoreMouseEvents(true); 
    savePanel->add(pic, "PictureSave");

    labelName->setPosition("48.26%", "0%");
    labelName->setSize("19.69%", "19.12%");
    labelName->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
    labelName->setTextSize(13);
    savePanel->add(labelName, "LabelSaveName");

    labelTime->setPosition("23.17%", "12.83%");
    labelTime->setSize("42.66%", "14.81%");
    labelTime->setTextSize(13);
    savePanel->add(labelTime, "LabelSaveTime");

    labelScenario->setPosition("23.17%", "25.65%");
    labelScenario->setSize("41.31%", "14.81%");
    labelScenario->setTextSize(13);
    savePanel->add(labelScenario, "LabelSaveScenario");

    labelPop->setPosition("23.17%", "38.49%");
    labelPop->setSize("34.56%", "14.81%");
    labelPop->setTextSize(13);
    savePanel->add(labelPop, "LabelSavePopulation");

    // Button Load
    btnLoad->setPosition("36.68%", "75.57%");
    btnLoad->setSize("20%", "20%");
    btnLoad->setTextSize(13);
    btnLoad->setVisible(isExistingSave); // Only show load if a save exists
    btnLoad->onClick([onLoadAction, fileName, window]() {
      onLoadAction(fileName);
      window->close();
    });
    savePanel->add(btnLoad, "ButtonSaveLoad");

    // Button Save/Override
    btnSave->setPosition("57.91%", "75.57%");
    btnSave->setSize("20%", "20%");
    btnSave->setTextSize(13);
    btnSave->setVisible(isSaving); // Only show if the user opened the window via the Save menu
    btnSave->onClick([onSaveAction, fileName]() {
      onSaveAction(fileName);
    });
    savePanel->add(btnSave, "ButtonSaveSave");

    // Button Delete
    btnDelete->setPosition("79.15%", "75.57%");
    btnDelete->setSize("20%", "20%");
    btnDelete->setTextSize(13);
    btnDelete->setVisible(isExistingSave); // Only show delete if a save exists
    btnDelete->onClick([fileName, onDeleteAction]() { 
      onDeleteAction(fileName); 
    });
    savePanel->add(btnDelete, "ButtonSaveDelete");

    layout->add(savePanel);
    totalHeight += panelHeight;
  }

  layout->setSize("100%", totalHeight);
  scrollPanel->setContentSize({0.f, totalHeight});

  gui.add(window, "WindowSaves");

  return true;
}

#undef QUICK_FIND