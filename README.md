# Patch Voice PE

Firmware that turns a [Home Assistant Voice Preview Edition](https://www.home-assistant.io/voice-pe/) into a voice device for [Patch](https://github.com/patch-hq/patch). Say "Hey Jarvis", talk, and Patch answers out loud. Patch does all the thinking; the device listens, plays the reply and shows what is happening on its light ring.

It speaks Patch's generic voice-device endpoint (Patch `spec/16-voice-device.md`). Nothing here is needed by Patch itself, and the firmware has its own releases.

## Setting one up

1. In Patch, open Settings → Voice → Voice devices → **Add device**. Copy the server address and token it shows.
2. Plug the Voice PE into a computer and open the setup page, <https://patch-hq.github.io/patch-voice-pe/>, in Chrome or Edge.
3. Install the firmware, then enter your Wi-Fi network, the server address and the token.

The device restarts and connects. To remove it, revoke it in the same Patch settings list.

## Using it

| Light                          | Meaning                                   |
| ------------------------------ | ----------------------------------------- |
| Off                            | Waiting for "Hey Jarvis"                  |
| One white flash                | Heard the wake word                       |
| Solid white                    | Listening                                 |
| Spinning white                 | Thinking                                  |
| White glow following the voice | Speaking                                  |
| White ring draining away       | Open for a follow-up, no wake word needed |
| Solid red                      | Muted (switch on the side)                |
| Slow red pulse                 | Can't reach Patch                         |
| Blue spinning                  | Not set up yet                            |

- Say "Hey Jarvis" or press the button while it is speaking to interrupt it.
- Turn the dial to change the volume (0 to 10, remembered across restarts).
- Hold the button for 10 seconds to forget the server and token.

## Updates

The device checks this repo's latest release every six hours and installs anything newer. To push a build yourself from any machine on the same network:

```sh
curl -H "Authorization: Bearer <device token>" --data-binary @patch-voice-pe.bin \
  http://patch-voice-XXXXXX.local/update
```

`XXXXXX` is the end of the device's MAC address (its console prints the name at boot). `GET /info` returns the running version. To follow a different release feed, set `update-url` over the console (below).

## USB console

The device's USB serial console (115200 baud) takes these lines, which is what the setup page sends:

```
set wifi <network>
set wifi-password <password>
set server <https://patch.example.com>
set token <device token>
set update-url <manifest url>
apply            save and restart
status           prints a SETUP {...} line
forget           forget the server and token
update-check     check for an update now
```

There are also hardware diagnostics (`tone`, `speech`, `vol <0-10>`, `wake`, `mute on|off`); see `main/main.c`.

## Building

ESP-IDF v5.3.2:

```sh
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodem* flash monitor
```

Flashing only the app (`0x20000`) keeps the device's settings. Host tests for the pure-C parts:

```sh
cmake -S host_test -B /tmp/ht && cmake --build /tmp/ht && ctest --test-dir /tmp/ht
```

Pushing a `v*` tag builds the release (`patch-voice-pe.bin`, a full factory image and `manifest.json`) and publishes the setup page.

## Hardware notes

`HARDWARE.md` records what was learned bringing up the board's audio path: the speaker bus clocking, the codec setup, the amplifier and the echo levels. Read it before changing anything in `components/patch_audio` or `components/patch_codec`.
