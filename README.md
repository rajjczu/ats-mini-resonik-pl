<img src="docs/resonik-symbol.svg" width="56" alt="RESONIK symbol">

# ATS Mini — RESONIK PL+ Edition (`ats-mini-resonik-pl`)

Zmodyfikowana, polska edycja firmware'u dla kieszonkowego odbiornika **ATS Mini (ESP32-S3 + SI4732, OSPI 8 MB PSRAM / 16 MB Flash)**, zbudowana na bazie otwartoźródłowego projektu **[ep3mhb/resonik-ats-mini](https://github.com/ep3mhb/resonik-ats-mini) (v2.45-r4)** oraz **[esp32-si4732/ats-mini](https://github.com/esp32-si4732/ats-mini)**.

### Co dodano w wersji PL+
* **Przesuwny analizator widma + wodospad na żywo (`Tuning → Scope`, `Scope.h` / `Scope.cpp`):**
  * 61-kanałowe okno widma wycentrowane na aktualnej częstotliwości + 20-liniowy wodospad bezpośrednio na ekranie 320×170.
  * Płynne przesuwanie widma i wodospadu w trakcie kręcenia gałką (`SWEEP`), szybkie przełączanie kliknięciem w podsłuch (`LISTEN`) i powrót do skanowania.
* **Dekoder `RTTY 45.45 Bd` oraz `NAVTEX 100 Bd` (`Ham & SWL → RTTY/NVTX`, `RttyDecoder.h` / `CwAudio.cpp`):**
  * Podwójny filtr Goertzela (Mark 1275 Hz / Space 1445 Hz) ze wskaźnikami dostrojenia `M` i `S` oraz dekodowaniem Baudot ITA2 / SITOR-B.
  * Automatyczne wykrywanie NAVTEX w paśmie 485–525 kHz; kliknięcie przełącza tryb ręcznie.
* **7 dodatkowych pasm (`Menu.cpp`):**
  * `CB-PL` (26.960–27.400 MHz, AM, raster 10 kHz „zera”, start CH19 = 27.180 MHz)
  * `CB-DX` (27.405–27.995 MHz, USB, start 27.555 MHz)
  * `60MH` (5.3515–5.3665 MHz, USB — amatorskie 60 m)
  * `LW-PL` (148–285 kHz, AM, krok 9 kHz, start 225 kHz PR1)
  * `NVTX` (485–525 kHz, USB — morski NAVTEX 490 / 518 kHz)
  * `VOLM` (5.400–11.400 MHz, USB — lotnicze komunikaty meteo HF VOLMET)
  * `TIME` (2.495–15.005 MHz, AM — stacje wzorca czasu RWM / WWV)
* **82 gotowe pamięci startowe (`ats-mini.ino`):**
  * Kraków i Małopolska FM (15 stacji), CB Radio PL/DX (10), lotnictwo HF VOLMET i MWARA EUR/NAT (15), służby meteo RTTY/WEFAX/NAVTEX/czas/UVB-76 (12), fale długie i średnie (6), krótkofalowcy SSB/CW/FT8/Beacons (14) oraz rozgłośnie KF (10).

---

# RESONIK edition of the ATS Mini firmware

