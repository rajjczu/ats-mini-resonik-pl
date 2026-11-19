#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Storage.h"
#include "Resonik.h"
#include "CwAudio.h"
#include "Scope.h"
#include <time.h>

#define SCOPE_BINS      61
#define SCOPE_CENTER    (SCOPE_BINS / 2)
#define WF_ROWS         20

static uint8_t  scopeRssi[SCOPE_BINS];
static uint8_t  scopeSnr[SCOPE_BINS];
static uint8_t  scopeWf[WF_ROWS][SCOPE_BINS];
static uint32_t scopeCenterFreq = 0;
static uint32_t scopeStep = 5000;
static bool     scopeSweepActive = true;
static int8_t   scopeScanIdx = 0;
static bool     scopeTuning = false;
static uint32_t scopeTuneStart = 0;
static uint32_t scopeLastPoll = 0;

static uint32_t chooseScopeStep()
{
  if(currentMode == FM) return 100000;
  if(isSSB() || currentMode == CW) return 1000;
  const Band *b = getCurrentBand();
  if(b && strcmp(b->bandName, "CB-PL") == 0) return 10000;
  return 5000;
}

static uint32_t binFreq(int idx)
{
  int64_t f = (int64_t)scopeCenterFreq + (int64_t)(idx - SCOPE_CENTER) * (int64_t)scopeStep;
  if(f < 0) return 0;
  return (uint32_t)f;
}

static void shiftBuffers(int deltaBins)
{
  if(deltaBins == 0) return;
  if(abs(deltaBins) >= SCOPE_BINS)
  {
    memset(scopeRssi, 0, sizeof(scopeRssi));
    memset(scopeSnr, 0, sizeof(scopeSnr));
    memset(scopeWf, 0, sizeof(scopeWf));
    return;
  }
  if(deltaBins > 0)
  {
    int keep = SCOPE_BINS - deltaBins;
    memmove(&scopeRssi[0], &scopeRssi[deltaBins], keep);
    memmove(&scopeSnr[0], &scopeSnr[deltaBins], keep);
    memset(&scopeRssi[keep], 0, deltaBins);
    memset(&scopeSnr[keep], 0, deltaBins);
    for(int r = 0; r < WF_ROWS; r++)
    {
      memmove(&scopeWf[r][0], &scopeWf[r][deltaBins], keep);
      memset(&scopeWf[r][keep], 0, deltaBins);
    }
  }
  else
  {
    int sh = -deltaBins;
    int keep = SCOPE_BINS - sh;
    memmove(&scopeRssi[sh], &scopeRssi[0], keep);
    memmove(&scopeSnr[sh], &scopeSnr[0], keep);
    memset(&scopeRssi[0], 0, sh);
    memset(&scopeSnr[0], 0, sh);
    for(int r = 0; r < WF_ROWS; r++)
    {
      memmove(&scopeWf[r][sh], &scopeWf[r][0], keep);
      memset(&scopeWf[r][0], 0, sh);
    }
  }
}

static void pushWaterfallRow()
{
  memmove(&scopeWf[1][0], &scopeWf[0][0], (WF_ROWS - 1) * SCOPE_BINS);
  memcpy(&scopeWf[0][0], scopeRssi, SCOPE_BINS);
}

static void startBinTune(int idx)
{
  const Band *band = getCurrentBand();
  while(idx < SCOPE_BINS)
  {
    uint32_t f = binFreq(idx);
    if(f >= band->minimumFreq && f <= band->maximumFreq)
    {
      scopeScanIdx = (int8_t)idx;
      scopeTuning = true;
      scopeTuneStart = millis();
      scopeLastPoll = millis();
      rx.setFrequency(freqFromHz(f, currentMode));
      return;
    }
    idx++;
  }
  pushWaterfallRow();
  scopeScanIdx = 0;
  scopeTuning = false;
}

static void enterSweepMode()
{
  scopeSweepActive = true;
  muteOn(MUTE_TEMP, true);
  if(isSSB()) rx.setSSBBfo(0);
  scopeScanIdx = 0;
  scopeTuning = false;
}

