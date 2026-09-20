/* Async screen capture: memcpy into a pending slot; writer thread fwrite → ffmpeg.
 * Encode stays off the attract/vsync path; if the writer is busy the frame drops.
 */

#include "sys24_viewer_record.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

static pthread_t g_thread;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static int g_running;
static int g_have_frame;
static int g_active;
static size_t g_frame_bytes;
static u8 *g_pending;
static u8 *g_scratch;
static FILE *g_pipe;
static char g_path[512];
static unsigned g_frames_written;
static unsigned g_frames_dropped;

static int path_wants_avi(const char *path)
{
    size_t n;

    if (!path)
        return 1;
    n = strlen(path);
    if (n >= 4 && (strcmp(path + n - 4, ".avi") == 0 || strcmp(path + n - 4, ".AVI") == 0))
        return 1;
    return 0;
}

static void *record_writer(void *arg)
{
    (void)arg;

    for (;;) {
        pthread_mutex_lock(&g_mu);
        while (g_running && !g_have_frame)
            pthread_cond_wait(&g_cv, &g_mu);
        if (!g_running && !g_have_frame) {
            pthread_mutex_unlock(&g_mu);
            break;
        }
        memcpy(g_scratch, g_pending, g_frame_bytes);
        g_have_frame = 0;
        pthread_mutex_unlock(&g_mu);

        if (g_pipe) {
            size_t n = fwrite(g_scratch, 1, g_frame_bytes, g_pipe);

            if (n != g_frame_bytes) {
                fprintf(stderr, "lift: record fwrite short (%zu/%zu): %s\n",
                        n, g_frame_bytes, strerror(errno));
                break;
            }
            g_frames_written++;
        }
    }
    return NULL;
}

int sys24_viewer_record_start(const char *path, int width, int height, int fps)
{
    char cmd[1024];
    int avi;

    if (g_active)
        sys24_viewer_record_stop();

    if (!path || !*path || width < 1 || height < 1) {
        fprintf(stderr, "lift: record_start: bad args\n");
        return -1;
    }
    if (fps < 1)
        fps = 60;

#if defined(_WIN32)
    fprintf(stderr, "lift: --record not supported on this host\n");
    return -1;
#else
    /* Packed RGB24 — BGRA with A=0 decoded as black in some MJPEG players. */
    g_frame_bytes = (size_t)width * (size_t)height * 3u;
    g_have_frame = 0;
    g_frames_written = 0;
    g_frames_dropped = 0;
    snprintf(g_path, sizeof(g_path), "%s", path);

    g_pending = (u8 *)malloc(g_frame_bytes);
    g_scratch = (u8 *)malloc(g_frame_bytes);
    if (!g_pending || !g_scratch) {
        fprintf(stderr, "lift: record alloc failed\n");
        free(g_pending);
        free(g_scratch);
        g_pending = NULL;
        g_scratch = NULL;
        return -1;
    }

    avi = path_wants_avi(path);
    if (avi) {
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -loglevel error -y -f rawvideo -pix_fmt rgb24 "
                 "-video_size %dx%d -framerate %d -i pipe:0 "
                 "-an -c:v mjpeg -q:v 5 \"%s\"",
                 width, height, fps, path);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -loglevel error -y -f rawvideo -pix_fmt rgb24 "
                 "-video_size %dx%d -framerate %d -i pipe:0 "
                 "-an -c:v libx264 -preset ultrafast -crf 23 -pix_fmt yuv420p \"%s\"",
                 width, height, fps, path);
    }

    g_pipe = popen(cmd, "w");
    if (!g_pipe) {
        fprintf(stderr, "lift: record: popen ffmpeg failed (is ffmpeg installed?)\n");
        free(g_pending);
        free(g_scratch);
        g_pending = NULL;
        g_scratch = NULL;
        return -1;
    }

    g_running = 1;
    if (pthread_create(&g_thread, NULL, record_writer, NULL) != 0) {
        fprintf(stderr, "lift: record: pthread_create failed\n");
        pclose(g_pipe);
        g_pipe = NULL;
        g_running = 0;
        free(g_pending);
        free(g_scratch);
        g_pending = NULL;
        g_scratch = NULL;
        return -1;
    }

    g_active = 1;
    fprintf(stderr,
            "lift: recording %dx%d @ %d fps → %s (%s, async; drops if encode lags)\n",
            width, height, fps, path, avi ? "mjpeg avi" : "x264");
    return 0;
#endif
}

int sys24_viewer_record_active(void)
{
    return g_active;
}

void sys24_viewer_record_submit_bgra(const void *pixels, size_t nbytes)
{
    if (!g_active || !pixels || nbytes < g_frame_bytes)
        return;

    pthread_mutex_lock(&g_mu);
    if (g_have_frame) {
        g_frames_dropped++;
        pthread_mutex_unlock(&g_mu);
        return;
    }
    memcpy(g_pending, pixels, g_frame_bytes);
    g_have_frame = 1;
    pthread_cond_signal(&g_cv);
    pthread_mutex_unlock(&g_mu);
}

void sys24_viewer_record_stop(void)
{
    if (!g_active)
        return;

    pthread_mutex_lock(&g_mu);
    g_running = 0;
    pthread_cond_signal(&g_cv);
    pthread_mutex_unlock(&g_mu);

    pthread_join(g_thread, NULL);

    if (g_pipe) {
        int rc = pclose(g_pipe);

        g_pipe = NULL;
        if (rc != 0)
            fprintf(stderr, "lift: ffmpeg exit status %d\n", rc);
    }

    free(g_pending);
    free(g_scratch);
    g_pending = NULL;
    g_scratch = NULL;

    fprintf(stderr,
            "lift: record stopped — wrote %u frame(s), dropped %u → %s\n",
            g_frames_written, g_frames_dropped, g_path);
    g_active = 0;
    g_have_frame = 0;
}
