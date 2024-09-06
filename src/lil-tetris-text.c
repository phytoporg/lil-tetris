#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>

static TTF_Font* g_pDefaultFont = NULL;

typedef struct
{
    char Message[64]; // We really don't need much
    int  Size;
    SDL_Texture* pTexture;
    SDL_Surface* pSurface;
} TextEntry_t;

typedef Uint8 HTEXT;

#define TEXT_FONT "pixel_digivolve.otf"
#define TEXT_MAX_ENTRIES 8
#define TEXT_INVALID_HANDLE 255

static TextEntry_t g_TextEntries[TEXT_MAX_ENTRIES];
static Uint8 g_TextLastEntry = TEXT_INVALID_HANDLE;
static bool g_TextInitialized = false;

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

    memset(g_TextEntries, 0, sizeof(g_TextEntries));

    g_TextInitialized = true;
    return true;
}

HTEXT TextCreateEntry()
{
    if (!g_TextInitialized)
    {
        fprintf(stderr, "Failed to create text entry: text is not initialized.\n");
        return TEXT_INVALID_HANDLE;
    }

    if (g_TextLastEntry != TEXT_INVALID_HANDLE && 
        g_TextLastEntry >= (TEXT_MAX_ENTRIES - 1))
    {
        fprintf(stderr, "Failed to create text entry: too many existing entries.\n");
        return TEXT_INVALID_HANDLE;
    }

    g_TextLastEntry++;
    HTEXT hEntry = (HTEXT)g_TextLastEntry;
    return hEntry;
}

bool TextSetEntryData(HTEXT hText, SDL_Renderer* pRenderer, char* pMessage)
{
    if (!g_TextInitialized)
    {
        fprintf(stderr, "Failed to set text data: text is not initialized.\n");
        return false;
    }

    if (hText > g_TextLastEntry || hText == TEXT_INVALID_HANDLE)
    {
        fprintf(stderr, "Failed to set text data: unexpected handle.\n");
        return false;
    }

    // We need to create a new surface and texture if:
    // - The entry currently has no valid surface/texture
    // - The entry has valid surface/texture but the message doesn't match
    // Otherwise, just use what's there
    TextEntry_t* pEntry = &g_TextEntries[hText];
    if ((!pEntry->pSurface || !pEntry->pTexture) || 
         strlen(pEntry->Message) != strlen(pMessage) ||
         strncmp(pEntry->Message, pMessage, pEntry->Size) != 0)
    {
        strncpy(pEntry->Message, pMessage, sizeof(pEntry->Message));
        pEntry->Size = strlen(pEntry->Message);

        // Free any existing resources
        if (pEntry->pSurface)
        {
            SDL_FreeSurface(pEntry->pSurface);
        }

        if (pEntry->pTexture)
        {
            SDL_DestroyTexture(pEntry->pTexture);
        }

        if (!pEntry->Size)
        {
            // If the message is now empty, there's nothing further to do
            return true;
        }

        const SDL_Color kWhite = { 255, 255, 255};
        pEntry->pSurface = TTF_RenderText_Solid(g_pDefaultFont, pMessage, kWhite);
        if (!pEntry->pSurface)
        {
            fprintf(stderr, "Failed to create text surface: %s\n", SDL_GetError());
            return false;
        }

        pEntry->pTexture = SDL_CreateTextureFromSurface(pRenderer, pEntry->pSurface);
        if (!pEntry->pTexture)
        {
            fprintf(stderr, "Failed to create text texture: %s\n", SDL_GetError());
            return false;
        }
    }

    return true;
}

bool TextDrawEntry(HTEXT hText, SDL_Renderer* pRenderer, int x, int y)
{
    if (!g_TextInitialized)
    {
        fprintf(stderr, "Failed to draw text: text is not initialized.\n");
        return false;
    }

    if (hText > g_TextLastEntry || hText == TEXT_INVALID_HANDLE)
    {
        fprintf(stderr, "Failed to draw text: unexpected handle.\n");
        return false;
    }

    TextEntry_t* pEntry = &g_TextEntries[hText];
    int w, h;
    if (TTF_SizeUTF8(g_pDefaultFont, pEntry->Message, &w, &h) < 0)
    {
        fprintf(stderr, "Failed to size text: %s\n", SDL_GetError());
        return false;
    }

    SDL_Rect messageRect;
    messageRect.x = x;
    messageRect.y = y;
    messageRect.w = w;
    messageRect.h = h;

    SDL_RenderCopy(pRenderer, pEntry->pTexture, NULL, &messageRect);
    return true;
}

