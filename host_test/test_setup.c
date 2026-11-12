#include "test_helpers.h"
#include "patch_setup.h"

#include <stdio.h>
#include <string.h>

static void test_server_url(void) {
    char host[64]; int port = 0; bool tls = false;
    ASSERT_TRUE(psu_parse_server_url("https://patch.example.com", host, sizeof host, &port, &tls));
    ASSERT_TRUE(strcmp(host, "patch.example.com") == 0);
    ASSERT_EQ_INT(port, 443); ASSERT_TRUE(tls);
    ASSERT_TRUE(psu_parse_server_url("http://192.168.1.5:3970/", host, sizeof host, &port, &tls));
    ASSERT_TRUE(strcmp(host, "192.168.1.5") == 0);
    ASSERT_EQ_INT(port, 3970); ASSERT_TRUE(!tls);
    ASSERT_TRUE(!psu_parse_server_url("patch.example.com", host, sizeof host, &port, &tls));
    ASSERT_TRUE(!psu_parse_server_url("https://", host, sizeof host, &port, &tls));
    ASSERT_TRUE(!psu_parse_server_url("https://h/path", host, sizeof host, &port, &tls));
    ASSERT_TRUE(!psu_parse_server_url("https://h:0", host, sizeof host, &port, &tls));
    ASSERT_TRUE(!psu_parse_server_url("https://h:99999", host, sizeof host, &port, &tls));
}

static void test_token_device_id(void) {
    // header.payload.sig where payload = {"surface_id":"01ABC","surface_kind":"voice-device"}
    const char *jwt = "eyJhbGciOiJFZERTQSJ9.eyJzdXJmYWNlX2lkIjoiMDFBQkMiLCJzdXJmYWNlX2tpbmQiOiJ2b2ljZS1kZXZpY2UifQ.c2ln";
    char id[32];
    ASSERT_TRUE(psu_token_device_id(jwt, id, sizeof id));
    ASSERT_TRUE(strcmp(id, "01ABC") == 0);
    ASSERT_TRUE(!psu_token_device_id("not-a-jwt", id, sizeof id));
    ASSERT_TRUE(!psu_token_device_id("a.eyJmb28iOiJiYXIifQ.c", id, sizeof id)); // no surface_id
    char tiny[3];
    ASSERT_TRUE(!psu_token_device_id(jwt, tiny, sizeof tiny));
}

static void test_version_cmp(void) {
    ASSERT_TRUE(psu_version_cmp("v1.2.3", "1.2.3") == 0);
    ASSERT_TRUE(psu_version_cmp("v1.2.3", "v1.2.4") < 0);
    ASSERT_TRUE(psu_version_cmp("v1.10.0", "v1.9.9") > 0);
    ASSERT_TRUE(psu_version_cmp("v2.0.0", "v1.99.99") > 0);
    ASSERT_TRUE(psu_version_cmp("v0.3.0-4-gabc", "v0.3.0") == 0);
    ASSERT_TRUE(psu_version_cmp("garbage", "v0.0.1") < 0);
}

int main(void) {
    fprintf(stderr, "test_setup\n");
    TEST_RUN(test_server_url);
    TEST_RUN(test_token_device_id);
    TEST_RUN(test_version_cmp);
    return 0;
}
