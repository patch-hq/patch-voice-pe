// Pure helpers for device setup and updates. No ESP-IDF dependencies, so the
// same code runs in host tests.
#ifndef PATCH_SETUP_H
#define PATCH_SETUP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Split "https://host[:port][/]" into host, port and TLS. http:// means no TLS
// and port 80 by default; https:// means TLS and 443. Any path is refused.
bool psu_parse_server_url(const char *url, char *host, size_t host_cap, int *port, bool *tls);

// Read the "surface_id" claim from a device token (a JWT) without verifying
// it: the server verifies; the device only needs to know its own id.
bool psu_token_device_id(const char *jwt, char *out, size_t cap);

// Compare two versions like "v1.2.3" or "1.2.3" (a leading "v" and anything
// after a "-" or "+" are ignored). Returns <0, 0 or >0. Unparseable versions
// compare as 0.0.0.
int psu_version_cmp(const char *a, const char *b);

#ifdef __cplusplus
}
#endif

#endif
