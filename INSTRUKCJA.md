# Instrukcja obsługi — ATS Mini (RESONIK PL+ Edition)

## 1. Podstawowa obsługa gałki (enkodera)
* **Obrót:** zmiana częstotliwości (lub przewijanie pozycji w menu / przesuwanie widma w trybie **Widmo**). Szybszy obrót automatycznie zwiększa krok strojenia.
* **Krótkie kliknięcie:** otwarcie menu / wybór opcji / zatwierdzenie.
* **Przytrzymanie (ok. 0,5 s):** wyjście z ekranu/menu bez zapisywania lub powrót do głównego ekranu radia.
* **Wciśnięcie i obrót na ekranie głównym:** szybkie strojenie wybraną cyfrą częstotliwości.

---

## 2. Polskie menu kafelkowe (6 kategorii)
Po kliknięciu na ekranie głównym otwiera się 6 kolorowych kategorii:

1. **Strojenie:**
   * `Pasmo` — wybór pasma (m.in. `VHF` FM, `ALL`, `CB-PL`, `CB-DX`, `60MH`, `LW-PL`, `NVTX`, `VOLM`, `TIME` oraz pasma krótkofalarskie i radiofoniczne).
   * `Tryb` — `FM`, `AM`, `LSB`, `USB`, `CW`.
   * `Widmo` — **przesuwny analizator widma + wodospad na żywo** (opis poniżej).
   * `Skaner` / `Skanuj` / `Szukaj` — automatyczne skanowanie pasma i zatrzymywanie na sygnałach.
   * `Krok`, `Szerokosc` (filtr audio), `VFO A/B`, `Blokada` (blokada częstotliwości — gałka zmienia wtedy głośność), `RDS`, `Region FM`.
2. **Audio:**
   * `Glosnosc`, `AGC/ATTN`, `Auto Att.` (automatyczne tłumienie przesterowań), `AVC`, `FM Stereo`, `SoftMute`, `Squelch` (bramka szumów).
3. **Pamiec:**
   * `Pamiec` — 99 slotów pamięci (sloty 01–82 są fabrycznie wypełnione stacjami dla Krakowa/Małopolski, CB, lotnictwa HF, służb meteo, fal długich/średnich, krótkofalowców i KF).
   * `Skan Pam.` — automatyczne skanowanie zapisanych w pamięci stacji (zatrzymuje się na aktywnych kanałach).
   * `Logger` — zapisywanie logów sygnałów do pliku CSV (do pobrania przez WWW).
4. **Ham i SWL:**
   * `Pogoda` — **aktualna pogoda dla Krakowa z internetu** (temperatura, wilgotność, ciśnienie hPa, wiatr km/h z Open-Meteo + skrót propagacji KF).
   * `Propagacja` — aktualne warunki propagacyjne KF (SFI, plamy słoneczne, indeksy A/K z hamqsl.com).
   * `FT8/FT4` — wbudowany dekoder FT8 i FT4 z wodospadem *(wymaga płytki V4B lub mostka audio na GPIO11)*.
   * `Dekoder CW` / `Pasek CW` — dekoder telegrafii Morse'a.
   * `RTTY/NVTX` — dekoder RTTY (45.45 Bd) oraz morskiego NAVTEX (100 Bd SITOR-B).
   * `Beacony` — śledzenie 18 radiolatarni propagacyjnych NCDXF/IARU.
   * `Plan Pasm` / `Obszar` / `Pob. EiBi` — pasma IARU na skali oraz harmonogramy stacji krótkofalowych.
5. **Ekran:**
   * `Jasnosc`, `Motyw`, `Uklad UI` (klasyczny / duży S-metr), `Styl Menu` (kafelki / klasyczna lista), `Usypianie`, `Tryb Snu`, `Powieksz`, `Kierunek`.
6. **System:**
   * `Wi-Fi` — łączenie z domową siecią 2.4 GHz (wpisywanie hasła gałką) lub uruchomienie własnego AP (`10.1.1.1`).
   * `Port USB` — `Off`, `Ad hoc` (sterowanie komendami po `COM`), `CAT` (emulacja Kenwood TS-480 dla WSJT-X / flrig / Hamlib).
   * `Port TCP`, `Bluetooth`, `CPU` (80 / 160 / 240 MHz), `Data/Czas`, `Strefa`, `Enkoder`, `Aktualiz.`, `O Radiu`.

---

## 3. Przesuwny analizator widma + wodospad (`Strojenie → Widmo`)
* **Środek ekranu (żółta linia):** Twoja aktualna częstotliwość. Wokół niej widzisz 61 kanałów widma oraz 20-liniowy wodospad.
* **Obrót gałką:** przelatujesz płynnie po częstotliwościach — już zmierzone piki i wodospad przesuwają się razem z Tobą, a skaner na bieżąco doskanowuje nowy obszar.
* **Kliknięcie:** przełącza między trybem **`SCOPE [SWEEP]`** (ciągłe skanowanie widma) a **`SCOPE [LISTEN]`** (natychmiastowy odsłuch częstotliwości na żółtej linii).
* **Przytrzymanie:** wychodzi z widma, zostawiając radio dostrojone na wybranej częstotliwości.

---

## 4. Dekoder RTTY / NAVTEX (`Ham i SWL → RTTY/NVTX`)
* Dwa paski **M (1275 Hz)** i **S (1445 Hz)** ułatwiają dokładne dostrojenie stacji — kręć gałką, aż oba paski wyraźnie pracują na zmianę.
* Na częstotliwościach 485–525 kHz (np. `518 kHz`, `490 kHz`) automatycznie startuje tryb **NAVTEX 100 Bd**.
* **Kliknięcie:** przełącza ręcznie między **RTTY 45.45** (np. stacje meteo DWD `4583` / `7646` kHz USB) a **NAVTEX 100**.

---

## 5. Funkcje internetowe (po włączeniu Wi-Fi)
Po połączeniu radia z siecią Wi-Fi (`System → Wi-Fi`):
* **Pogoda dla Krakowa (`Ham i SWL → Pogoda`):** pobiera na żywo temperaturę, wilgotność, ciśnienie i wiatr z `api.open-meteo.com`. Kliknięcie odświeża dane.
* **Propagacja (`Ham i SWL → Propagacja`):** pobiera SFI, plamy słoneczne i indeksy A/K z `hamqsl.com`.
* **Synchronizacja czasu (NTP)** oraz **harmonogramy stacji KF (`Pob. EiBi`)**.
* **Sterowanie z przeglądarki (`http://<IP-radia>/control`):** pełny pulpit odbiornika, Band Scope, pamięci i podgląd ekranu radia.
  * *Uwaga o audio przez PC:* funkcja `Listen` w przeglądarce ma charakter eksperymentalny (ESP32-S3 współdzieli ADC2 z Wi-Fi, przez co dźwięk może bulgotać). Do czystego odsłuchu na komputerze użyj kabelka **jack 3.5 mm → Line-In / Mic w PC**.

---

## 6. Ograniczenia sprzętowe (warto wiedzieć)
* Układ **SI4732** odbiera wyłącznie **150 kHz – 30 MHz (AM/LSB/USB/CW)** oraz **64 – 108 MHz (FM)**.
* Pasmo lotnicze VHF (**118–137 MHz**) oraz służby VHF/UHF (**148–174 / 446 MHz**) są poza zakresem tego układu — z lotnictwa na tym radiu odbierasz dalekosiężne komunikaty **HF VOLMET** i **MWARA** (zapisane w pamięciach 26–40).