static void enterListenMode()
{
  scopeSweepActive = false;
  scopeTuning = false;
  if(isSSB()) rx.setFrequencyConfirmed(getReceiverFrequency(scopeCenterFreq));
  updateFrequency(scopeCenterFreq, false);
  muteOn(MUTE_TEMP, false);
  prefsRequestSave(SAVE_CUR_BAND);
}

void scopeOpen()
{
  currentCmd = CMD_SCOPE;
  scopeStep = chooseScopeStep();
  scopeCenterFreq = currentFrequency;
  memset(scopeRssi, 0, sizeof(scopeRssi));
  memset(scopeSnr, 0, sizeof(scopeSnr));
  memset(scopeWf, 0, sizeof(scopeWf));
  enterSweepMode();
}

bool scopeTick()
{
  if(currentCmd != CMD_SCOPE || !scopeSweepActive) return false;
  if(!scopeTuning) { startBinTune(scopeScanIdx); return true; }
  if(millis() - scopeLastPoll < 6) return false;
  scopeLastPoll = millis();
  if(!rx.pollTuneStatus() && (millis() - scopeTuneStart < 60)) return false;

  rx.getCurrentReceivedSignalQuality();
  scopeRssi[scopeScanIdx] = rx.getCurrentRSSI();
  scopeSnr[scopeScanIdx]  = rx.getCurrentSNR();
  scopeTuning = false;
  scopeScanIdx++;
  if(scopeScanIdx >= SCOPE_BINS)
  {
    pushWaterfallRow();
    scopeScanIdx = 0;
  }
  return true;
}

void scopeTurn(int16_t enc)
{
  if(enc == 0) return;
  const Band *band = getCurrentBand();
  int64_t next = (int64_t)scopeCenterFreq + (int64_t)enc * (int64_t)scopeStep;
  if(next < (int64_t)band->minimumFreq) next = band->minimumFreq;
  if(next > (int64_t)band->maximumFreq) next = band->maximumFreq;
  int actualDelta = (int)((next - (int64_t)scopeCenterFreq) / (int64_t)scopeStep);
  scopeCenterFreq = (uint32_t)next;
  shiftBuffers(actualDelta);

  if(scopeSweepActive)
  {
    scopeScanIdx = (actualDelta > 0) ? (SCOPE_BINS - min(actualDelta, SCOPE_BINS)) : 0;
    scopeTuning = false;
  }
  else
  {
    updateFrequency(scopeCenterFreq, false);
    rx.getCurrentReceivedSignalQuality();
    scopeRssi[SCOPE_CENTER] = rx.getCurrentRSSI();
    scopeSnr[SCOPE_CENTER]  = rx.getCurrentSNR();
    prefsRequestSave(SAVE_CUR_BAND);
  }
}

void scopeClick(bool shortPress)
{
  if(shortPress)
  {
    if(scopeSweepActive) enterListenMode();
    currentCmd = CMD_NONE;
    return;
  }
  if(scopeSweepActive) enterListenMode();
  else enterSweepMode();
}

static void formatScopeFreq(uint32_t hz, char *buf, size_t len)
{
  if(currentMode == FM)
    snprintf(buf, len, "%lu.%02lu MHz", (unsigned long)(hz / 1000000), (unsigned long)((hz % 1000000) / 10000));
  else
    snprintf(buf, len, "%lu.%03lu kHz", (unsigned long)(hz / 1000), (unsigned long)(hz % 1000));
}

static uint16_t rssiColor(uint8_t r, uint8_t lo, uint8_t span)
{
  int norm = ((int)r - (int)lo) * 100 / (span > 0 ? span : 1);
  if(norm > 75) return 0xF800;
  if(norm > 50) return 0xFD20;
  if(norm > 25) return 0x07E0;
  if(norm > 8)  return 0x07FF;
  return 0x0210;
}

