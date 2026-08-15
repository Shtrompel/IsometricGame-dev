#pragma once

#include "window/game_window.hpp"
#include "window/gui_utils.hpp"



struct WindowMenu : GameWindow
{

    WindowMenu();

    ~WindowMenu();

    bool load() override
    {
        return true;
    }

    bool
        init_window(sf::RenderWindow* window, sf::View* view) override;

    bool init() override;

    void close() override;

    void update(float delta) override;

    void
        render() override;

    void
        mouse_released(const sf::Vector2i& mousePos) override;

    PopupConfirmationData* popupTest = nullptr;

};

