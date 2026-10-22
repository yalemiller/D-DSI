#include "palette.h"

#define RGB(hex) ((uint16_t)((((hex) >> 19) & 31) | ((((hex) >> 11) & 31) << 5) | ((((hex) >> 3) & 31) << 10)))

// Columns: bg, panel, panel2, line, text, dim, gold, red, green, blue, ink
const Theme THEMES[THEME_COUNT] = {
    { "TORCHLIGHT",
      { RGB(0x181010), RGB(0x282018), RGB(0x383028), RGB(0x585040), RGB(0xF0E0C0), RGB(0xA89878), RGB(0xF0A830),
        RGB(0xD84830), RGB(0x78A848), RGB(0x7898D0), RGB(0x181010) } },
    { "GRIMOIRE",
      { RGB(0x200810), RGB(0x381020), RGB(0x501830), RGB(0x703048), RGB(0xF8E8C8), RGB(0xC8A090), RGB(0xE8C050),
        RGB(0xF06048), RGB(0x80C068), RGB(0x88A8E8), RGB(0x200810) } },
    { "ELVEN",
      { RGB(0x101810), RGB(0x182818), RGB(0x283828), RGB(0x405840), RGB(0xE8F0D0), RGB(0x98B090), RGB(0xD8B048),
        RGB(0xE06048), RGB(0x88D068), RGB(0x78B0E0), RGB(0x101810) } },
    { "PARCHMENT",
      { RGB(0xE8D8B0), RGB(0xD8C898), RGB(0xC8B080), RGB(0xA08860), RGB(0x281810), RGB(0x584830), RGB(0x903010),
        RGB(0xA82818), RGB(0x386820), RGB(0x284880), RGB(0xF8F0D8) } },
    { "NIGHT",
      { RGB(0x181020), RGB(0x282038), RGB(0x383050), RGB(0x504868), RGB(0xF8F0D8), RGB(0xA098A8), RGB(0xF8B840),
        RGB(0xE04848), RGB(0x58C068), RGB(0x5890E8), RGB(0x181020) } },
};