void drawScopePage()
{
  const uint16_t bg = TFT_BLACK;
  const uint16_t hdr = 0x09C9;
  const uint16_t grid = 0x2124;
  const uint16_t muted = 0x9CD3;
  const uint16_t yellow = 0xFFE0;
  const uint16_t green = 0x07E0;
  const uint16_t cyan = 0x07FF;

  spr.fillSprite(bg);
  spr.fillRect(0, 0, 320, 26, hdr);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(scopeSweepActive ? cyan : green);
  spr.drawString(scopeSweepActive ? "WIDMO [SKAN]" : "WIDMO [ODSLUCH]", 8, 13, uiFont(true));

  char fbuf[24];
  formatScopeFreq(scopeCenterFreq, fbuf, sizeof(fbuf));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(yellow);
  spr.drawString(fbuf, 312, 13, uiFont(false));

  uint8_t lo = 255, hi = 0;
  for(int i = 0; i < SCOPE_BINS; i++)
  {
    if(scopeRssi[i] > 0 && scopeRssi[i] < lo) lo = scopeRssi[i];
    if(scopeRssi[i] > hi) hi = scopeRssi[i];
  }
  if(lo == 255) lo = 0;
  uint8_t spanR = (hi > lo + 6) ? (hi - lo) : 12;

  const int px = 8, py = 30, pw = 305, ph = 72;
  for(int g = 0; g <= 3; g++) spr.drawFastHLine(px, py + (ph * g) / 3, pw, grid);
  for(int g = 0; g <= 4; g++) spr.drawFastVLine(px + (pw * g) / 4, py, ph, grid);

  for(int i = 0; i < SCOPE_BINS; i++)
  {
    if(scopeRssi[i] == 0) continue;
    int val = (int)scopeRssi[i] - (int)lo;
    if(val < 0) val = 0;
    int h = (val * (ph - 4)) / spanR;
    if(h > ph - 2) h = ph - 2;
    if(h < 2) h = 2;
    spr.fillRect(px + i * 5, py + ph - h, 4, h, rssiColor(scopeRssi[i], lo, spanR));
  }

  int cx = px + SCOPE_CENTER * 5 + 2;
  spr.drawFastVLine(cx, py, ph, yellow);
  if(scopeSweepActive) spr.drawFastVLine(px + scopeScanIdx * 5 + 2, py, ph, 0xFFFF);

  const int wy = 105;
  for(int r = 0; r < WF_ROWS; r++)
    for(int i = 0; i < SCOPE_BINS; i++)
      if(scopeWf[r][i] > 0)
        spr.fillRect(px + i * 5, wy + r * 2, 5, 2, rssiColor(scopeWf[r][i], lo, spanR));
  spr.drawFastVLine(cx, wy, WF_ROWS * 2, yellow);

  char info[56];
  snprintf(info, sizeof(info), "RSSI:%u SNR:%u  Krok:%luk",
           scopeRssi[SCOPE_CENTER], scopeSnr[SCOPE_CENTER], (unsigned long)(scopeStep / 1000));
  spr.setTextDatum(BL_DATUM);
  spr.setTextColor(0xFFFF);
  spr.drawString(info, 6, 158, uiFont(true));
  spr.setTextDatum(BR_DATUM);
  spr.setTextColor(muted);
  spr.drawString("Klik:Skan/Odsluch Press:Wyjscie", 314, 168, uiFont(true));
  spr.pushSprite(0, 0);
}

//
// Bottom bar on main screen: Skala / Historia RSSI / Mini-Widmo
//
uint8_t bottomBarIdx = 1; // Historia RSSI active by default!
const char *const bottomBarDesc[3] = { "Skala", "Historia", "MiniWidmo" };

#define HIST_POINTS 72
static uint8_t histRssi[HIST_POINTS] = {};
static uint32_t histLastTick = 0;

static uint8_t miniSpec[SCOPE_BINS] = {};
static uint32_t miniLastFreq = 0;
static uint32_t miniStep = 5000;

bool bottomBarShown()
{
  return bottomBarIdx != 0 && !sleepOn();
}

