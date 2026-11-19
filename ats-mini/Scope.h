#ifndef SCOPE_H
#define SCOPE_H

#include <stdint.h>

void scopeOpen();
bool scopeTick();
void scopeTurn(int16_t enc);
void scopeClick(bool shortPress);
void drawScopePage();

#endif // SCOPE_H
