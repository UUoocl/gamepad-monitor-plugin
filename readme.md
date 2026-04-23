# Gamepad Monitor Plugin

A high-performance OBS Studio plugin for monitoring gamepad input and providing haptic feedback. Part of the **mediaWarp** suite, this plugin bridges physical controllers with the OBS ecosystem and web-based overlays.

## Features

- **Automatic Discovery**: Seamlessly detects and initializes connected gamepads using SDL2.
- **Real-time Input Tracking**: High-frequency monitoring of controller buttons and analog axes.
- **Bidirectional Signal Pipeline**:
    - **Transmit**: Broadcasts gamepad state (buttons, axes, GUIDs) via the `media_warp_transmit` OBS signal.
    - **Receive**: Responds to `rumble` requests from other plugins or browser sources via the `media_warp_receive` signal.
- **Haptic Feedback**: Support for native controller rumble (vibration) with configurable frequency and duration.
- **Device Management**:
    - Enable/Disable specific controllers.
    - Assign custom **Aliases** to controllers for easier routing in downstream applications.
- **Persistent Configuration**: Automatically saves your settings (aliases, enabled states) between OBS sessions.
- **Universal Support**: Built for macOS (Universal Binary) and Windows.

## Tech Overview

The plugin is built with a focus on performance and low-latency input processing:

- **Core**: C++17
- **Input Engine**: SDL2 (Stable Release 2.30.2), integrated via `FetchContent` for consistent cross-platform builds.
- **UI Framework**: Qt6, providing a native settings dialog within the OBS Tools menu.
- **OBS Integration**: Utilizes the `libobs` API and `obs-frontend-api` for deep integration with the OBS signal system and UI.
- **Communication Protocol**: Data is transmitted as JSON packets, making it compatible with WebSocket bridges like the `local-mongoose-webserver`.

## How to Use

### Installation
1. Ensure you have the `gamepad-monitor` binary installed in your OBS plugins directory.
2. Restart OBS Studio.

### Configuration
1. Open OBS and navigate to **Tools** > **Gamepad Monitor Settings**.
2. A list of connected controllers will be displayed.
3. **Enable** the controllers you wish to monitor.
4. (Optional) Set an **Alias** (e.g., "Player1") to make it easier to identify the device in your overlays or scripts.
5. Close the dialog; settings are saved automatically.

### Integration with Overlays
The plugin broadcasts input data using the OBS signal `media_warp_transmit`. To receive this data in a browser-based overlay:
1. Ensure you have the `local-mongoose-webserver` plugin (or a similar WebSocket bridge) active.
2. Listen for JSON messages with `type: "gamepad_input"`.

**Example Outbound Packet:**
```json
{
  "type": "gamepad_input",
  "alias": "Player1",
  "guid": "...",
  "axes": {
    "axis_0": 0.5,
    "axis_1": -0.2
  },
  "buttons": {
    "btn_0": true,
    "btn_1": false
  }
}
```

### Triggering Rumble
To trigger haptic feedback from a browser source, send a JSON command to the `media_warp_receive` signal:
```json
{
  "type": "rumble",
  "gamepad": "Player1",
  "low": 1.0,
  "high": 0.5,
  "duration": 500
}
```
*(Values for `low` and `high` frequencies range from 0.0 to 1.0)*

---

## Disclaimer

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND**, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---
*Developed as part of the **mediaWarp** ecosystem.*

**Made with Antigravity**