bool bottomBarTick()
{
  if(bottomBarIdx == 0 || currentCmd != CMD_NONE || sleepOn()) return false;
  uint32_t now = millis();
  if(bottomBarIdx == 1)
  {
    if(now - histLastTick < 500) return false;
    histLastTick = now;
    memmove(&histRssi[0], &histRssi[1], HIST_POINTS - 1);
    histRssi[HIST_POINTS - 1] = rssi;
    return true;
  }
  else
  {
    uint32_t step = chooseScopeStep();
    if(step != miniStep || abs((int64_t)currentFrequency - (int64_t)miniLastFreq) > (int64_t)step * 30)
    {
      memset(miniSpec, 0, sizeof(miniSpec));
      miniStep = step;
      miniLastFreq = currentFrequency;
    }
    int delta = (int)(((int64_t)currentFrequency - (int64_t)miniLastFreq) / (int64_t)miniStep);
    if(delta != 0)
    {
      if(abs(delta) >= SCOPE_BINS) memset(miniSpec, 0, sizeof(miniSpec));
      else if(delta > 0)
      {
        int k = SCOPE_BINS - delta;
        memmove(&miniSpec[0], &miniSpec[delta], k);
        memset(&miniSpec[k], 0, delta);
      }
      else
      {
        int sh = -delta, k = SCOPE_BINS - sh;
        memmove(&miniSpec[sh], &miniSpec[0], k);
        memset(&miniSpec[0], 0, sh);
      }
      miniLastFreq = currentFrequency;
    }
    if(now - histLastTick < 250 && miniSpec[SCOPE_CENTER] == rssi) return false;
    histLastTick = now;
    miniSpec[SCOPE_CENTER] = rssi;
    return true;
  }
}

void drawBottomBar()
{
  const uint16_t box = 0x09C9;
  const uint16_t yellow = 0xFFE0;
  const uint16_t cyan = 0x07FF;
  const uint16_t muted = 0x9CD3;

  spr.fillRoundRect(0, 131, 320, 39, 8, box);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(cyan);
  spr.drawString(bottomBarIdx == 1 ? "RSSI" : "WIDMO", 8, 136, uiFont(true));

  char rbuf[16];
  snprintf(rbuf, sizeof(rbuf), "%udB", rssi);
  spr.setTextColor(yellow);
  spr.drawString(rbuf, 8, 152, uiFont(true));

  const int gx = 68, gy = 135, gw = 244, gh = 31;
  if(bottomBarIdx == 1)
  {
    uint8_t lo = 255, hi = 0;
    for(int i = 0; i < HIST_POINTS; i++)
    {
      if(histRssi[i] > 0 && histRssi[i] < lo) lo = histRssi[i];
      if(histRssi[i] > hi) hi = histRssi[i];
    }
    if(lo == 255) lo = 0;
    uint8_t span = (hi > lo + 6) ? (hi - lo) : 12;
    for(int i = 0; i < HIST_POINTS; i++)
    {
      if(histRssi[i] == 0) continue;
      int h = ((int)histRssi[i] - (int)lo) * (gh - 2) / span;
      if(h < 2) h = 2;
      if(h > gh) h = gh;
      spr.fillRect(gx + i * 3 + (i / 3), gy + gh - h, 2, h, rssiColor(histRssi[i], lo, span));
    }
  }
  else
  {
    uint8_t lo = 255, hi = 0;
    for(int i = 0; i < SCOPE_BINS; i++)
    {
      if(miniSpec[i] > 0 && miniSpec[i] < lo) lo = miniSpec[i];
      if(miniSpec[i] > hi) hi = miniSpec[i];
    }
    if(lo == 255) lo = 0;
    uint8_t span = (hi > lo + 6) ? (hi - lo) : 12;
    for(int i = 0; i < SCOPE_BINS; i++)
    {
      if(miniSpec[i] == 0) continue;
      int h = ((int)miniSpec[i] - (int)lo) * (gh - 2) / span;
      if(h < 2) h = 2;
      if(h > gh) h = gh;
      spr.fillRect(gx + i * 4, gy + gh - h, 3, h, rssiColor(miniSpec[i], lo, span));
    }
    spr.drawFastVLine(gx + SCOPE_CENTER * 4 + 1, gy, gh, yellow);
  }
  (void)muted;
}

//
// Dual-Watch (VFO A/B)
//
uint8_t dualWatchIdx = 0;
const char *const dualWatchDesc[2] = { "Off", "VFO A/B" };

