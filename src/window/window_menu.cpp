#include "window/window_menu.hpp"

#include <filesystem>

#include "utils/class/logger.hpp"
#include "utils/utils.hpp"
#include "window/window_gameplay.hpp"
#include "window/window_manager.hpp"
#include "window/gui_utils.hpp" // Ensure gui_utils is included for file_delete, file_read_jsonpack, etc.

// 1. Define PopupConfirmationData implementations for the Menu
struct LoadMenuPopupData : public PopupConfirmationData {
    WindowMenu* menuWindow;
    std::wstring fileName;

    LoadMenuPopupData(WindowMenu* mw, const std::wstring& fName) 
        : PopupConfirmationData("Load game?", "Are you sure you want to load this save? Current progress will be lost."), 
          menuWindow(mw), fileName(fName) {}

    void onConfirmation() override {
        WindowManager *manager = menuWindow->managerParent;
        GameWindow *gwindow = manager->get_window_from_id("GAMEPLAY");
        WindowGameplay *window = (WindowGameplay *)gwindow;

        LOG("Reading Json Pack from file");
        t_jsonpack jsonPack = file_read_jsonpack(fileName, window->flags.compactJson);
        
        LOG("Loading Json Pack");
        window->jsonpack_to_game(jsonPack);

        manager->change_window(window);
    }
};

struct DeleteMenuPopupData : public PopupConfirmationData {
    WindowMenu* menuWindow;
    tgui::Gui& gui;
    std::wstring fileName;
    std::function<void(const std::wstring&)> onLoad;
    std::function<void(const std::wstring&)> onDelete;
    std::function<void(const std::wstring&)> onSave;

    DeleteMenuPopupData(WindowMenu* mw, tgui::Gui& g, const std::wstring& fName,
                        std::function<void(const std::wstring&)> loadCb,
                        std::function<void(const std::wstring&)> deleteCb,
                        std::function<void(const std::wstring&)> saveCb)
        : PopupConfirmationData("Delete file?", "Are you sure you want to delete this save? This save will be lost."),
          menuWindow(mw), gui(g), fileName(fName), onLoad(loadCb), onDelete(deleteCb), onSave(saveCb) {}

    void onConfirmation() override {
        // Delete the file using the global utility
        ::file_delete(fileName);

        // Fetch compactJson flag from Gameplay window
        WindowManager *manager = menuWindow->managerParent;
        WindowGameplay *window = (WindowGameplay *)manager->get_window_from_id("GAMEPLAY");
        bool isCompact = window ? window->flags.compactJson : true;

        // Refresh the save files window
        guiutils_save_files(gui, false, isCompact, true, onLoad, onDelete, onSave);
    }
};

WindowMenu::WindowMenu() {}

WindowMenu::~WindowMenu() { }

bool WindowMenu::init_window(sf::RenderWindow *window, sf::View *view) {
  GameWindow::init_window(window, view);
  gui.setTarget(*window);
  return true;
}

bool WindowMenu::init() {
  std::string guiFormPath =
      std::filesystem::current_path().string() + "/assets/gui/FormMenu.txt";
  try {
    gui.loadWidgetsFromFile(guiFormPath);
  } catch (std::exception const &e) {
    LOG_ERROR("Unable to call loadWidgetsFromFile. "
              "Error: %s",
              e.what());
    return false;
  }

  // Apply scaling
  float scaleFactor = managerParent->get_settings().uiScale;
  gui.setRelativeView(tgui::FloatRect{0.f, 0.f, 1.f / scaleFactor, 1.f / scaleFactor});

  // ButtonSaveFiles Implementation
  get_widget<tgui::Button>("ButtonSaveFiles")
      ->onPress(
          [this](WindowMenu *menuWindow, tgui::Gui &gui) {
              
            auto onLoad = [this](const std::wstring &fileName) {
                guiutils_popup_confirmation_window(this->gui, new LoadMenuPopupData(this, fileName));
            };

            auto onDeletePtr = std::make_shared<std::function<void(const std::wstring &)>>();
            auto onSavePtr = std::make_shared<std::function<void(const std::wstring &)>>(); 
            
            // Dummy save action, saving is disabled from the main menu
            *onSavePtr = [](const std::wstring &) {}; 

            *onDeletePtr = [this, &gui, onLoad, onDeletePtr, onSavePtr](const std::wstring &fileName) {
                guiutils_popup_confirmation_window(gui, new DeleteMenuPopupData(this, gui, fileName, onLoad, *onDeletePtr, *onSavePtr));
            };

            // Grab the compactJson flag from the gameplay window state
            WindowManager *manager = this->managerParent;
            WindowGameplay *window = (WindowGameplay *)manager->get_window_from_id("GAMEPLAY");
            bool isCompact = window ? window->flags.compactJson : true;

            guiutils_save_files(gui, false, isCompact, true, onLoad, *onDeletePtr, *onSavePtr);
          },
          this, std::ref(gui));

  // --- Missing FormMenu Buttons ---
  get_widget<tgui::Button>("ButtonCampaign")
      ->onPress(
          [](WindowMenu *menuWindow) { LOG("Campaign window to be implemented"); },
          this);

  get_widget<tgui::Button>("ButtonScenarioMaker")
      ->onPress(
          [](WindowMenu *menuWindow) { LOG("Scenario Maker to be implemented"); },
          this);

  // --- Existing Buttons ---
  get_widget<tgui::Button>("ButtonSettings")
      ->onPress([](WindowMenu *menuWindow, tgui::Gui &gui, WindowManager *manager) { 
            guiutils_settings(manager, gui); 
        }, this, std::ref(gui), std::ref(this->managerParent));

  get_widget<tgui::Button>("ButtonAbout")
      ->onPress(
          [](WindowMenu *menuWindow) { LOG("About window to be implemented"); },
          this);

  get_widget<tgui::Button>("ButtonExitGame")
      ->onPress(
          [](WindowMenu *menuWindow) {
            if (menuWindow->window)
              menuWindow->window->close();
          },
          this);

  return true;
}

void WindowMenu::close() {}

void WindowMenu::update(float delta) {}

void WindowMenu::render() { gui.draw(); }

void WindowMenu::mouse_released(const sf::Vector2i &mousePos) {}