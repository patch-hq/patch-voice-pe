#include "patch_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool psu_parse_server_url(const char *url, char *host, size_t host_cap, int *port, bool *tls) {
    if (!url || !host || !port || !tls || host_cap == 0) return false;
    const char *p;
    if (strncmp(url, "https://", 8) == 0) { *tls = true; *port = 443; p = url + 8; }
    else if (strncmp(url, "http://", 7) == 0) { *tls = false; *port = 80; p = url + 7; }
    else return false;
    size_t n = strcspn(p, ":/");
    if (n == 0 || n >= host_cap) return false;
    memcpy(host, p, n);
    host[n] = '\0';
    p += n;
    if (*p == ':') {
        char *end = NULL;
        long v = strtol(p + 1, &end, 10);
        if (end == p + 1 || v < 1 || v > 65535) return false;
        *port = (int)v;
        p = end;
    }
    if (*p == '/') p++;
    return *p == '\0';
}

static int b64url_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-' || c == '+') return 62;
    if (c == '_' || c == '/') return 63;
    return -1;
}

bool psu_token_device_id(const char *jwt, char *out, size_t cap) {
    if (!jwt || !out || cap == 0) return false;
    const char *a = strchr(jwt, '.');
    if (!a) return false;
    const char *b = strchr(a + 1, '.');
    if (!b) return false;
    char payload[1024];
    size_t pn = 0;
    unsigned buf = 0;
    int bits = 0;
    for (const char *p = a + 1; p < b; p++) {
        if (*p == '=') break;
        int v = b64url_val(*p);
        if (v < 0) return false;
        buf = (buf << 6) | (unsigned)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (pn + 1 >= sizeof payload) return false;
            payload[pn++] = (char)((buf >> bits) & 0xff);
        }
    }
    payload[pn] = '\0';
    const char *key = strstr(payload, "\"surface_id\"");
    if (!key) return false;
    const char *q = strchr(key + 12, '"');
    if (!q) return false;
    const char *e = strchr(q + 1, '"');
    if (!e || e == q + 1) return false;
    size_t n = (size_t)(e - q - 1);
    if (n >= cap) return false;
    memcpy(out, q + 1, n);
    out[n] = '\0';
    return true;
}

// Only "vX.Y.Z" or "X.Y.Z" (anything after "-" or "+" ignored) is a version.
// Anything else, such as a bare commit hash from a local build, is 0.0.0, so
// any release counts as newer.
static void parse_ver(const char *s, long v[3]) {
    v[0] = v[1] = v[2] = 0;
    if (!s) return;
    if (*s == 'v' || *s == 'V') s++;
    long got[3];
    for (int i = 0; i < 3; i++) {
        char *end = NULL;
        if (*s < '0' || *s > '9') return;
        got[i] = strtol(s, &end, 10);
        if (i < 2 && *end != '.') return;
        if (i == 2 && *end != '\0' && *end != '-' && *end != '+') return;
        s = end + 1;
    }
    v[0] = got[0]; v[1] = got[1]; v[2] = got[2];
}

int psu_version_cmp(const char *a, const char *b) {
    long va[3], vb[3];
    parse_ver(a, va);
    parse_ver(b, vb);
    for (int i = 0; i < 3; i++) {
        if (va[i] != vb[i]) return va[i] < vb[i] ? -1 : 1;
    }
    return 0;
}
