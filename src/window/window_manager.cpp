#include "window/window_manager.hpp"

#include "window/window_gameplay.hpp"
#include "window/window_menu.hpp"

WindowManager::WindowManager(sf::RenderWindow* window, sf::View* view)
{
	this->window = window;
	this->view = view;
}

GameWindow *WindowManager::add_window(GameWindow *window, const std::string &id, bool setCurrent)
{
	windows.push_back(window);

	if (this->windowsMap.count(id))
		return nullptr;
	this->windowsMap[id] = windows.back();
    window->managerParent = this;

	if (setCurrent)
		current = window;

	return window;
}

GameWindow* WindowManager::get_current()
{
	return this->current;
}

SettingsDefaults& WindowManager::get_default_settings()
{
	return this->defaultSettings;
}

GameSettings& WindowManager::get_settings()
{
	return this->settings;
}

GameWindow* WindowManager::get_window_from_id(const std::string& str) const
{
	auto itr = windowsMap.find(str);
	if (itr == windowsMap.end())
		return nullptr;
	return (*itr).second;
}

std::vector<GameWindow*>& WindowManager::get_windows()
{
	return windows;
}

bool WindowManager::change_window() const
{
	return this->changeWindow;
}

void WindowManager::close()
{
	for (GameWindow* window : windows)
	{
		if (window)
			window->close();
	}
}

void WindowManager::change_window(GameWindow* other)
{
	if (other != this->current)
	{
		this->current = other;
		changeWindow = true;
	}
}

bool WindowManager::set_settings_data(GameSettings& settings)
{
	this->settings = settings;

	return true;
}

bool WindowManager::apply_settings()
{
	unsigned x = (unsigned)settings.resolution.x; 
	unsigned y = (unsigned)settings.resolution.y;

	window->close();
	if (settings.isFullscreen)
	{

		const sf::VideoMode& mode = sf::VideoMode::getDesktopMode();
		x = mode.size.x;
		y = mode.size.y;

		// sf::Style::Fullscreen
		window->create(sf::VideoMode({x, y}), windowName, sf::State::Fullscreen);
	}
	else
	{
		window->create(sf::VideoMode({x, y}), windowName, sf::State::Windowed);
	}

	for (GameWindow* gameWindow : windows)
	{
		gameWindow->gui.setTarget(*window);
	}

	window->setVerticalSyncEnabled(settings.enableVSync);

	return true;
}

#define QUICK_FIND(container,strId,TYPE) \
(std::find_if(container.begin(), container.end(), [strId](const TYPE& a) \
{ \
	return a.title == strId; \
}))



bool WindowManager::set_default_settings(const SettingsDefaults& defaults)
{
	this->defaultSettings = defaults;

	return true;

}

bool json_settings_load(
	const nlohmann::json& jSettings, 
	GameSettings& out, 
	const SettingsDefaults& ds)
{
	GameSettings settings;

	DEBUG("%s", jSettings.dump(4).c_str());

	try {
		jSettings.at("volume_ui").get_to(settings.volumeUI);
		jSettings.at("volume_music").get_to(settings.volumeMusic);
		jSettings.at("volume_game").get_to(settings.volumeGame);

		std::string qualityStr;
		jSettings.at("graphics_quality").get_to(qualityStr);
		auto itrQ = QUICK_FIND(ds.qualities, qualityStr, SettingsQuality);
		if (itrQ != ds.qualities.end())
			settings.quality = *itrQ;
		else
		{
			WARNING(
				"Error when parsing settings."
				" Can't find quality %s",
				qualityStr.c_str());
			return false;
		}



		std::string resolutionStr;
		jSettings.at("resolution").get_to(resolutionStr);
		auto itrR = QUICK_FIND(ds.resolutions, resolutionStr, SettingsResolution);
		if (itrR != ds.resolutions.end())
			settings.resolution = *itrR;
		else
		{
			WARNING(
				"Error when parsing settings."
				" Can't find resolution %s",
				resolutionStr.c_str());
			return false;
		}



		std::string intervalStr;
		jSettings.at("autosave_interval").get_to(intervalStr);
		auto itrS = QUICK_FIND(ds.intervals, intervalStr, SettingsAutosaveInterval);
		if (itrS != ds.intervals.end())
			settings.interval = *itrS;
		else
		{
			WARNING(
				"Error when parsing settings."
				" Can't find autosave interval %s",
				intervalStr.c_str());
			return false;
		}

		jSettings.at("is_fullscreen").get_to(settings.isFullscreen);
		jSettings.at("enable_vsync").get_to(settings.enableVSync);

		jSettings.at("ui_scaling").get_to(settings.uiScale);

	}
	catch (const nlohmann::json::exception& e)
	{
		WARNING(
			"Error when parsing settings: %s",
			e.what());
		return false;
	}

	out = settings;

	return true;
}

