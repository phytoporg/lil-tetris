#include <SDL2/SDL.h>

typedef struct
{
    Uint8 r, g, b;
} Color;

typedef struct
{
    Color Outer;
    Color Inner;
} PatternTheme;

#define NUM_THEMES 8

static PatternTheme g_DefaultThemes[NUM_THEMES] = {
    { {  0,   0,   0}, {  0,  0,    0} }, // PATTERN_NONE
    { {248,  120, 32}, {248, 216,  96} }, // PATTERN_L_L
    { { 96,  96, 248}, {192, 192, 248} }, // PATTERN_L_R,
    { {240,  24,  24}, {248, 104, 104} }, // PATTERN_Z_L,
    { {  0, 160,   0}, {128, 248, 128} }, // PATTERN_Z_R,
    { {216,  40, 216}, {248, 160, 248} }, // PATTERN_T_SHAPE,
    { { 32, 200, 248}, {160, 248, 248} }, // PATTERN_LINE_SHAPE,
    { {248, 208,   0}, {248, 248, 160} }, // PATTERN_SQUARE_SHAPE,
};

Color* ThemeGetOuterColor(PatternTheme* pThemes, int patternIndex)
{
    if (patternIndex < 0 || patternIndex >= NUM_THEMES)
    {
        // Invalid index
        return NULL;
    }

    return &(pThemes[patternIndex].Outer);
}

Color* ThemeGetInnerColor(PatternTheme* pThemes, int patternIndex)
{
    if (patternIndex < 0 || patternIndex >= NUM_THEMES)
    {
        // Invalid index
        return NULL;
    }

    return &(pThemes[patternIndex].Inner);
}
