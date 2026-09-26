#include "model2_gl.h"

#include <stdint.h>

#ifdef I960_HOST_HAVE_GL

#ifdef I960_HOST_HAVE_SDL
#include <SDL.h>
#endif

#if defined(_WIN32)

#include <stdio.h>

PFN_model2_glCreateShader model2_glCreateShader;
PFN_model2_glShaderSource model2_glShaderSource;
PFN_model2_glCompileShader model2_glCompileShader;
PFN_model2_glGetShaderiv model2_glGetShaderiv;
PFN_model2_glGetShaderInfoLog model2_glGetShaderInfoLog;
PFN_model2_glDeleteShader model2_glDeleteShader;
PFN_model2_glCreateProgram model2_glCreateProgram;
PFN_model2_glAttachShader model2_glAttachShader;
PFN_model2_glLinkProgram model2_glLinkProgram;
PFN_model2_glDeleteProgram model2_glDeleteProgram;
PFN_model2_glGetProgramiv model2_glGetProgramiv;
PFN_model2_glGetProgramInfoLog model2_glGetProgramInfoLog;
PFN_model2_glGetUniformLocation model2_glGetUniformLocation;
PFN_model2_glUseProgram model2_glUseProgram;
PFN_model2_glActiveTexture model2_glActiveTexture;
PFN_model2_glUniform1i model2_glUniform1i;
PFN_model2_glUniform1f model2_glUniform1f;
PFN_model2_glUniform4f model2_glUniform4f;
PFN_model2_glUniform2f model2_glUniform2f;

static void *gl_get_proc(const char *name)
{
    void *p = NULL;

#ifdef I960_HOST_HAVE_SDL
    p = (void *)SDL_GL_GetProcAddress(name);
#endif
    if (!p)
        p = (void *)(uintptr_t)wglGetProcAddress(name);
    return p;
}

int model2_gl_load(void)
{
    static int once;
    static int ok;

    if (once)
        return ok;
    once = 1;
    ok = 1;

#define LOAD(sym, name)                                                      \
    do {                                                                     \
        sym = (PFN_##sym)gl_get_proc(name);                                  \
        if (!sym)                                                            \
            ok = 0;                                                          \
    } while (0)

    LOAD(model2_glCreateShader, "glCreateShader");
    LOAD(model2_glShaderSource, "glShaderSource");
    LOAD(model2_glCompileShader, "glCompileShader");
    LOAD(model2_glGetShaderiv, "glGetShaderiv");
    LOAD(model2_glGetShaderInfoLog, "glGetShaderInfoLog");
    LOAD(model2_glDeleteShader, "glDeleteShader");
    LOAD(model2_glCreateProgram, "glCreateProgram");
    LOAD(model2_glAttachShader, "glAttachShader");
    LOAD(model2_glLinkProgram, "glLinkProgram");
    LOAD(model2_glDeleteProgram, "glDeleteProgram");
    LOAD(model2_glGetProgramiv, "glGetProgramiv");
    LOAD(model2_glGetProgramInfoLog, "glGetProgramInfoLog");
    LOAD(model2_glGetUniformLocation, "glGetUniformLocation");
    LOAD(model2_glUseProgram, "glUseProgram");
    LOAD(model2_glActiveTexture, "glActiveTexture");
    LOAD(model2_glUniform1i, "glUniform1i");
    LOAD(model2_glUniform1f, "glUniform1f");
    LOAD(model2_glUniform4f, "glUniform4f");
    LOAD(model2_glUniform2f, "glUniform2f");
#undef LOAD

    if (!ok)
        fprintf(stderr, "lift: OpenGL 2.0 shader entry points missing\n");
    return ok;
}

#else /* !_WIN32 */

int model2_gl_load(void)
{
    return 1;
}

#endif /* _WIN32 */

#else /* !I960_HOST_HAVE_GL */

int model2_gl_load(void)
{
    return 0;
}

#endif /* I960_HOST_HAVE_GL */
