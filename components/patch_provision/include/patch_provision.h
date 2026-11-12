// What the device needs to reach Patch: Wi-Fi (kept by the Wi-Fi driver), the
// server address and a device token. All three are set over the USB console
// (see main.c `set ...` commands), usually by the web setup page.
#ifndef PATCH_PROVISION_H
#define PATCH_PROVISION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char server_url[160];   // e.g. https://patch.example.com
    char server_host[128];  // parsed from server_url
    int  server_port;
    bool tls;
    char token[1024];       // device token from Patch (Settings -> Voice devices)
    char device_id[64];     // the token's surface_id
    char update_url[256];   // release manifest the device checks for updates
} patch_provision_state_t;

// Load what's stored. Returns true only when the server and a usable token
// are both present (Wi-Fi is checked separately).
bool patch_provision_load(patch_provision_state_t *out);

// Store one setting: "server", "token" or "update-url". Validates the value.
bool patch_provision_set(const char *key, const char *value);

// Wi-Fi network to join. Stored by the Wi-Fi driver; takes effect on reboot.
bool patch_provision_set_wifi(const char *ssid, const char *password);

bool patch_provision_wifi_configured(void);

// Forget the server and token (Wi-Fi is kept).
void patch_provision_factory_reset(void);

#ifdef __cplusplus
}
#endif

#endif
