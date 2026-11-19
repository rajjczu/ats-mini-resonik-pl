#ifndef SCOPE_H
#define SCOPE_H

#include <stdint.h>

// Full-screen live panning Scope + Waterfall
void scopeOpen();
bool scopeTick();
void scopeTurn(int16_t enc);
void scopeClick(bool shortPress);
void drawScopePage();

// Bottom bar on main screen: 0 = Skala, 1 = Historia RSSI, 2 = Mini-Widmo
extern uint8_t bottomBarIdx;
extern const char *const bottomBarDesc[3];
bool bottomBarShown();
bool bottomBarTick();
void drawBottomBar();

// Dual-Watch (VFO A/B): 0 = Off, 1 = VFO A/B
extern uint8_t dualWatchIdx;
extern const char *const dualWatchDesc[2];
bool dualWatchTick();

// Recently detected stations by scanner
void recordDetected(uint32_t freq, uint8_t mode, uint8_t rssi);
void detectedOpen();
void detectedTurn(int16_t enc);
void detectedClick(bool shortPress);
void drawDetectedPage();

// WSPR 2-minute slot monitor
void wsprOpen();
bool wsprTick();
void wsprTurn(int16_t enc);
void wsprClick(bool shortPress);
void drawWsprPage();

#endif // SCOPE_H
