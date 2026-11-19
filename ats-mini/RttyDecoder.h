#ifndef RTTY_DECODER_H
#define RTTY_DECODER_H

// RTTY (45.45 Bd, 5N1.5 ITA2) & NAVTEX (100 Bd, 7-bit SITOR-B) decoder
// Driven at 4000 Hz sample rate from CwAudio.cpp (GPIO11 ADC2).

#include <stdint.h>
#include <string.h>
#include <math.h>

class RttyDecoder
{
public:
  static constexpr int TEXT_MAX = 160;
  static constexpr int FS = 4000;
  static constexpr float MARK_HZ = 1275.0f;
  static constexpr float SPACE_HZ = 1445.0f;

  enum Mode : uint8_t { MODE_RTTY45 = 0, MODE_NAVTEX100 = 1 };

  void init()
  {
    reset();
    coeffMark  = 2.0f * cosf(2.0f * 3.14159265f * MARK_HZ / FS);
    coeffSpace = 2.0f * cosf(2.0f * 3.14159265f * SPACE_HZ / FS);
  }

  void setMode(Mode m)
  {
    if(mode == m) return;
    mode = m;
    resetStream();
  }

  Mode getMode() const { return mode; }

  void reset()
  {
    clearText();
    resetStream();
  }

  void resetStream()
  {
    figs = false;
    state = IDLE;
    sampleInBit = 0;
    bitIdx = 0;
    rxByte = 0;
    sitorSync = false;
    sitorReg = 0;
    sitorBitPos = 0;
    goertzelN = 0;
    q1M = q2M = q1S = q2S = 0.0f;
    markLevel = spaceLevel = 0;
  }

  void clearText()
  {
    textLen = 0;
    text[0] = '\0';
  }

  const char *getText() const { return text; }
  uint8_t getMarkLevel() const { return markLevel; }
  uint8_t getSpaceLevel() const { return spaceLevel; }

  void processSamples(const int16_t *samples, int count)
  {
    const int win = (mode == MODE_RTTY45) ? 22 : 20; // ~5ms Goertzel window
    const int bitLen = (mode == MODE_RTTY45) ? 88 : 40;

    for(int i = 0; i < count; i++)
    {
      float x = (float)samples[i];
      float q0M = coeffMark * q1M - q2M + x;
      q2M = q1M; q1M = q0M;
      float q0S = coeffSpace * q1S - q2S + x;
      q2S = q1S; q1S = q0S;

      if(++goertzelN >= win)
      {
        float pM = q1M * q1M + q2M * q2M - coeffMark * q1M * q2M;
        float pS = q1S * q1S + q2S * q2S - coeffSpace * q1S * q2S;
        q1M = q2M = q1S = q2S = 0.0f;
        goertzelN = 0;

        float mMag = sqrtf(pM > 0.0f ? pM : 0.0f) / win;
        float sMag = sqrtf(pS > 0.0f ? pS : 0.0f) / win;

        // Smooth levels (0..100) for UI tuning bars
        int mScaled = (int)(mMag * 0.4f);
        int sScaled = (int)(sMag * 0.4f);
        if(mScaled > 100) mScaled = 100;
        if(sScaled > 100) sScaled = 100;
        markLevel  = (uint8_t)((markLevel * 3 + mScaled) / 4);
        spaceLevel = (uint8_t)((spaceLevel * 3 + sScaled) / 4);

        currentBit = (mMag >= sMag) ? 1 : 0; // 1 = Mark, 0 = Space
      }

      if(mode == MODE_RTTY45)
        stepRtty(currentBit, bitLen);
      else
        stepNavtex(currentBit, bitLen);
    }
  }

private:
  enum RxState : uint8_t { IDLE, START_BIT, DATA_BITS, STOP_BIT };

  Mode mode = MODE_RTTY45;
  float coeffMark = 0.0f;
  float coeffSpace = 0.0f;
  float q1M = 0.0f, q2M = 0.0f;
  float q1S = 0.0f, q2S = 0.0f;
  int goertzelN = 0;
  uint8_t currentBit = 1;
  uint8_t markLevel = 0;
  uint8_t spaceLevel = 0;

  RxState state = IDLE;
  int sampleInBit = 0;
  int bitIdx = 0;
  uint8_t rxByte = 0;
  bool figs = false;

  // SITOR-B / NAVTEX state
  bool sitorSync = false;
  uint8_t sitorReg = 0;
  uint8_t sitorBitPos = 0;

  char text[TEXT_MAX + 1] = {};
  int textLen = 0;

  void appendChar(char c)
  {
    if(c == '\r' || c == '\n') c = ' ';
    if(c < 32 || c > 126) return;
    if(c == ' ' && (textLen == 0 || text[textLen - 1] == ' ')) return;
    if(textLen >= TEXT_MAX)
    {
      memmove(text, text + 1, TEXT_MAX - 1);
      textLen = TEXT_MAX - 1;
    }
    text[textLen++] = c;
    text[textLen] = '\0';
  }

