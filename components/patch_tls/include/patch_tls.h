// TLS certificate checking for every HTTPS/WSS connection the device makes.
#ifndef PATCH_TLS_H
#define PATCH_TLS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Use as `crt_bundle_attach`. The same as esp_crt_bundle_attach, except that a
// certificate whose subject and public key match a root in the bundle counts
// as trusted however it is signed. Servers commonly send a trusted root
// cross-signed by an older root (Let's Encrypt's ISRG Root X2 signed by X1,
// which expired in 2025); browsers stop at the trusted root, and so does this.
esp_err_t patch_tls_attach(void *conf);

#ifdef __cplusplus
}
#endif

#endif