bool json_settings_save(nlohmann::json& jOut, const GameSettings& settings)
{
	jOut["volume_ui"] = settings.volumeUI;
	jOut["volume_music"] = settings.volumeMusic;
	jOut["volume_game"] = settings.volumeGame;
	jOut["graphics_quality"] = settings.quality.title;
	jOut["resolution"] = settings.resolution.title;
	jOut["is_fullscreen"] = settings.isFullscreen;
	jOut["enable_vsync"] = settings.enableVSync;
	jOut["autosave_interval"] = settings.interval.title;
	jOut["ui_scaling"] = settings.uiScale;

	return true;
}


bool json_to_default_settings(SettingsDefaults& defaultSettings, const nlohmann::json& j)
{
	const nlohmann::json& jRess = j.at("resolutions");
	for (auto& jRes : jRess)
	{
		SettingsResolution res;
		try {
			jRes.at("ratio").at("x").get_to(res.ratioX);
			jRes.at("ratio").at("y").get_to(res.ratioY);

			jRes.at("x").get_to(res.x);
			jRes.at("y").get_to(res.y);
			jRes.at("title").get_to(res.title);
		}
		catch (const nlohmann::json::exception& e)
		{
			WARNING(
				"Error when parsing the default settings: %s",
				e.what());
		}

		defaultSettings.resolutions.push_back(res);
	}

	if (defaultSettings.resolutions.empty())
	{
		WARNING(
			"No resolutions in the default settings was found, aborting.");
		return false;
	}

	const nlohmann::json& jQuants = j.at("quality");
	for (auto& jQuant : jQuants)
	{
		SettingsQuality quan;
		try {
			jQuant.at("title").get_to(quan.title);
			jQuant.at("index").get_to(quan.index);
		}
		catch (const nlohmann::json::exception& e)
		{
			WARNING(
				"Error when parsing the default settings: %s",
				e.what());
		}

		defaultSettings.qualities.push_back(quan);
	}

	if (defaultSettings.qualities.empty())
	{
		WARNING(
			"No quality in the default settings was found, aborting.");
		return false;
	}



	const nlohmann::json& jIntrs = j.at("autosave inerval");
	for (auto& jIntr : jIntrs)
	{
		SettingsAutosaveInterval interval;
		try {
			jIntr.at("title").get_to(interval.title);
			jIntr.at("minutes").get_to(interval.minutes);
		}
		catch (const nlohmann::json::exception& e)
		{
			WARNING(
				"Error when parsing the default settings: %s",
				e.what());
		}

		defaultSettings.intervals.push_back(interval);
	}

	if (defaultSettings.intervals.empty())
	{
		WARNING(
			"No quality in the default settings was found, aborting.");
		return false;
	}

	GameSettings settings;
	if (j.count("defaults"))
	{
		const nlohmann::json& jSettings = j.at("defaults");
		if (!json_settings_load(
			jSettings,
			defaultSettings.settings,
			defaultSettings))
		{
			WARNING("Were no able to read default settings");
			return false;
		}
	}
	else
	{
		WARNING("No defaults object was found in default settings");
		return false;
	}

	return true;
}
