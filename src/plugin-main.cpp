#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <QPointer>
#include <QTimer>

#include "gamepad-manager.hpp"
#include "gamepad-settings-dialog.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("gamepad-monitor", "en-US")

static QPointer<GamepadSettingsDialog> settingsDialog;

// Transmit gamepad data via OBS signal
void HandleGamepadToSignal(const std::string &alias, const std::string &jsonString)
{
	signal_handler_t *sh = obs_get_signal_handler();
	if (sh) {
		obs_data_t *packet = obs_data_create_from_json(jsonString.c_str());
		if (packet) {
			obs_data_set_string(packet, "a", "gamepad_input");

			calldata_t cd = {0};
			calldata_set_ptr(&cd, "packet", packet);

			// 1. Send to general 'gamepad' topic
			obs_data_set_string(packet, "t", "gamepad");
			signal_handler_signal(sh, "media_warp_transmit", &cd);

			// 2. Send to device-specific 'gamepad/[alias]' topic
			std::string topic = "gamepad/" + alias;
			obs_data_set_string(packet, "t", topic.c_str());
			signal_handler_signal(sh, "media_warp_transmit", &cd);

			obs_data_release(packet);
		}
	}
}

// Receive signals from other plugins (e.g., rumble from browser)
static void on_media_warp_receive(void *data, calldata_t *cd)
{
	(void)data;
	const char *json_str = calldata_string(cd, "json_str");
	if (!json_str)
		return;

	obs_data_t *obj = obs_data_create_from_json(json_str);
	if (!obj)
		return;

	const char *type = obs_data_get_string(obj, "type");
	if (type && strcmp(type, "rumble") == 0) {
		const char *gamepad = obs_data_get_string(obj, "gamepad");
		double low = obs_data_get_double(obj, "low");
		double high = obs_data_get_double(obj, "high");
		int duration = (int)obs_data_get_int(obj, "duration");

		blog(LOG_INFO, "[Gamepad Monitor] Received rumble request for '%s' (low: %.2f, high: %.2f, dur: %dms)", 
			gamepad ? gamepad : "NULL", low, high, duration);

		if (gamepad) {
			bool success = GetGamepadManager().Rumble(gamepad, (uint16_t)(low * 65535), (uint16_t)(high * 65535),
						   (uint32_t)duration);
			blog(LOG_INFO, "[Gamepad Monitor] Rumble execution %s", success ? "SUCCESS" : "FAILED (Gamepad not found or doesn't support rumble)");
		}
	}

	obs_data_release(obj);
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Gamepad Monitor] Plugin loading...");

	auto &mgr = GetGamepadManager();
	mgr.LoadConfig();
	mgr.SetMessageCallback(HandleGamepadToSignal);
	mgr.StartPolling();

	// Connect to receive signal
	signal_handler_t *sh = obs_get_signal_handler();
	if (sh) {
		signal_handler_add(sh, "void media_warp_receive(string json_str)");
		signal_handler_connect(sh, "media_warp_receive", on_media_warp_receive, nullptr);
	}

	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void * /*param*/) {
			if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
				QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(
					"Gamepad Monitor Settings");
				QObject::connect(action, &QAction::triggered, []() {
					if (!settingsDialog) {
						settingsDialog = new GamepadSettingsDialog(
							(QWidget *)obs_frontend_get_main_window());
						settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
					}
					settingsDialog->show();
					settingsDialog->raise();
					settingsDialog->activateWindow();
				});
			}
		},
		nullptr);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Gamepad Monitor] Unloading...");

	if (settingsDialog) {
		delete settingsDialog;
	}

	signal_handler_t *sh = obs_get_signal_handler();
	if (sh) {
		signal_handler_disconnect(sh, "media_warp_receive", on_media_warp_receive, nullptr);
	}

	auto &mgr = GetGamepadManager();
	mgr.StopPolling();
	mgr.SaveConfig();
}
