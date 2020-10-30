typedef struct {
  float m[16];
} matrix4;

typedef struct {
  void *memory;
  b32 initialised;
} gamememory;

typedef struct {
  r32 lStickX;
  r32 lStickY;
  r32 rStickX;
  r32 rStickY;
  b32 btnDown;
} gameinput;

#pragma pack(push, 1)
typedef struct {
  s16 signature;
  u32 bytes;
  s16 reserved0;
  s16 reserved1;
  u32 offset;
  u32 dibHeaderBytes;
  u32 width;
  u32 height;
} bitmapheader;
#pragma pack(pop)

typedef struct {
  file source;
  bitmapheader *header;
  u32 *data;
} bitmap;

typedef struct {
  s8 chunkId[4];
  u32 chunkSize;
  s8 format[4];

  u8 useless[24];

  u8 useless2[8];

  s16 data;
} wav;

typedef struct {
  u32 samples;
  s16 *memory;
} audio;

typedef struct {
  file source;
  u32 vertexCount;
  u32 triangleCount;
  u32 *index;
  r32 *vertex;
  r32 *texture;
  r32 *normal;
} sobj;

#pragma pack(push, 1)
typedef struct {
  u32 triangleCount;
  u32 vertexCount;
} sobjheader;
#pragma pack(pop)

typedef struct {
  sobj model;
  u32 vao;
  u32 ebo;
  u32 vbo0, vbo1, vbo2;
} smodel;
