#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <functional>

#include "nlohmann/json.hpp"

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

typedef std::unordered_map<std::string, nlohmann::json> t_jsonpack;
typedef std::map<int, std::wstring> t_savedgames;

struct GameSettings;
struct SettingsDefaults;
class WindowManager;

struct PopupConfirmationData
{
    std::string title = "";
    std::string description = "";

    std::string cancelStr = "Cancel";
    std::string okStr = "Okay";

    PopupConfirmationData(
        const std::string& title,
        const std::string& description)
    {
        this->title = title;
        this->description = description;
    }

    virtual ~PopupConfirmationData()
    {
    }

    virtual void onConfirmation()
    {
    }

    virtual void onCancel()
    {
    }

    virtual void postInit()
    {
    }
};

class GenericPopup : public tgui::ChildWindow
{
public:
    using Ptr = std::shared_ptr<GenericPopup>;

    GenericPopup(const tgui::String& title, const tgui::String& message, std::function<void()> onAccept)
    {
        setTitle(title);
        setSize({"50%", "30%"});
        setPosition("50%", "50%");
        setOrigin(0.5f, 0.5f); 

        auto label = tgui::Label::create(message);
        label->setPosition({"5%", "10%"});
        label->setSize({"90%", "50%"});
        label->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
        add(label);

        auto btnYes = tgui::Button::create("Yes");
        btnYes->setSize({"30%", "20%"});
        btnYes->setPosition({"15%", "70%"});
        btnYes->onPress([this, onAccept]() {
            if (onAccept) onAccept();
            close();
        });
        add(btnYes);

        auto btnNo = tgui::Button::create("Cancel");
        btnNo->setSize({"30%", "20%"});
        btnNo->setPosition({"55%", "70%"});
        btnNo->onPress([this]() {
            close();
        });
        add(btnNo);
    }

    static Ptr create(const tgui::String& title, const tgui::String& message, std::function<void()> onAccept)
    {
        return std::make_shared<GenericPopup>(title, message, onAccept);
    }
};

bool file_delete(const std::wstring &fileName);

void guiutils_popup_confirmation_window(tgui::Gui& gui, PopupConfirmationData* data);

bool guiutils_settings(WindowManager* manager, tgui::Gui& gui);

bool guiutils_save_files(
    tgui::Gui &gui, 
    bool isSaving, bool compact, bool forceRefresh,
    std::function<void(const std::wstring &)> onLoadAction,
    std::function<void(const std::wstring &)> onDeleteAction,
    std::function<void(const std::wstring &)> onSaveAction);

t_savedgames list_saved_games();

t_jsonpack file_read_jsonpack(const std::wstring &fileName, bool compact);