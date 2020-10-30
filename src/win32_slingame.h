#pragma warning(push, 0)
#include <stdint.h>
#include <windows.h>
#include <gl/gl.h>
#include "gltext.h"
#include <xinput.h>
#include <dsound.h>
#pragma warning(pop)

#define assert(exp) if (!(exp)) *((u8 *)0) = 0;

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef s32 b32;
typedef float r32;
typedef double r64;

PFNGLCREATESHADERPROC glCreateShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;

PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;

PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

typedef struct {
  void *memory;
  u32 size;
} file;