extern VfoState vfos[2];
extern void switchVfo();

bool dualWatchTick()
{
  static uint32_t lastCheck = 0;
  if(!dualWatchIdx || currentCmd != CMD_NONE || bandScanActive() || memScanActive() || sleepOn())
    return false;
  uint32_t now = millis();
  if(now - lastCheck < 5000) return false;
  lastCheck = now;

  int other = 1 - vfoIdx;
  if(vfos[other].freq == 0) return false;
  uint8_t thr = currentSquelch ? currentSquelch : 18;
  if(rssi >= thr) return false; // Stay if current VFO already has signal

  // Brief peek at the other VFO
  switchVfo();
  delay(20);
  rx.getCurrentReceivedSignalQuality();
  uint8_t r = rx.getCurrentRSSI();
  if(r >= thr)
  {
    statusShow("DualWatch", "Aktywne VFO!", 1500);
    return true;
  }
  switchVfo();
  return false;
}

//
// Recently detected stations by scanner
//
#define DET_MAX 16
struct DetectedEntry
{
  uint32_t freq;
  uint8_t mode;
  uint8_t rssi;
};
static DetectedEntry detList[DET_MAX] = {};
static uint8_t detCount = 0;
static int8_t detCursor = 0;

void recordDetected(uint32_t freq, uint8_t mode, uint8_t r)
{
  if(!freq) return;
  if(detCount > 0 && detList[0].freq == freq && detList[0].mode == mode)
  {
    detList[0].rssi = r;
    return;
  }
  int move = (detCount < DET_MAX) ? detCount : (DET_MAX - 1);
  if(move > 0) memmove(&detList[1], &detList[0], move * sizeof(DetectedEntry));
  detList[0] = { freq, mode, r };
  if(detCount < DET_MAX) detCount++;
}

void detectedOpen()
{
  detCursor = 0;
  currentCmd = CMD_DETECTED;
}

void detectedTurn(int16_t enc)
{
  if(detCount == 0) return;
  int n = detCursor + (enc > 0 ? 1 : -1);
  if(n < 0) n = detCount - 1;
  if(n >= detCount) n = 0;
  detCursor = (int8_t)n;
}

void detectedClick(bool shortPress)
{
  if(!shortPress && detCount > 0 && detCursor < detCount)
    tuneToFrequency(detList[detCursor].freq, detList[detCursor].mode);
  currentCmd = CMD_NONE;
}

void drawDetectedPage()
{
  const uint16_t deep = 0x0108, hdr = 0x09C9, yellow = 0xFFE0, cyan = 0x07FF, muted = 0x9CD3;
  spr.fillSprite(deep);
  spr.fillRect(0, 0, 320, 28, hdr);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(0xFFFF);
  spr.drawString("Ostatnio wykryte", 10, 14, uiFont(false));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(cyan);
  char cbuf[16];
  snprintf(cbuf, sizeof(cbuf), "%u/%u", detCount ? detCursor + 1 : 0, detCount);
  spr.drawString(cbuf, 310, 14, uiFont(true));

  if(detCount == 0)
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(yellow);
    spr.drawString("Brak wykrytych stacji", 160, 84, uiFont(false));
    spr.setTextColor(muted);
    spr.drawString("Uruchom Skaner lub Skan Pam.", 160, 110, uiFont(true));
  }
  else
  {
    int start = (detCursor >= 4) ? (detCursor - 4) : 0;
    for(int row = 0; row < 5 && (start + row) < detCount; row++)
    {
      int idx = start + row;
      int y = 34 + row * 23;
      bool sel = (idx == detCursor);
      if(sel) spr.fillRoundRect(6, y, 308, 21, 5, hdr);
      char fbuf[40];
      if(detList[idx].mode == FM)
        snprintf(fbuf, sizeof(fbuf), "%lu.%02lu MHz  %s  RSSI:%u",
                 (unsigned long)(detList[idx].freq / 1000000),
                 (unsigned long)((detList[idx].freq % 1000000) / 10000),
                 bandModeDesc[detList[idx].mode], detList[idx].rssi);
      else
        snprintf(fbuf, sizeof(fbuf), "%lu.%03lu kHz  %s  RSSI:%u",
                 (unsigned long)(detList[idx].freq / 1000),
                 (unsigned long)(detList[idx].freq % 1000),
                 bandModeDesc[detList[idx].mode], detList[idx].rssi);
      spr.setTextDatum(ML_DATUM);
      spr.setTextColor(sel ? yellow : 0xFFFF);
      spr.drawString(fbuf, 14, y + 10, uiFont(true));
    }
  }
  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(muted);
  spr.drawString("Obrot: wybor  Klik: dostroj  Press: wyjscie", 160, 168, uiFont(true));
  spr.pushSprite(0, 0);
}

