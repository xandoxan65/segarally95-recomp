/* SDL audio pull from libmodel2_snd. The lib does not link SDL. */

#include "model2_snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef I960_HOST_HAVE_SDL
#include <SDL.h>
#endif

#ifdef I960_HOST_HAVE_SDL
static SDL_AudioDeviceID g_dev;
#endif
static int g_open;

#ifdef I960_HOST_HAVE_SDL
static void snd_audio_cb(void *userdata, Uint8 *stream, int len)
{
    unsigned frames;

    (void)userdata;
    if (len <= 0)
        return;
    frames = (unsigned)len / (unsigned)(2 * (int)sizeof(i16));
    model2_snd_render((signed short *)stream, frames);
}
#endif

int model2_snd_host_audio_open(void)
{
    const char *mute = getenv("I960_SND_MUTE");

    if (mute && mute[0] && mute[0] != '0')
        return 0;
#ifdef I960_HOST_HAVE_SDL
    {
        SDL_AudioSpec want, have;

        if (g_open)
            return 0;
        memset(&want, 0, sizeof(want));
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = snd_audio_cb;
        g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (!g_dev) {
            fprintf(stderr, "lift: sound SDL_OpenAudioDevice failed: %s\n",
                    SDL_GetError());
            return -1;
        }
        SDL_PauseAudioDevice(g_dev, 0);
        g_open = 1;
        fprintf(stderr, "lift: sound audio %d Hz stereo s16 (68k+SCSP)\n",
                have.freq);
        return 0;
    }
#else
    (void)g_open;
    return 0;
#endif
}

void model2_snd_host_audio_close(void)
{
#ifdef I960_HOST_HAVE_SDL
    if (g_dev) {
        SDL_CloseAudioDevice(g_dev);
        g_dev = 0;
    }
#endif
    g_open = 0;
}
