#include "gamepad-manager.hpp"
#include <obs-module.h>
#include <util/platform.h>
#include <sstream>
#include <iomanip>

static std::string GetGuidString(SDL_JoystickGUID guid) {
    char buf[33];
    SDL_JoystickGetGUIDString(guid, buf, sizeof(buf));
    return std::string(buf);
}

GamepadManager::GamepadManager() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_JOYSTICK) != 0) {
        blog(LOG_ERROR, "[Gamepad Monitor] SDL_Init Error: %s", SDL_GetError());
    }
}

GamepadManager::~GamepadManager() {
    StopPolling();
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    for (auto& dev : devices) {
        if (dev->controller) {
            SDL_GameControllerClose(dev->controller);
        }
    }
    SDL_Quit();
}

GamepadManager& GamepadManager::Get() {
    static GamepadManager instance;
    return instance;
}

GamepadManager& GetGamepadManager() {
    return GamepadManager::Get();
}

void GamepadManager::RefreshDevices() {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    
    // Force SDL to update device list
    SDL_GameControllerUpdate();
    SDL_JoystickUpdate();

    // Mark all as disconnected (instanceId = -1)
    for (auto& dev : devices) {
        dev->instanceId = -1;
    }

    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
            std::string guidStr = GetGuidString(guid);
            const char* controllerName = SDL_GameControllerNameForIndex(i);
            std::string name = controllerName ? controllerName : "Unknown Controller";

            auto it = std::find_if(devices.begin(), devices.end(), [&](const GamepadDevicePtr& d) {
                return d->guid == guidStr;
            });

            GamepadDevicePtr dev;
            if (it != devices.end()) {
                dev = *it;
                // Update name if it was empty (e.g. loaded from config)
                if (dev->name.empty() || dev->name == "Unknown Controller") {
                    dev->name = name;
                }
            } else {
                dev = std::make_shared<GamepadDevice>();
                dev->guid = guidStr;
                dev->name = name;
                dev->alias = name; // Default alias
                devices.push_back(dev);
            }

            SDL_JoystickID instanceId = SDL_JoystickGetDeviceInstanceID(i);
            dev->instanceId = (int)instanceId;

            if (dev->enabled && !dev->controller) {
                dev->controller = SDL_GameControllerOpen(i);
                if (dev->controller) {
                    blog(LOG_INFO, "[Gamepad Monitor] Opened controller: %s (%s)", dev->name.c_str(), dev->guid.c_str());
                }
            }
        }
    }

    // Close controllers that are no longer present or disabled
    for (auto& dev : devices) {
        if (dev->controller && (dev->instanceId == -1 || !dev->enabled)) {
            SDL_GameControllerClose(dev->controller);
            dev->controller = nullptr;
            blog(LOG_INFO, "[Gamepad Monitor] Closed controller: %s", dev->name.c_str());
        }
    }
}

std::vector<GamepadDevicePtr> GamepadManager::GetDevices() {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    return devices;
}

void GamepadManager::SetDeviceEnabled(const std::string& guid, bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    for (auto& dev : devices) {
        if (dev->guid == guid) {
            dev->enabled = enabled;
            RefreshDevices(); // Re-open or close
            break;
        }
    }
}

void GamepadManager::SetDeviceAlias(const std::string& guid, const std::string& alias) {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    for (auto& dev : devices) {
        if (dev->guid == guid) {
            dev->alias = alias;
            break;
        }
    }
}

void GamepadManager::StartPolling() {
    if (polling) return;
    polling = true;
    pollThread = std::thread(&GamepadManager::PollThread, this);
}

void GamepadManager::StopPolling() {
    polling = false;
    if (pollThread.joinable()) {
        pollThread.join();
    }
}

void GamepadManager::PollThread() {
    while (polling) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            HandleControllerEvent(event);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GamepadManager::HandleControllerEvent(const SDL_Event& event) {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    
    int instanceId = -1;
    if (event.type == SDL_CONTROLLERAXISMOTION) instanceId = event.caxis.which;
    else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) instanceId = event.cbutton.which;
    else if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) {
        RefreshDevices();
        return;
    }

    if (instanceId == -1) return;

    auto it = std::find_if(devices.begin(), devices.end(), [&](const GamepadDevicePtr& d) {
        return d->instanceId == instanceId;
    });

    if (it != devices.end()) {
        GamepadDevicePtr dev = *it;
        if (!dev->enabled) return;

        bool changed = false;
        if (event.type == SDL_CONTROLLERAXISMOTION) {
            int axis = event.caxis.axis;
            int value = event.caxis.value;
            if (dev->axisState[axis] != value) {
                dev->axisState[axis] = value;
                changed = true;
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            int button = event.cbutton.button;
            bool pressed = (event.cbutton.state == SDL_PRESSED);
            if (dev->buttonState[button] != pressed) {
                dev->buttonState[button] = pressed;
                changed = true;
            }
        }

        if (changed) {
            EmitState(dev);
        }
    }
}

