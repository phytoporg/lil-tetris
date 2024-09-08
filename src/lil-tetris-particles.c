#include <SDL2/SDL.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
    Uint8 FramesSinceSpawn;
    Uint8 Lifetime;
    int   X;
    int   Y;
} SquareParticle_t;

// Should be way more than we need
#define MAX_PARTICLES 255

// Parallel arrays
static SquareParticle_t g_Particles[MAX_PARTICLES];
static bool g_ValidParticles[MAX_PARTICLES];
static bool g_ParticlesInitialized = false;
static Uint8 g_ParticleCount = 0;

// i want to emit a stream of particles which die after some time
// let's start there

void ParticlesInitialize()
{
    memset(g_Particles, 0, sizeof(g_Particles));
    memset(g_ValidParticles, 0, sizeof(g_ValidParticles));
    g_ParticlesInitialized = true;
}

SquareParticle_t* MakeParticle()
{
    assert(g_ParticlesInitialized);

    if (g_ParticleCount >= MAX_PARTICLES)
    {
        fprintf(stderr, "Exceeded max particle count\n");
        return NULL;
    }

    // Find the next available particle slot
    Sint8 nextParticleIndex = -1;
    for (Uint8 i = 0; i < MAX_PARTICLES; ++i)
    {
        if (!g_ValidParticles[i])
        {
            nextParticleIndex = i;
            break;
        }
    }

    if (nextParticleIndex < 0)
    {
        // We shouldn't get here
        assert(false);
        fprintf(stderr, "Could not find available particle in MakeParticle()\n");
        return NULL;
    }

    SquareParticle_t* pParticle = &(g_Particles[nextParticleIndex]);
    memset(pParticle, 0, sizeof(*pParticle));

    g_ValidParticles[nextParticleIndex] = true;
    ++g_ParticleCount;

    return pParticle;
}

void ParticlesTick()
{
    assert(g_ParticlesInitialized);

    Sint8 particlesRemaining = g_ParticleCount;
    Uint8 index = 0;
    while(particlesRemaining > 0)
    {
        if (g_ValidParticles[index])
        {
            // TODO
            SquareParticle_t* pParticle = &(g_Particles[index]);
            pParticle->FramesSinceSpawn++;

            if (pParticle->FramesSinceSpawn == pParticle->Lifetime)
            {
                // This particle has expired
                g_ValidParticles[index] = false;
                g_ParticleCount--;
            }

            particlesRemaining--;
        }
        index++;
    }
}

void ParticlesRender(SDL_Renderer* pRenderer)
{
    assert(g_ParticlesInitialized);

    SDL_Rect particleRects[MAX_PARTICLES];
    Sint8 particlesRemaining = g_ParticleCount;
    Uint8 index = 0;
    Uint8 numRects = 0;
    while(particlesRemaining > 0)
    {
        if (g_ValidParticles[index])
        {
            // TODO: temp, obviously
            Uint8 kWidth = 5;
            Uint8 kHeight = 5;

            SquareParticle_t* pParticle = &(g_Particles[index]);
            SDL_Rect* pRect = (&particleRects[numRects]);

            pRect->x = pParticle->X;
            pRect->y = pParticle->Y;
            pRect->w = kWidth;
            pRect->h = kHeight;

            numRects++;
            particlesRemaining--;
        }
        index++;
    }

    if (!numRects)
    {
        // Nothing to draw
        return;
    }

    const SDL_Color kWhite = { 255, 255, 255, SDL_ALPHA_OPAQUE };
    SDL_SetRenderDrawColor( pRenderer, kWhite.r, kWhite.g, kWhite.b, kWhite.a);
    SDL_RenderFillRects(pRenderer, particleRects, numRects);
}

Uint8 ParticlesGetCount()
{
    assert(g_ParticlesInitialized);
    return g_ParticleCount;
}
