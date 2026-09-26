/* Host OpenGL headers + Windows 1.2+/2.0 loader.
 *
 * macOS / Linux link shader entry points from the system GL library.
 * Windows <GL/gl.h> is 1.1 — load the rest via SDL_GL_GetProcAddress (or wgl).
 */
#ifndef MODEL2_GL_H
#define MODEL2_GL_H

#ifdef I960_HOST_HAVE_GL

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812Fu
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1u
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367u
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0u
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1u
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31u
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30u
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81u
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82u
#endif

#if defined(_WIN32)

#ifndef GLAPIENTRY
#ifdef APIENTRY
#define GLAPIENTRY APIENTRY
#else
#define GLAPIENTRY
#endif
#endif

#ifndef GL_CHAR_DEFINED_HOST
typedef char GLchar;
#endif

typedef GLuint (GLAPIENTRY *PFN_model2_glCreateShader)(GLenum);
typedef void (GLAPIENTRY *PFN_model2_glShaderSource)(GLuint, GLsizei, const GLchar *const *,
                                                    const GLint *);
typedef void (GLAPIENTRY *PFN_model2_glCompileShader)(GLuint);
typedef void (GLAPIENTRY *PFN_model2_glGetShaderiv)(GLuint, GLenum, GLint *);
typedef void (GLAPIENTRY *PFN_model2_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void (GLAPIENTRY *PFN_model2_glDeleteShader)(GLuint);
typedef GLuint (GLAPIENTRY *PFN_model2_glCreateProgram)(void);
typedef void (GLAPIENTRY *PFN_model2_glAttachShader)(GLuint, GLuint);
typedef void (GLAPIENTRY *PFN_model2_glLinkProgram)(GLuint);
typedef void (GLAPIENTRY *PFN_model2_glDeleteProgram)(GLuint);
typedef void (GLAPIENTRY *PFN_model2_glGetProgramiv)(GLuint, GLenum, GLint *);
typedef void (GLAPIENTRY *PFN_model2_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLint (GLAPIENTRY *PFN_model2_glGetUniformLocation)(GLuint, const GLchar *);
typedef void (GLAPIENTRY *PFN_model2_glUseProgram)(GLuint);
typedef void (GLAPIENTRY *PFN_model2_glActiveTexture)(GLenum);
typedef void (GLAPIENTRY *PFN_model2_glUniform1i)(GLint, GLint);
typedef void (GLAPIENTRY *PFN_model2_glUniform1f)(GLint, GLfloat);
typedef void (GLAPIENTRY *PFN_model2_glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (GLAPIENTRY *PFN_model2_glUniform2f)(GLint, GLfloat, GLfloat);

extern PFN_model2_glCreateShader model2_glCreateShader;
extern PFN_model2_glShaderSource model2_glShaderSource;
extern PFN_model2_glCompileShader model2_glCompileShader;
extern PFN_model2_glGetShaderiv model2_glGetShaderiv;
extern PFN_model2_glGetShaderInfoLog model2_glGetShaderInfoLog;
extern PFN_model2_glDeleteShader model2_glDeleteShader;
extern PFN_model2_glCreateProgram model2_glCreateProgram;
extern PFN_model2_glAttachShader model2_glAttachShader;
extern PFN_model2_glLinkProgram model2_glLinkProgram;
extern PFN_model2_glDeleteProgram model2_glDeleteProgram;
extern PFN_model2_glGetProgramiv model2_glGetProgramiv;
extern PFN_model2_glGetProgramInfoLog model2_glGetProgramInfoLog;
extern PFN_model2_glGetUniformLocation model2_glGetUniformLocation;
extern PFN_model2_glUseProgram model2_glUseProgram;
extern PFN_model2_glActiveTexture model2_glActiveTexture;
extern PFN_model2_glUniform1i model2_glUniform1i;
extern PFN_model2_glUniform1f model2_glUniform1f;
extern PFN_model2_glUniform4f model2_glUniform4f;
extern PFN_model2_glUniform2f model2_glUniform2f;

#define glCreateShader model2_glCreateShader
#define glShaderSource model2_glShaderSource
#define glCompileShader model2_glCompileShader
#define glGetShaderiv model2_glGetShaderiv
#define glGetShaderInfoLog model2_glGetShaderInfoLog
#define glDeleteShader model2_glDeleteShader
#define glCreateProgram model2_glCreateProgram
#define glAttachShader model2_glAttachShader
#define glLinkProgram model2_glLinkProgram
#define glDeleteProgram model2_glDeleteProgram
#define glGetProgramiv model2_glGetProgramiv
#define glGetProgramInfoLog model2_glGetProgramInfoLog
#define glGetUniformLocation model2_glGetUniformLocation
#define glUseProgram model2_glUseProgram
#define glActiveTexture model2_glActiveTexture
#define glUniform1i model2_glUniform1i
#define glUniform1f model2_glUniform1f
#define glUniform4f model2_glUniform4f
#define glUniform2f model2_glUniform2f

#endif /* _WIN32 */

#endif /* I960_HOST_HAVE_GL */

int model2_gl_load(void);

#endif /* MODEL2_GL_H */
