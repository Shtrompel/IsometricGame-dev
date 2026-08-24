#pragma once

#include <cstdio>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "window/game_window.hpp"

struct SettingsQuality
{
	std::string title = "";
	int index = 0;

	std::string to_string() const
	{
		char out[50];
		sprintf(out, "{%s %d}", title.c_str(), index);
		return std::string(out);
	}
};

struct SettingsResolution
{
	std::string title{};
	int x = 0;
	int y = 0;
	int ratioX, ratioY = 0;

	std::string item_name() const
	{
		char out[50];
		sprintf(out, "{%dx%d (%d/%d)}", 
			x, y, ratioX, ratioY);
		return std::string(out);
	}
	
	std::string to_string() const
	{
		std::stringstream ss{};
		ss << "{ " << title << " : ";
		ss << item_name() << "}";
		return ss.str();
	}
};

struct SettingsAutosaveInterval
{
	std::string title{};
	int minutes = 0;

	std::string to_string() const
	{
		char out[50];
		sprintf(out, "{%s, %d minutes}", title.c_str(), minutes);
		return std::string(out);
	}
};

struct GameSettings
{
	// All volume ranges between zero (silent) to 100 (full volume).
	int volumeMusic = 0;
	int volumeGame = 0;
	int volumeUI = 0;

	SettingsQuality quality{};
	SettingsResolution resolution{};
	bool isFullscreen = false;
	bool enableVSync = false;
	SettingsAutosaveInterval interval{};

	float uiScale = 1.0;

	std::string to_string() const
	{
		std::stringstream ss;
		ss << "{ ";
		ss << "Music Volume: " << volumeMusic << ", ";
		ss << "Game Volume: " << volumeGame << ", ";
		ss << "UI Volume: " << volumeUI << ", ";
		ss << "Graphics Quality: " << quality.to_string() << ", ";
		ss << "Resolution: " << resolution.to_string() << ", ";

		ss << "Is Fullscreen: " << 
			(isFullscreen ? "true" : "false") << ", ";
		ss << "Enable VSync: " <<
			(enableVSync ? "true" : "false") << ", ";

		ss << "Autosave Intervals: " << interval.to_string() << ", ";
		ss << "}";
		return ss.str();
	}
};

struct SettingsDefaults
{
	std::vector<SettingsResolution> resolutions;
	std::vector<SettingsQuality> qualities;
	std::vector<SettingsAutosaveInterval> intervals;

	GameSettings settings;
};


bool json_settings_load(
	const nlohmann::json&,
	GameSettings&,
	const SettingsDefaults&);

bool json_settings_save(nlohmann::json&, const GameSettings&);

bool json_to_default_settings(SettingsDefaults&, const nlohmann::json&);

class WindowManager
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

	std::vector< GameWindow* > windows;
	GameWindow* current = nullptr;
	bool changeWindow = false;

	SettingsDefaults defaultSettings;
	GameSettings settings;

public:

	std::string windowName;

	std::map<std::string, GameWindow*> windowsMap;

	WindowManager(sf::RenderWindow*, sf::View*);

	GameWindow* add_window(GameWindow* window, const std::string& id, bool setCurrent=false);

	GameWindow* get_current();

	SettingsDefaults& get_default_settings();

	GameSettings& get_settings();

	GameWindow* get_window_from_id(const std::string&) const;

	std::vector< GameWindow* >& get_windows();

	bool change_window() const;

	void close();

	void change_window(GameWindow* other);

	bool set_settings_data(GameSettings&);

	bool apply_settings();

	bool set_default_settings(const SettingsDefaults&);

};

