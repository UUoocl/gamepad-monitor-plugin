#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <SDL.h>

struct GamepadDevice {
    std::string guid;
    std::string name;
    std::string alias;
    bool enabled = false;
    int instanceId = -1;
    SDL_GameController* controller = nullptr;
    
    // Last state for change detection
    std::map<int, int> axisState;
    std::map<int, bool> buttonState;
};

using GamepadDevicePtr = std::shared_ptr<GamepadDevice>;

class GamepadManager {
public:
    GamepadManager();
    ~GamepadManager();

    static GamepadManager& Get();

    void RefreshDevices();
    std::vector<GamepadDevicePtr> GetDevices();
    
    void SetDeviceEnabled(const std::string& guid, bool enabled);
    void SetDeviceAlias(const std::string& guid, const std::string& alias);

    void StartPolling();
    void StopPolling();

    bool Rumble(const std::string& identifier, uint16_t lowFreq, uint16_t highFreq, uint32_t durationMs);

    void LoadConfig();
    void SaveConfig();

    using MessageCallback = std::function<void(const std::string& alias, const std::string& jsonString)>;
    void SetMessageCallback(MessageCallback cb) { messageCallback = cb; }

private:
    void PollThread();
    void HandleControllerEvent(const SDL_Event& event);
    void EmitState(GamepadDevicePtr dev);

    std::recursive_mutex devicesMutex;
    std::vector<GamepadDevicePtr> devices;
    
    std::atomic<bool> polling{false};
    std::thread pollThread;
    
    MessageCallback messageCallback;
};

GamepadManager& GetGamepadManager();
