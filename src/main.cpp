

#define ENABLE_GAME

#ifdef ENABLE_GAME

#include "window/window_gameplay.hpp"
#include "window/window_manager.hpp"



typedef std::chrono::high_resolution_clock chrono_namespace;
typedef std::chrono::time_point<chrono_namespace> chrono_clock;
typedef std::chrono::duration<float> float_duration;

constexpr size_t LOGIC_DELAY = 1;

static sf::Clock sGlobalClock = sf::Clock();
static chrono_clock sChronoStart = chrono_namespace::now(), sChronoCurrent;

constexpr int MIN_CLICK_DISTANCE = 16;
constexpr t_seconds FOCUS_TIME = 0.5f;


int game_main()
{

	// Initialize sfml essentials
	sf::VideoMode videoMode;
#if MODILE_DEVICE
	videoMode = sf::VideoMode::getDesktopMode();
#else
	videoMode = sf::VideoMode({1920, 1080});
#endif
	sf::RenderWindow window(videoMode, "");
	sf::View view = window.getDefaultView();
	window.setVerticalSyncEnabled(true);

	// Make a manager for windows
	WindowManager manager{ &window, &view };
	manager.windowName = "Isometric Game";
	manager.init_manager();
	if (!manager.get_current())
	{
		LOG_ERROR("No default window set. Exiting!");
		return EXIT_FAILURE;
	}

	// Load the data needed for the settings
	nlohmann::json jsonSettings, jsonDefaults;
	std::string pathSettings, pathDefaults;
	pathSettings = "saves/settings.json";
	pathDefaults = "assets/json/settings_hard.json";

	// Load the default settings
	if (!json_file_load(jsonDefaults, pathDefaults))
	{
		DEBUG("Unable to load defaultSettings from %s", pathDefaults.c_str());
		return EXIT_FAILURE;
	}
	// Convert defaults json to SettingsDefaults
	SettingsDefaults settingDefaults;
	if (!json_to_default_settings(settingDefaults, jsonDefaults))
	{
		DEBUG("Unable to convert jsonDefaults to settingDefaults");
		return EXIT_FAILURE;
	}
	// Set the settings
	manager.set_default_settings(settingDefaults);

	// Load the saved settings
	GameSettings intervals;
	// If the loading failed, save the default settings
	if (!json_file_load(jsonSettings, pathSettings))
	{
		assert(json_settings_save(
			jsonSettings["settings"], 
			manager.get_default_settings().settings));
		
		json_file_write(
			jsonSettings,
			pathSettings);

		manager.set_settings_data(settingDefaults.settings);
	}
	else
	{
		// If found the settings json
		
		if (!jsonSettings.count("settings"))
		{
			DEBUG("Unable to get settings key in %s", pathSettings.c_str());
			return EXIT_FAILURE;
		}

		nlohmann::json jSetting = jsonSettings["settings"];

		// Convert settings json to GameSettings
		GameSettings settings;
		if (!json_settings_load(jSetting, settings, settingDefaults))
		{
			DEBUG("Unable to convert jsonSettings to settings");
			return EXIT_FAILURE;
		}
		// Set the settings
		manager.set_settings_data(settings);
	}

	// Initialize all game windows
	for (GameWindow* gameWindow : manager.get_windows())
	{
		gameWindow->init_window(&window, &view);

		if (!gameWindow->load())
			return EXIT_FAILURE;

		if (!gameWindow->init())
			return EXIT_FAILURE;
	}

	GameWindow* game = manager.get_current();
	
	bool enableLogic = true;
	
	int mouseFrames = 0;
	int focusFrames = 0;
	sf::Vector2i mousePressPos;
	bool mouseMoved = false;
	bool pitchMoved = false;
	int mouseDist = -1, pmouseDist = -1;
	bool isFocus = false;
	sf::Vector2i mousePos, pmousePos;
	sf::Vector2i pPitchCenter;
	
	sf::Clock frameClock, deltaClock;
	sf::Clock focusTimer;
	sGlobalClock.restart();

	size_t mainLoopFrame = 0;
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
	
	start = std::chrono::high_resolution_clock::now();
	while (window.isOpen())
	{
		// Change the window if needed

		if (manager.change_window())
		{
			game = manager.get_current();
		}

		// Event handling
		while (const std::optional<sf::Event> event = window.pollEvent()) 
		{
			game->gui.handleEvent(*event);

			if (const auto* resized = event->getIf<sf::Event::Resized>()) 
			{
				view.setSize({(float)resized->size.x, (float)resized->size.y});
				view.setCenter({(float)resized->size.x / 2.f, (float)resized->size.y / 2.f});
			}

			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if (keyPressed->code == sf::Keyboard::Key::Escape)
					window.close();
			}

			if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>())
			{
				game->mouse_zoom(scrolled->position, (int)(scrolled->delta * 50));
			}

			if (event->is<sf::Event::FocusLost>())
			{
				enableLogic = false;
				game->on_focus();
			}

			if (event->is<sf::Event::FocusGained>())
			{
				enableLogic = true;
				game->on_unfocus();
			}

			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}

			if (event->is<sf::Event::MouseButtonPressed>())
			{
				//LOG("%d %d\n", event.mouseButton.x, event.mouseButton.y);
				break;
			}
		}

		if (!enableLogic)
			continue;

		bool isDown = false;
