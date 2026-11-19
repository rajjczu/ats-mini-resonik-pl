#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Storage.h"
#include "Scope.h"

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
    // Frequency moved UP -> spectrum shifts LEFT
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
    // Frequency moved DOWN -> spectrum shifts RIGHT
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
  // Completed a full pass
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

  if(!scopeTuning)
  {
    startBinTune(scopeScanIdx);
    return true;
  }

  if(millis() - scopeLastPoll < 6) return false;
  scopeLastPoll = millis();

  if(!rx.pollTuneStatus() && (millis() - scopeTuneStart < 60))
    return false;

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
    // Start scanning from the newly exposed edge
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
  if(norm > 75) return 0xF800; // Red
  if(norm > 50) return 0xFD20; // Orange
  if(norm > 25) return 0x07E0; // Green
  if(norm > 8)  return 0x07FF; // Cyan
  return 0x0210;               // Dark blue
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
  spr.drawString(scopeSweepActive ? "SCOPE [SWEEP]" : "SCOPE [LISTEN]", 8, 13, uiFont(true));

  char fbuf[24];
  formatScopeFreq(scopeCenterFreq, fbuf, sizeof(fbuf));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(yellow);
  spr.drawString(fbuf, 312, 13, uiFont(false));

  // Find dynamic min/max for scaling
  uint8_t lo = 255, hi = 0;
  for(int i = 0; i < SCOPE_BINS; i++)
  {
    if(scopeRssi[i] > 0 && scopeRssi[i] < lo) lo = scopeRssi[i];
    if(scopeRssi[i] > hi) hi = scopeRssi[i];
  }
  if(lo == 255) lo = 0;
  uint8_t spanR = (hi > lo + 6) ? (hi - lo) : 12;

  // Spectrum plot: x = 8..313 (61 bins * 5px = 305px), y = 30..102 (72px)
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
    uint16_t col = rssiColor(scopeRssi[i], lo, spanR);
    spr.fillRect(px + i * 5, py + ph - h, 4, h, col);
  }

  // Center frequency marker
  int cx = px + SCOPE_CENTER * 5 + 2;
  spr.drawFastVLine(cx, py, ph, yellow);

  // Sweep beam indicator
  if(scopeSweepActive)
  {
    int sx = px + scopeScanIdx * 5 + 2;
    spr.drawFastVLine(sx, py, ph, 0xFFFF);
  }

  // Waterfall: y = 105..145 (20 rows * 2px = 40px)
  const int wy = 105;
  for(int r = 0; r < WF_ROWS; r++)
  {
    for(int i = 0; i < SCOPE_BINS; i++)
    {
      uint8_t v = scopeWf[r][i];
      if(v == 0) continue;
      spr.fillRect(px + i * 5, wy + r * 2, 5, 2, rssiColor(v, lo, spanR));
    }
  }
  spr.drawFastVLine(cx, wy, WF_ROWS * 2, yellow);

  // Footer
  char info[56];
  snprintf(info, sizeof(info), "RSSI:%u SNR:%u  Step:%luk",
           scopeRssi[SCOPE_CENTER], scopeSnr[SCOPE_CENTER], (unsigned long)(scopeStep / 1000));
  spr.setTextDatum(BL_DATUM);
  spr.setTextColor(0xFFFF);
  spr.drawString(info, 6, 158, uiFont(true));
  spr.setTextDatum(BR_DATUM);
  spr.setTextColor(muted);
  spr.drawString("Click:Sweep/Listen Press:Exit", 314, 168, uiFont(true));
  spr.pushSprite(0, 0);
}
