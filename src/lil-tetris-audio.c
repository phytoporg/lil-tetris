#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_mixer.h>

#define AUDIO_PATH_MUSIC "music.mp3"
#define AUDIO_PATH_LINECLEAR "lineclear.mp3"
#define AUDIO_PATH_GAMEOVER "gameover.mp3"

static Mix_Music* g_pMusic = NULL;
static Mix_Chunk* g_pLineClearChunk = NULL;
static Mix_Chunk* g_pGameOverChunk = NULL;

bool AudioInitialize(char* pAssetPathRoot)
{
    if (!pAssetPathRoot)
    {
        fprintf(stderr, "Audio asset path root is null\n");
        return false;
    }

    if (Mix_Init(MIX_INIT_MP3) == 0)
    {
        fprintf(stderr, "Failed to initialize SDL Mixer: %s\n", SDL_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
    {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        return false;
    }

    // Just double check
    char fullPath[512];
    const int rootPathLen = strlen(pAssetPathRoot);
    const int musicPathLen = strlen(AUDIO_PATH_MUSIC);
    const int lineclearPathLen = strlen(AUDIO_PATH_LINECLEAR);
    const int gameoverPathLen = strlen(AUDIO_PATH_GAMEOVER);

    if ((rootPathLen + musicPathLen + 1) >= sizeof(fullPath))
    {
        fprintf(stderr, "Music file path is too long\n");
        return false;
    }

    if ((rootPathLen + lineclearPathLen + 1) >= sizeof(fullPath))
    {
        fprintf(stderr, "lineclear file path is too long\n");
        return false;
    }

    if ((rootPathLen + gameoverPathLen + 1) >= sizeof(fullPath))
    {
        fprintf(stderr, "lineclear file path is too long\n");
        return false;
    }

    sprintf(fullPath, "%s/%s", pAssetPathRoot, AUDIO_PATH_MUSIC);
    g_pMusic = Mix_LoadMUS(fullPath);
    if (!g_pMusic)
    {
        fprintf(stderr, "Failed to load music: %s\n", SDL_GetError());
        return false;
    }

    sprintf(fullPath, "%s/%s", pAssetPathRoot, AUDIO_PATH_LINECLEAR);
    g_pLineClearChunk = Mix_LoadWAV(fullPath);
    if (!g_pLineClearChunk)
    {
        fprintf(stderr, "Failed to load lineclear sfx: %s\n", SDL_GetError());
        return false;
    }

    sprintf(fullPath, "%s/%s", pAssetPathRoot, AUDIO_PATH_GAMEOVER);
    g_pGameOverChunk = Mix_LoadWAV(fullPath);
    if (!g_pGameOverChunk)
    {
        fprintf(stderr, "Failed to load gameover sfx: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void AudioUninitialize()
{
    Mix_FreeChunk(g_pGameOverChunk);
    Mix_FreeChunk(g_pLineClearChunk);
    Mix_FreeMusic(g_pMusic);
    Mix_CloseAudio();
    Mix_Quit();
}

bool AudioPlayMusic()
{
    if (!g_pMusic)
    {
        return false;
    }
    
    if (Mix_PlayingMusic())
    {
        return true;
    }

    return Mix_PlayMusic(g_pMusic, -1) >= 0;
}

void AudioPauseMusic()
{
    if (Mix_PlayingMusic())
    {
        Mix_PauseMusic();
    }
}

void AudioResumeMusic()
{
    if (Mix_PausedMusic())
    {
        Mix_ResumeMusic();
    }
}

void AudioStopMusic()
{
    if (Mix_PlayingMusic())
    {
        Mix_HaltMusic();
    }
}

bool AudioPlayLineClear()
{
    if (!g_pLineClearChunk)
    {
        return false;
    }

    return Mix_PlayChannel(-1, g_pLineClearChunk, 0) >= 0;
}

bool AudioPlayGameOver()
{
    if (!g_pGameOverChunk)
    {
        return false;
    }

    return Mix_PlayChannel(-1, g_pGameOverChunk, 0) >= 0;
}
