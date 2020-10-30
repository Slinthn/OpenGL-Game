#pragma warning(push, 0)
#include <stdint.h>
#include <windows.h>
#pragma warning(pop)

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

void writeData(char c0, char c1, char *pointer, u64 maxPointer, r32 *writePointer, u32 numbers) {
  while ((u64)pointer < maxPointer) {
    if (*pointer == c0 && *(pointer + 1) == c1) {
      char *start = pointer + 1;
      char *end = start;
      for (u32 i = 0; i < numbers; i++)
        *writePointer++ = strtof(end + 1, &end);
    }
    
    while (*pointer++ != '\n' && (u64)pointer < maxPointer);
  }
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int show) {
  HANDLE file = CreateFileA("model.obj", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return -1;

  u32 filesize = GetFileSize(file, 0);
  void *filememory = VirtualAlloc(0, filesize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE); 
  ReadFile(file, filememory, filesize, 0, 0);
  CloseHandle(file);
  
  u32 vertexCount = 0,
    textureCount = 0,
    normalCount = 0,
    triangleCount = 0;

  // TODO(slin): I was writing this because i noticed that faces sometimes were squares and not
  // triangles as I expected. What i didn't expect is for both triangles AND squares to be used.
  // I fucking give up for now, like seriously just use triangles people. Can't make this shit
  // any harder like.
  // Oh, and we have pentagons too??? hello???????? are we ok people?
  u32 vertexWidth = 0,
    textureWidth = 0,
    normalWidth = 0,
    triangleWidth = 0;

  u32 vertexSize = 0,
    textureSize = 0,
    normalSize = 0,
    triangleSize = 0;
  
  u32 *currentWidth = 0;
  u32 *currentSize = 0;
  
  char *pointer = filememory;
  u64 maxPointer = (u64)((u8 *)filememory + filesize);
  while ((u64)pointer < maxPointer) {
    if (*pointer == 'v' && *(pointer + 1) == ' ') {
      vertexCount++;
      currentWidth = &vertexWidth;
      currentSize = &vertexSize;
    } else if (*pointer == 'v' && *(pointer + 1) == 't') {
      textureCount++;
      currentWidth = &textureWidth;
      currentSize = &textureSize;
    } else if (*pointer == 'v' && *(pointer + 1) == 'n') {
      normalCount++;
      currentWidth = &normalWidth;
      currentSize = &normalSize;
    } else if (*pointer == 'f' && *(pointer + 1) == ' ') {
      triangleCount++;
      currentWidth = &triangleWidth;
      currentSize = &triangleSize;
    } else {
      currentWidth = 0;
      currentSize = 0;
    }
    
    u32 width = 0;
    b32 space = 0;
    while (*pointer++ != '\n' && (u64)pointer < maxPointer) {
      if (*pointer == ' ') {
        if (!space)
          width++;
        space = 1;
      } else
        space = 0;
    }
    if (currentWidth)
      *currentWidth = width;
    if (currentSize)
      *currentSize += width*4;
  }

  u32 totalSize = 2*sizeof(u32) + triangleCount*3*sizeof(u32) + 8*sizeof(r32)*vertexCount;
  r32 *outputmemory = VirtualAlloc(0, totalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

  r32 *vertexMemory = VirtualAlloc(0, vertexCount*3*sizeof(r32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  r32 *textureMemory = VirtualAlloc(0, textureCount*2*sizeof(r32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  r32 *normalMemory = VirtualAlloc(0, normalCount*3*sizeof(r32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  u32 *triangleMemory = VirtualAlloc(0, triangleCount*3*3*sizeof(u32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

  writeData('v', ' ', filememory, maxPointer, vertexMemory, 3);
  writeData('v', 't', filememory, maxPointer, textureMemory, 2);
  writeData('v', 'n', filememory, maxPointer, normalMemory, 3);
  
  pointer = filememory;
  u32 *uwritePointer = triangleMemory;
  while ((u64)pointer < maxPointer) {
    if (*pointer == 'f' && *(pointer + 1) == ' ') {
      char *start = pointer + 2;
      char *end = 0;

      for (u32 i = 0; i < 3; i++) {
        *uwritePointer++ = strtol(start, &end, 10) - 1;
        *uwritePointer++ = strtol(end + 1, &end, 10) - 1;
        *uwritePointer++ = strtol(end + 1, &end, 10) - 1;
        start = end + 1;
      }
    }
    
    while (*pointer++ != '\n' && (u64)pointer < maxPointer);
  }

  r32 *output = outputmemory + 2;

  ((u32 *)outputmemory)[0] = triangleCount;
  ((u32 *)outputmemory)[1] = vertexCount;
  
  u32 *startOfIndices = (u32 *)output;
  r32 *startOfVertices = output + triangleCount*3;
  r32 *startOfTextures = output + triangleCount*3 + vertexCount*3;
  r32 *startOfNormals = output + triangleCount*3 + vertexCount*5;

  for (u32 i = 0; i < triangleCount*3*3; i += 3) {
    u32 realI = i / 3;
    
    u32 vertexIndex = triangleMemory[i];
    u32 textureIndex = triangleMemory[i + 1];
    u32 normalIndex = triangleMemory[i + 2];

    r32 vx = vertexMemory[vertexIndex*3];
    r32 vy = vertexMemory[vertexIndex*3 + 1];
    r32 vz = vertexMemory[vertexIndex*3 + 2];

    r32 tx = textureMemory[textureIndex*2];
    r32 ty = textureMemory[textureIndex*2 + 1];

    r32 nx = normalMemory[normalIndex*3];
    r32 ny = normalMemory[normalIndex*3 + 1];
    r32 nz = normalMemory[normalIndex*3 + 2];

    startOfIndices[realI] = vertexIndex;
    
    startOfVertices[vertexIndex*3] = vx;
    startOfVertices[vertexIndex*3 + 1] = vy;
    startOfVertices[vertexIndex*3 + 2] = vz;

    startOfTextures[vertexIndex*2] = tx;
    startOfTextures[vertexIndex*2 + 1] = ty;

    startOfNormals[vertexIndex*3] = nx;
    startOfNormals[vertexIndex*3 + 1] = ny;
    startOfNormals[vertexIndex*3 + 2] = nz;
  }

  file = CreateFileA("model.sobj", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return -1;
  
  DWORD written;
  WriteFile(file, outputmemory, totalSize, &written, 0);
  CloseHandle(file);
  return 0;
}