void GamepadManager::EmitState(GamepadDevicePtr dev) {
    obs_data_t* data = obs_data_create();
    obs_data_set_string(data, "type", "gamepad_input");
    obs_data_set_string(data, "alias", dev->alias.c_str());
    obs_data_set_string(data, "guid", dev->guid.c_str());

    obs_data_t* axes = obs_data_create();
    for (auto const& [axis, val] : dev->axisState) {
        std::string key = "axis_" + std::to_string(axis);
        obs_data_set_double(axes, key.c_str(), val / 32767.0);
    }
    obs_data_set_obj(data, "axes", axes);
    obs_data_release(axes);

    obs_data_t* buttons = obs_data_create();
    for (auto const& [button, pressed] : dev->buttonState) {
        std::string key = "btn_" + std::to_string(button);
        obs_data_set_bool(buttons, key.c_str(), pressed);
    }
    obs_data_set_obj(data, "buttons", buttons);
    obs_data_release(buttons);

    if (messageCallback) {
        const char* json = obs_data_get_json(data);
        messageCallback(dev->alias, json);
    }

    obs_data_release(data);
}

bool GamepadManager::Rumble(const std::string& identifier, uint16_t lowFreq, uint16_t highFreq, uint32_t durationMs) {
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    
    for (auto& dev : devices) {
        if (dev->controller && (dev->alias == identifier || dev->guid == identifier)) {
            // Try GameController Rumble first
            if (SDL_GameControllerRumble(dev->controller, lowFreq, highFreq, durationMs) == 0) {
                return true;
            } else {
                // Fallback to Joystick Rumble
                SDL_Joystick* j = SDL_GameControllerGetJoystick(dev->controller);
                if (j && SDL_JoystickRumble(j, lowFreq, highFreq, durationMs) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void GamepadManager::LoadConfig() {
    char* configPath = obs_module_config_path("gamepad-monitor-settings.json");
    if (!configPath) return;

    obs_data_t* data = obs_data_create_from_json_file(configPath);
    bfree(configPath);

    if (data) {
        obs_data_array_t* devArray = obs_data_get_array(data, "devices");
        if (devArray) {
            std::lock_guard<std::recursive_mutex> lock(devicesMutex);
            size_t count = obs_data_array_count(devArray);
            for (size_t i = 0; i < count; i++) {
                obs_data_t* devData = obs_data_array_item(devArray, i);
                std::string guid = obs_data_get_string(devData, "guid");
                std::string alias = obs_data_get_string(devData, "alias");
                bool enabled = obs_data_get_bool(devData, "enabled");

                auto it = std::find_if(devices.begin(), devices.end(), [&](const GamepadDevicePtr& d) {
                    return d->guid == guid;
                });

                if (it != devices.end()) {
                    (*it)->alias = alias;
                    (*it)->enabled = enabled;
                } else {
                    GamepadDevicePtr dev = std::make_shared<GamepadDevice>();
                    dev->guid = guid;
                    dev->alias = alias;
                    dev->enabled = enabled;
                    devices.push_back(dev);
                }
                obs_data_release(devData);
            }
            obs_data_array_release(devArray);
        }
        obs_data_release(data);
    }
    RefreshDevices();
}

void GamepadManager::SaveConfig() {
    char* configPath = obs_module_config_path("gamepad-monitor-settings.json");
    if (!configPath) return;

    // Ensure directory exists
    std::string path(configPath);
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dir = path.substr(0, lastSlash);
        os_mkdir(dir.c_str());
    }

    obs_data_t* data = obs_data_create();
    obs_data_array_t* devArray = obs_data_array_create();
    
    std::lock_guard<std::recursive_mutex> lock(devicesMutex);
    for (const auto& dev : devices) {
        obs_data_t* devData = obs_data_create();
        obs_data_set_string(devData, "guid", dev->guid.c_str());
        obs_data_set_string(devData, "alias", dev->alias.c_str());
        obs_data_set_bool(devData, "enabled", dev->enabled);
        obs_data_array_push_back(devArray, devData);
        obs_data_release(devData);
    }
    obs_data_set_array(data, "devices", devArray);
    obs_data_array_release(devArray);

    obs_data_save_json(data, configPath);
    obs_data_release(data);
    bfree(configPath);
}