  void decodeIta2(uint8_t code)
  {
    static const char ltrs[32] = {
      '\0', 'E', '\n', 'A', ' ', 'S', 'I', 'U',
      '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
      'T',  'Z', 'L', 'W', 'H', 'Y', 'P', 'Q',
      'O',  'B', 'G', '\0','M', 'X', 'V', '\0'
    };
    static const char figsTbl[32] = {
      '\0', '3', '\n', '-', ' ', '\'', '8', '7',
      '\r', '$', '4', '!', ',', '!', ':', '(',
      '5',  '+', ')', '2', '#', '6', '0', '1',
      '9',  '?', '&', '\0','.', '/', '=', '\0'
    };
    code &= 0x1F;
    if(code == 0x1F) { figs = false; return; } // LTRS
    if(code == 0x1B) { figs = true;  return; } // FIGS
    char c = figs ? figsTbl[code] : ltrs[code];
    if(c) appendChar(c);
  }

  void stepRtty(uint8_t bit, int bitLen)
  {
    switch(state)
    {
      case IDLE:
        if(bit == 0) // Start bit (Space)
        {
          state = START_BIT;
          sampleInBit = 1;
        }
        break;
      case START_BIT:
        if(++sampleInBit >= bitLen / 2)
        {
          if(bit == 0)
          {
            state = DATA_BITS;
            sampleInBit = 0;
            bitIdx = 0;
            rxByte = 0;
          }
          else
            state = IDLE;
        }
        break;
      case DATA_BITS:
        if(++sampleInBit >= bitLen)
        {
          sampleInBit = 0;
          if(bit) rxByte |= (1u << bitIdx);
          if(++bitIdx >= 5) state = STOP_BIT;
        }
        break;
      case STOP_BIT:
        if(++sampleInBit >= bitLen / 2)
        {
          if(bit == 1) decodeIta2(rxByte);
          state = IDLE;
        }
        break;
    }
  }

  // CCIR 476 / SITOR-B 7-bit (LSB-first) to 5-bit ITA2 mapping
  static int8_t sitorToIta2(uint8_t code7)
  {
    struct Map { uint8_t sitor; uint8_t ita2; };
    static const Map tbl[] = {
      {0x6A, 0x03}, // A
      {0x72, 0x19}, // B
      {0x2E, 0x0E}, // C
      {0x36, 0x09}, // D
      {0x5C, 0x01}, // E
      {0x35, 0x0D}, // F
      {0x4E, 0x1A}, // G
      {0x47, 0x14}, // H
      {0x5A, 0x06}, // I
      {0x6C, 0x0B}, // J
      {0x3C, 0x0F}, // K
      {0x4B, 0x12}, // L
      {0x4D, 0x1C}, // M
      {0x4E ^ 0x00, 0x0C}, // placeholder avoided below
    };
    (void)tbl;
    // Direct CCIR-476 lookup (7-bit codes with 4 ones / 3 zeros)
    switch(code7 & 0x7F)
    {
      case 0x5C: return 0x01; // E
      case 0x6A: return 0x03; // A
      case 0x59: return 0x05; // S
      case 0x5A: return 0x06; // I
      case 0x66: return 0x07; // U
      case 0x36: return 0x09; // D
      case 0x2D: return 0x0A; // R
      case 0x6C: return 0x0B; // J
      case 0x2B: return 0x0C; // N
      case 0x35: return 0x0D; // F
      case 0x2E: return 0x0E; // C
      case 0x3C: return 0x0F; // K
      case 0x17: return 0x10; // T
      case 0x55: return 0x11; // Z
      case 0x4B: return 0x12; // L
      case 0x65: return 0x13; // W
      case 0x47: return 0x14; // H
      case 0x69: return 0x15; // Y
      case 0x27: return 0x16; // P
      case 0x39: return 0x17; // Q
      case 0x4D: return 0x18; // O
      case 0x72: return 0x19; // B
      case 0x4E: return 0x1A; // G
      case 0x1D: return 0x1C; // M
      case 0x53: return 0x1D; // X
      case 0x1E: return 0x1E; // V
      case 0x71: return 0x04; // Space
      case 0x33: return 0x02; // LF
      case 0x1B: return 0x08; // CR
      case 0x56: return 0x1F; // LTRS
      case 0x3A: return 0x1B; // FIGS
      default:   return -1;
    }
  }

  void stepNavtex(uint8_t bit, int bitLen)
  {
    if(++sampleInBit < bitLen) return;
    sampleInBit = 0;

    sitorReg = (uint8_t)((sitorReg >> 1) | (bit ? 0x40 : 0x00));
    if(!sitorSync)
    {
      if(sitorToIta2(sitorReg) >= 0)
      {
        sitorSync = true;
        sitorBitPos = 0;
      }
      return;
    }
    if(++sitorBitPos >= 7)
    {
      sitorBitPos = 0;
      int8_t ita2 = sitorToIta2(sitorReg);
      if(ita2 >= 0) decodeIta2((uint8_t)ita2);
      else sitorSync = false;
    }
  }
};

#endif // RTTY_DECODER_H
