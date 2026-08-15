#pragma once

#include <cstdio>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <SFML/Graphics.hpp>
#include <nlohmann/json_fwd.hpp>

class WindowManager;


bool json_file_load(
	nlohmann::json& json, 
	const std::string& filePath);

bool json_file_write(
	const nlohmann::json& json, 
	const std::string& filePath);


struct GameWindow
{
	// SFML stuff
	sf::RenderWindow* window = nullptr;
	sf::View* view = nullptr;

	// Data
	int frameCount = 0;
	float fps = -1.0f;
	float time = 0.0f;

	sf::Vector2i mousePos = { -1, -1 };
	bool mousePressed = false;

	tgui::Gui gui;
	WindowManager* managerParent = nullptr;

	GameWindow()
	{
	}

	virtual bool
		init_window(sf::RenderWindow* window, sf::View* view)
	{
		this->window = window;
		this->view = view;
		return true;
	}

	virtual bool
		load() { return false; }

	virtual bool
		init() { return false; }

	virtual void
		update(float delta) {}

	virtual void
		close() { }

	virtual void render() {}

	virtual void mouse_pressed(const sf::Vector2i& mousePos) {}

	virtual void mouse_start(const sf::Vector2i& pos) {}

	virtual void mouse_released(const sf::Vector2i& pos) {}

	virtual void mouse_dragged(
		const sf::Vector2i& mousePos,
		const sf::Vector2i& pmousePos)
	{
	}

	virtual void mouse_zoom(
		const sf::Vector2i& iCenterPos,
		int dist) {};

	virtual void mouse_zoom_dragged(
		const sf::Vector2i& mousePos,
		const sf::Vector2i& pmousePos)
	{
	}

	virtual void mouse_focus_start(
		const sf::Vector2i& startPos)
	{
	}

	virtual void mouse_focus(
		const sf::Vector2i& startPos,
		const sf::Vector2i& endPos)
	{
	}

	virtual void mouse_focus_end(
		const sf::Vector2i& startPos,
		const sf::Vector2i& endPos)
	{
	}

	virtual void on_focus()
	{
	}

	virtual void on_unfocus()
	{
	}

	template <class T>
	typename std::shared_ptr<T> get_widget(
		tgui::Container::Ptr x,
		const tgui::String& str)
	{
		typename T::Ptr out = x->get<T>(str);
		if (!out)
		{
			printf("Widget \"%s\" was not found.", 
				str.toStdString().c_str());
			assert(out);
			return std::make_shared<T>();
		}
		return out;
	}

	template <class T>
	typename std::shared_ptr<T> get_widget(const tgui::String& str)
	{
		typename T::Ptr out = gui.get<T>(str);
		if (!out)
		{
			printf("Widget \"%s\" was not found.", 
				str.toStdString().c_str());
			assert(out);
			return std::make_shared<T>();
		}
		return out;
	}

};