[![GitHub downloads](https://img.shields.io/github/downloads/ep3mhb/resonik-ats-mini/total?label=GitHub%20downloads&color=108AB1)](https://github.com/ep3mhb/resonik-ats-mini/releases)
[![Web installs](https://img.shields.io/endpoint?url=https%3A%2F%2Fresonik.mikroutik.com%2Fapi%2Fbadge%3Ftype%3Dweb)](https://resonik.mikroutik.com)
[![Total downloads](https://img.shields.io/endpoint?url=https%3A%2F%2Fresonik.mikroutik.com%2Fapi%2Fbadge%3Ftype%3Dtotal)](https://resonik.mikroutik.com)

An improved firmware for the **ATS Mini** pocket receiver (ESP32-S3 + SI4732): LW, MW, SW and FM, with AM, LSB, USB and CW.
It is built on the open-source [ats-mini](https://github.com/esp32-si4732/ats-mini) firmware and keeps everything it does,
adding features for shortwave listeners and radio amateurs.

> **Latest release: [v2.45-r4](https://github.com/ep3mhb/resonik-ats-mini/releases/latest)** (15 November 2026).

Website, browser installer and manual: **<https://resonik.mikroutik.com>**

What is coming in each release: [roadmap (PDF)](website/resonik-roadmap.pdf) · [نقشه‌ی راه فارسی (PDF)](website/resonik-roadmap-fa.pdf), source in `docs/roadmap/`.

## What v2.45-r4 adds

- **Web Control:** the whole radio in a browser on your phone or computer: frequency, S-meter, RDS, every setting,
  memories, bands and modes, a band scope (click a signal to tune), a live world map of the NCDXF/IARU beacons,
  and Listen (experimental, V4B)
- **Wi-Fi on the radio:** one Wi-Fi page with the IP address, Mode, Power, and Join a network with an on-screen keyboard
- **Seek in SSB and CW**, on the radio and in the browser
- The classic menu has the same six groups as the tile menu; faster band and mode changes; dark radio web pages
- Beacons: Off or Names only; betas update to the release over Wi-Fi

## Also included (from v2.45-r3)

- FT8 and FT4 decoder on a full-screen page with waterfall; it decodes even without a clock, finding the slot timing from the stations heard (needs the V4B board, speaker line on GPIO11)
- CW (Morse) decoder page, and a CW strip on the normal screen
- Band scanner: the knob sets the direction; holds on signals, 5 s per station on AM and FM broadcasts
- Automatic band plan: mode, filter and step follow the segment you tune into; Plan Area All regions, Region 1, 2, 3, Iran or UAE
- Frequency lock, Wi-Fi power (Normal, Low, Lowest) and processor speed (Battery, Balanced, Fast)
- Fixes: no freezes on slow networks after Wi-Fi connects, seek stays inside the band, smoother SSB tuning

## And from v2.45-r2

- A new default menu: six categories of large coloured tiles (Tuning, Audio, Memory, Ham & SWL, Display, System),
  settings shown full screen; the classic menu stays available
- NCDXF/IARU beacon tracker: names the beacon on the air, or follows one beacon across the five bands
- Memory scan, stopping on signals
- Signal logger to a CSV file, downloadable from the radio's web page
- Propagation data (solar flux, A and K index, band conditions) from hamqsl.com
- Kenwood TS-480 CAT control over USB (flrig, Hamlib, WSJT-X receive, logging programs)
- EiBi schedules with days of the week, language and target area
- System → Knob: 20 or 30 clicks per turn, for the two kinds of encoder

## And from v2.45-r1

- CW mode: true CW frequency shown, narrow filter, 10 Hz steps
- IARU Region 1 band plan on the tuning scale, with automatic LSB/USB
- Dual clock (UTC and local time) with named time zones and automatic daylight saving
- Battery time remaining
- Automatic attenuation when strong stations overload the receiver
- The clock survives switching off: automatic time sync at power-on, with a web-time fallback for networks that block NTP

Coming next: RTTY decoder, scan waterfall, status page, settings backup, setup wizard (see the roadmap).

## Which file do I need?

Each release has two variants. Check **Settings → About** on your radio (on RESONIK r2: **Menu → System → About**):

| PSRAM shown | Variant |
|---|---|
| 8192K (8 MB, octal) | `ospi` |
| 2048K (2 MB, quad) | `qspi` |

The `lilygo-t-embed` variant is only for the LILYGO T-Embed SI4732.

## Installing

- **In the browser:** <https://resonik.mikroutik.com>, Install button (Chrome or Edge on a computer).
- **From a RESONIK version:** Menu → System → Update FW (on r1: Settings → Update FW).
- **From the official ats-mini firmware:** by USB, once; after that, Settings → Update FW works.
- **By USB:** download `resonik-ats-mini-v2.45-r2-ospi.zip` (or `-qspi`) from the release, unzip, and flash with
  esptool-js / esptool as described in the upstream [flashing guide](https://esp32-si4732.github.io/ats-mini/flash.html):
  `ats-mini.ino.bootloader.bin` at 0x0, `ats-mini.ino.partitions.bin` at 0x8000, `boot_app0.bin` at 0xe000 and
  `ats-mini.ino.bin` at 0x10000. Your settings are kept.

Make a backup first (see the upstream [backup guide](https://esp32-si4732.github.io/ats-mini/recovery.html)).
You can always go back to the official firmware by USB.

## Versions

Release tags look like `v2.45-r1`: the ats-mini version the edition is based on, then the RESONIK release number.
A new upstream release (for example 2.46) is merged in and released as `v2.46-r1`.

## Building

Same as upstream: `make -C ats-mini` with arduino-cli, profile `esp32s3-ospi` or `esp32s3-qspi`
(see [development](https://esp32-si4732.github.io/ats-mini/development.html)). Unit tests: `make -C ats-mini test`.
GitHub Actions builds every push; running Build Firmware with a version such as `v2.45-r2` tags the branch and publishes the release.

## Credits and licence

This is a fork of [esp32-si4732/ats-mini](https://github.com/esp32-si4732/ats-mini) (MIT licence). The original
authors: PU2CLR (Ricardo Caratti), Volos Projects, Ralph Xavier, Sunnygold, Goshante, G8PTN (Dave), R9UCL (Max Arnold),
Marat Fayzullin, and all ats-mini contributors. The original README is in [README-upstream.md](README-upstream.md).

RESONIK edition by Mohammad Bahrololoum (EP2MHB). Released under the same MIT licence — see [LICENSE](LICENSE).

RESONIK and the RESONIK symbol are the project's name and mark; the licence covers the code, not the name or mark.
