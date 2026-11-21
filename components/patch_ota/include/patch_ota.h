// Firmware updates, independent of Patch:
//  - the device checks a release manifest (default: this repo's latest GitHub
//    release) every few hours and installs a newer version, and
//  - anything on the local network can push a firmware file to
//    http://<device>/update with the device's token as a bearer.
#ifndef PATCH_OTA_H
#define PATCH_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

// `update_url` may be empty to turn release checks off. `token` guards the
// push endpoint and must stay valid for the life of the program.
void patch_ota_start(const char *running_version, const char *update_url, const char *token,
                     const char *hostname);

// Check the release manifest now (also runs on a timer).
void patch_ota_check_now(void);

// Network diagnostics: keep recent log lines in memory for GET /log, and run
// POST /console bodies through the same handler as the USB console.
void patch_ota_log_capture_start(void);
void patch_ota_set_console_handler(void (*handler)(const char *line));

#ifdef __cplusplus
}
#endif

#endif
