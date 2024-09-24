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
    { { 50,   50,  50}, {  0,   0,   0} }, // PATTERN_NONE
    { {248,  120,  32}, {248, 216,  96} }, // PATTERN_L_L
    { { 96,   96, 248}, {192, 192, 248} }, // PATTERN_L_R,
    { {240,   24,  24}, {248, 104, 104} }, // PATTERN_Z_L,
    { {  0,  160,   0}, {128, 248, 128} }, // PATTERN_Z_R,
    { {216,   40, 216}, {248, 160, 248} }, // PATTERN_T_SHAPE,
    { { 32,  200, 248}, {160, 248, 248} }, // PATTERN_LINE_SHAPE,
    { {248,  208,   0}, {248, 248, 160} }, // PATTERN_SQUARE_SHAPE,
};

static PatternTheme g_ForestThemes[NUM_THEMES] = {
    { {  0,   0,   0}, {  0,  0,    0} },
    { { 34,  56,  30}, { 85, 107,  47} },
    { { 42,  30,  12}, {139,  69,  19} },
    { { 25,  61,  40}, {107, 142,  35} },
    { { 55,  47,  15}, {189, 183, 107} },
    { { 24,  45,  33}, { 60, 179, 113} },
    { { 44,  66,  22}, {154, 205,  50} },
    { { 10,  28,  10}, { 34, 139,  34} },
};

static PatternTheme g_SeaThemes[NUM_THEMES] = {
    { {  0,   0,   0}, {  0,  0,    0} },
    { {0, 51, 102}, {70, 130, 180} },
    { {0, 105, 148}, {135, 206, 250} },
    { {0, 119, 190}, {173, 216, 230} },
    { {25, 25, 112}, {65, 105, 225} },
    { {0, 77, 102}, {72, 209, 204} },
    { {0, 128, 128}, {64, 224, 208} },
    { {0, 51, 102}, {100, 149, 237} },
};

static PatternTheme g_FireThemes[NUM_THEMES] = {
    { {  0,   0,  0}, {  0,  0,    0} },
    { {139,   0,  0}, {255, 69,    0} },
    { {178,  34, 34}, {255, 99,   71} },
    { {165,  42, 42}, {255, 140,   0} },
    { {255,  69,  0}, {255, 165,   0} },
    { {255, 140,  0}, {255, 215,   0} },
    { {204,  85,  0}, {255, 160, 122} },
    { {255,  0,   0}, {255,  99,  71} }
};

static PatternTheme g_EarthThemes[NUM_THEMES] = {
    { {  0,   0,  0}, {  0,   0,   0} },
    { {101,  67, 33}, {210, 180, 140} },
    { {139,  69, 19}, {205, 133,  63} },
    { {160,  82, 45}, {222, 184, 135} },
    { {128,  70, 27}, {210, 105,  30} },
    { {153, 101, 21}, {244, 164,  96} },
    { {112,  66, 20}, {184, 134,  11} },
    { {85,   62, 42}, {188, 143, 143} },
};

static PatternTheme g_GoldThemes[NUM_THEMES] = {
    { {  0,   0,   0}, {  0,   0,   0} },
    { {184, 134,  11}, {255, 215,   0} },
    { {218, 165,  32}, {255, 223,   0} },
    { {184, 134,  11}, {238, 201,   0} },
    { {205, 173,   0}, {255, 193,  37} },
    { {218, 165,  32}, {255, 185,  15} },
    { {189, 183, 107}, {255, 246, 143} },
    { {184, 134,  11}, {255, 220, 135} },
};

static PatternTheme g_SinCityThemes[NUM_THEMES] = {
    { {  0,  0,  0}, {  0,  0,    0} },
	{ { 10, 10, 10}, {200, 200, 200} },
    { { 30, 30, 30}, {220, 220, 220} },
    { {  0,  0,  0}, {255, 255, 255} },
    { { 50, 50, 50}, {240, 240, 240} },
    { { 80, 80, 80}, {255,   0,   0} },
    { { 20, 20, 20}, {255, 255, 255} },
    { { 40, 40, 40}, {180,   0,   0} },
};

static PatternTheme* g_AllThemes[] = {
	g_DefaultThemes,
	g_ForestThemes,
	g_SeaThemes,
	g_FireThemes,
	g_EarthThemes,
	g_GoldThemes,
	g_SinCityThemes,
};

#define ALL_THEME_COUNT (sizeof(g_AllThemes) / sizeof(g_AllThemes[0]))

PatternTheme* ThemeGetNextTheme(PatternTheme* pTheme)
{
    int currentThemeIndex = -1;
    for (int i = 0; i < ALL_THEME_COUNT; ++i)
    {
        if (g_AllThemes[i] == pTheme)
        {
            currentThemeIndex = i;
            break;
        }
    }

    if (currentThemeIndex < 0)
    {
        // This is unexpected
        fprintf(stderr, "Couldn't find current theme among all themes\n");
        return NULL;
    }

    int nextThemeIndex = (currentThemeIndex + 1) % ALL_THEME_COUNT;
    return g_AllThemes[nextThemeIndex];
}

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

Color ThemeBlendColor(Color* pColorA, Color* pColorB, float alpha)
{
    Color returnColor;
    returnColor.r = alpha * pColorB->r + (1.0f - alpha) * pColorA->r;
    returnColor.g = alpha * pColorB->g + (1.0f - alpha) * pColorA->g;
    returnColor.b = alpha * pColorB->b + (1.0f - alpha) * pColorA->b;

    return returnColor;
}
