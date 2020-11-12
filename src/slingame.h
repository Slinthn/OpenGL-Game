// NOTE(slin): File structures/headers
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

typedef struct {
  u32 triangleCount;
  u32 vertexCount;
} sobjheader;

typedef struct {
  s8 chunkId[4];
  u32 chunkSize;
  s8 format[4];
  u8 unused0[24];
  u8 unused1[8];
  s16 data;
} wav;
#pragma pack(pop)

// NOTE(slin): Structs passed into update()
typedef struct {
  u32 samples;
  s16 *memory;
} gameaudio;

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
  b32 rShoulder;
} gameinput;

// NOTE(slin): General structures
typedef struct {
  file source;
  bitmapheader *header;
  u32 *data;
} bitmap;

typedef struct {
  file source;
  u32 vertexCount;
  u32 triangleCount;
  u32 *index;
  r32 *vertex;
  r32 *texture;
  r32 *normal;
} sobj;

typedef struct {
  sobj model;
  u32 vao;
  u32 ebo;
  u32 vbo0, vbo1, vbo2;
} smodel;