#if MODILE_DEVICE
		isDown = sf::Touch::isDown(0);
		if (isDown)
			mousePos = sf::Touch::getPosition(0);
#else
		isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
		mousePos = sf::Mouse::getPosition(window);
#endif

		game->mousePos = mousePos;
		game->mousePressed = isDown;

		// For touch controlls, is zooming was made with
		// two fingers, don't calulate button presses.
		static bool enablePress = true;
		float focusTime = focusTimer.getElapsedTime().asSeconds();
		if (isDown && game->gui.getFocusedChild() == nullptr)
		{
			if (sf::Touch::isDown(1))
			{
				enablePress = false;
				sf::Vector2i pitchCenter =
					(mousePos + sf::Touch::getPosition(1)) / 2;
				mouseDist = math_sqrt(vec_distsq<int>(sf::Touch::getPosition(0), sf::Touch::getPosition(1)));
				if (mouseDist != pmouseDist && pmouseDist != -1)
				{
					if (pmouseDist != -1)
						game->mouse_zoom(pitchCenter, mouseDist - pmouseDist);
				}
				pmouseDist = mouseDist;
				if (pPitchCenter != pitchCenter &&
					pitchMoved)
				{
					game->mouse_zoom_dragged(pitchCenter, pPitchCenter);
				}
				pPitchCenter = pitchCenter;
				pitchMoved = true;
			}
			else
			{
				pmouseDist = -1;

				if (mouseFrames == 0)
				{
					mousePressPos = mousePos;
					game->mouse_start(mousePos);
					focusTimer.restart();
					isFocus = true;
				}

				if (mousePos != pmousePos &&
					mouseMoved &&
					(!isFocus || focusTime < FOCUS_TIME))
				{
					game->mouse_dragged(mousePos, pmousePos);
					isFocus = false;
				}

				if (isFocus &&
					focusTime >= FOCUS_TIME)
				{
					if (focusFrames == 0)
					{
						game->mouse_focus_start(mousePressPos);
					}
					else
					{
						game->mouse_focus(mousePressPos, mousePos);
					}
					focusFrames++;
				}
				else
					focusFrames = 0;

				mouseMoved = true;
				mouseFrames++;
			}
		}
		else
		{
			if (mouseFrames > 0)
			{
				if (enablePress)
				{
					game->mouse_released(mousePos);
					int d = vec_distsq<int>(mousePos, mousePressPos);
					if (
						d < MIN_CLICK_DISTANCE)
					{
						game->mouse_pressed(mousePressPos);
					}
				}

				if (isFocus &&
					focusFrames > 0 &&
					focusTime >= FOCUS_TIME)
				{
					game->mouse_focus_end(mousePressPos, mousePos);
				}
			}

			enablePress = true;
			mouseFrames = 0;
			mouseMoved = false;
			pitchMoved = false;
			pmouseDist = -1;
			focusFrames = 0;
		}

		{
			game->time = sGlobalClock.getElapsedTime().asSeconds();
			game->update(deltaClock.getElapsedTime().asSeconds());
		}
		deltaClock.restart();


		window.clear(sf::Color::White);
		game->render();
		window.display();



		/*
		if (game.data.frameCount % 30 == 29)
		{
			float x = 30.0f / frameClock.getElapsedTime().asSeconds();
			game.fps = floor(x * 10.f) / 10.f;
			frameClock.restart();
		}
		*/

		if (mainLoopFrame % 30 == 29)
		{
			end = std::chrono::high_resolution_clock::now();
			float fps1 = (float)1e9 / (float)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
			fps1 *= 30;
			if (game->fps == -1.f)
				game->fps = fps1;
			else
				game->fps = 0.7f * fps1 + 0.3f * game->fps;
			start = std::chrono::high_resolution_clock::now();
		}

		pmousePos = mousePos;
		++mainLoopFrame;
	}

	manager.close();

	return EXIT_SUCCESS;

}

t_seconds GameData::get_time()
{
	sChronoCurrent = chrono_namespace::now();
	return std::chrono::duration_cast<float_duration>(
		sChronoCurrent - sChronoStart)
		.count();
}

//
int main(int argc, char* argv[])
{
	LOG("Args: %s", array_to_string(argv, argc).c_str());

	game_main();
}

#else // ENABLE_GAME

int main()
{
}

#endif // ENABLE_GAME