//
// WSPR 2-minute slot monitor
//
static const uint32_t wsprFreqs[] = {
  1836600, 3568600, 5287200, 7038600, 10138700, 14095600, 18104600, 21094600, 24924600, 28124600
};
static const char *const wsprNames[] = {
  "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"
};
static int8_t wsprIdx = 3; // 40m default
static uint32_t wsprLastDraw = 0;

void wsprOpen()
{
  tuneToFrequency(wsprFreqs[wsprIdx], USB);
  currentCmd = CMD_WSPR;
}

void wsprTurn(int16_t enc)
{
  const int n = (int)(sizeof(wsprFreqs) / sizeof(wsprFreqs[0]));
  int next = (int)wsprIdx + (enc > 0 ? 1 : -1);
  if(next < 0) next = n - 1;
  if(next >= n) next = 0;
  wsprIdx = (int8_t)next;
  tuneToFrequency(wsprFreqs[wsprIdx], USB);
}

void wsprClick(bool shortPress)
{
  if(shortPress) currentCmd = CMD_NONE;
  else wsprTurn(1);
}

bool wsprTick()
{
  if(currentCmd != CMD_WSPR) return false;
  if(millis() - wsprLastDraw < 500) return false;
  wsprLastDraw = millis();
  return true;
}

void drawWsprPage()
{
  const uint16_t deep = 0x0108, hdr = 0x09C9, yellow = 0xFFE0, cyan = 0x07FF, green = 0x07E0, muted = 0x9CD3;
  spr.fillSprite(deep);
  spr.fillRect(0, 0, 320, 28, hdr);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(0xFFFF);
  char hbuf[32];
  snprintf(hbuf, sizeof(hbuf), "WSPR %s USB", wsprNames[wsprIdx]);
  spr.drawString(hbuf, 10, 14, uiFont(false));

  snprintf(hbuf, sizeof(hbuf), "%lu.%03lu kHz",
           (unsigned long)(wsprFreqs[wsprIdx] / 1000), (unsigned long)(wsprFreqs[wsprIdx] % 1000));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(yellow);
  spr.drawString(hbuf, 310, 14, uiFont(false));

  time_t now = time(nullptr);
  int slotSec = (now > 1700000000) ? (int)(now % 120) : (int)((millis() / 1000) % 120);
  int rem = 120 - slotSec;

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(cyan);
  snprintf(hbuf, sizeof(hbuf), "Okno 2-min: %d / 120 s (pozostalo %d s)", slotSec, rem);
  spr.drawString(hbuf, 12, 40, uiFont(true));

  spr.fillRoundRect(12, 60, 296, 12, 6, hdr);
  spr.fillRoundRect(12, 60, (296 * slotSec) / 120, 12, 6, (slotSec < 111) ? green : yellow);

  snprintf(hbuf, sizeof(hbuf), "Sygnal RF: RSSI %u dBuV   SNR %u dB", rssi, snr);
  spr.setTextColor(0xFFFF);
  spr.drawString(hbuf, 12, 86, uiFont(false));

  uint16_t aud = audioInputLevel();
  snprintf(hbuf, sizeof(hbuf), "Poziom audio (GPIO11): %u", (unsigned)aud);
  spr.setTextColor(muted);
  spr.drawString(hbuf, 12, 110, uiFont(true));

  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(muted);
  spr.drawString("Obrot/Klik: pasmo WSPR   Press: wyjscie", 160, 168, uiFont(true));
  spr.pushSprite(0, 0);
}
