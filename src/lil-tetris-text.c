#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

static TTF_Font* g_pDefaultFont = NULL;
static SDL_Texture* g_pTextTexture = NULL;
static SDL_Surface* g_pTextSurface = NULL;

#define TEXT_FONT "pixel_digivolve.otf"

bool TextInitialize(SDL_Renderer* pRenderer, char* pAssetPathRoot)
{
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "TTF_Init() failed: %s\n", SDL_GetError());
        return false;
    }

    char fullPath[512];
    const int rootPathLen = strlen(pAssetPathRoot);
    const int fontPathLen = strlen(TEXT_FONT);
    if ((rootPathLen + fontPathLen + 1) >= sizeof(fullPath))
    {
        fprintf(stderr, "Font file path is too long\n");
        return false;
    }

    sprintf(fullPath, "%s/%s", pAssetPathRoot, TEXT_FONT);

    g_pDefaultFont = TTF_OpenFont(fullPath, 24);
    if (!g_pDefaultFont)
    {
        fprintf(stderr, "TTF_OpenFont() failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void TextUninitialize()
{
    if (g_pTextSurface)
    {
        SDL_FreeSurface(g_pTextSurface);
        g_pTextSurface = NULL;
    }

    if (g_pTextTexture)
    {
        SDL_DestroyTexture(g_pTextTexture);
        g_pTextTexture = NULL;
    }
}

void TextWrite(SDL_Renderer* pRenderer, char* pMessage, int x, int y)
{
    SDL_Color white = { 255, 255, 255};
    if (!g_pTextSurface)
    {
        g_pTextSurface = TTF_RenderText_Solid(g_pDefaultFont, pMessage, white);
        if (!g_pTextSurface)
        {
            fprintf(stderr, "Failed to create text surface: %s\n", SDL_GetError());
            return;
        }
    }

    if (!g_pTextTexture)
    {
        g_pTextTexture = SDL_CreateTextureFromSurface(pRenderer, g_pTextSurface);
        if (!g_pTextTexture)
        {
            fprintf(stderr, "Failed to create text texture: %s\n", SDL_GetError());
            return;
        }
    }

    int w, h;
    if (TTF_SizeUTF8(g_pDefaultFont, pMessage, &w, &h) < 0)
    {
        fprintf(stderr, "Failed to size text: %s\n", SDL_GetError());
        return;
    }

    SDL_Rect messageRect;
    messageRect.x = x;
    messageRect.y = y;
    messageRect.w = w;
    messageRect.h = h;

    SDL_RenderCopy(pRenderer, g_pTextTexture, NULL, &messageRect);
}
