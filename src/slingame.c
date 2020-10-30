#include "slingame.h"
#include <math.h>

#define PI_32 3.14159265358979323846f

static u32 viewportWidth, viewportHeight;

typedef struct {
  u32 program;
  smodel model0, model1, model2;
  u32 texture;
  matrix4 perspective;
  wav *wavData;

  u32 depthFBO;
  u32 depthTexture;
  u32 shadowProgram;

  u32 shadowWidth, shadowHeight;
} gamestate;

matrix4 perspectiveMatrix(r32 aspect, r32 fov, r32 farz, r32 nearz) {
  matrix4 result = {0};
  result.m[0] = 1.0f / (aspect*tanf(fov / 2.0f));
  result.m[5] = 1.0f / tanf(fov / 2.0f);
  result.m[10] = -(farz + nearz) / (farz - nearz);
  result.m[11] = -1;
  result.m[14] = -(2*farz*nearz) / (farz - nearz);
  return result;
}

matrix4 orthographicMatrix(r32 left, r32 right, r32 top, r32 bottom, r32 farz, r32 nearz) {
  matrix4 result = {0};
  result.m[0] = 2.0f / (right - left);
  result.m[5] = 2.0f / (top - bottom);
  result.m[10] = -2.0f / (farz - nearz);
  result.m[12] = -(right + left) / (right - left);
  result.m[13] = -(top + bottom) / (top - bottom);
  result.m[14] = -(farz + nearz) / (farz - nearz);
  result.m[15] = 1.0f;
  
  return result;
}

matrix4 transformMatrix(r32 tx, r32 ty, r32 tz, r32 rx, r32 ry, r32 rz, r32 sx, r32 sy, r32 sz) {
  matrix4 result = {0};
  result.m[0] = 1;
  result.m[5] = 1;
  result.m[10] = 1;
  result.m[15] = 1;

  rx *= PI_32 / 180.0f;
  ry *= PI_32 / 180.0f;
  rz *= PI_32 / 180.0f;
  
  result.m[0] = cosf(rz)*cosf(ry);
  result.m[4] = cosf(rz)*sinf(ry)*sinf(rx) - sinf(rz)*cosf(rx);
  result.m[8] = cosf(rz)*sinf(ry)*cosf(rx) + sinf(rz)*sinf(rx);
  result.m[1] = sinf(rz)*cosf(ry);
  result.m[5] = sinf(rz)*sinf(ry)*sinf(rx) + cosf(rz)*cosf(rx);
  result.m[9] = sinf(rz)*sinf(ry)*cosf(rx) - cosf(rz)*sinf(rx);
  result.m[2] = -sinf(ry);
  result.m[6] = cosf(ry)*sinf(rx);
  result.m[10] = cosf(ry)*cosf(rx);

  result.m[12] = tx;
  result.m[13] = ty;
  result.m[14] = tz;

  // TODO(slin): does this scale work
  result.m[0] *= sx;
  result.m[1] *= sx;
  result.m[2] *= sx;
  result.m[3] *= sx;

  result.m[4] *= sy;
  result.m[5] *= sy;
  result.m[6] *= sy;
  result.m[7] *= sy;

  result.m[8] *= sz;
  result.m[9] *= sz;
  result.m[10] *= sz;
  result.m[11] *= sz;
  return result;
}

u32 createShader(char *source, s32 type) {
  u32 shader = glCreateShader(type);

  glShaderSource(shader, 1, (GLchar **)&source, 0);
  glCompileShader(shader);

  s8 msgBuffer[512];
  glGetShaderInfoLog(shader, 512, 0, msgBuffer);
  assert(*msgBuffer == 0);
  return shader;
}

u32 createProgram(char *vSource, char *fSource) {
  file vFile = readFile(vSource);
  u32 vShader = createShader((char *)vFile.memory, GL_VERTEX_SHADER);
  file fFile = readFile(fSource);
  u32 fShader = createShader((char *)fFile.memory, GL_FRAGMENT_SHADER);

  u32 program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);

  closeFile(vFile);
  closeFile(fFile);
  return program;
}

bitmap loadBitmap(char *source) {
  bitmap result = {0};
  result.source = readFile(source);
  result.header = result.source.memory;
  result.data = (u32 *)((u8 *)result.source.memory + result.header->offset);
  return result;
}

sobj loadSObj(char *source) {
  sobj result = {0};

  result.source = readFile(source);
  u8 *memory = (u8 *)result.source.memory;
  sobjheader *header = (sobjheader *)memory;

  result.triangleCount = header->triangleCount;
  result.vertexCount = header->vertexCount;
  result.index = (u32 *)((u8 *)memory + sizeof(sobjheader));
  result.vertex =  (r32 *)((u8 *)memory + sizeof(sobjheader) + header->triangleCount*3*sizeof(u32));
  result.texture = (r32 *)((u8 *)memory + sizeof(sobjheader) + header->triangleCount*3*sizeof(u32) + header->vertexCount*3*sizeof(r32));
  result.normal =  (r32 *)((u8 *)memory + sizeof(sobjheader) + header->triangleCount*3*sizeof(u32) + header->vertexCount*5*sizeof(r32));
  
  return result;
}

smodel loadModel(char *sobjfile) {
  smodel result = {0};
  result.model = loadSObj(sobjfile);

  glGenVertexArrays(1, &result.vao);
  glBindVertexArray(result.vao);

  glGenBuffers(1, &result.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.model.triangleCount*3*sizeof(u32), result.model.index, GL_STATIC_DRAW);
    
  glGenBuffers(1, &result.vbo0);
  glBindBuffer(GL_ARRAY_BUFFER, result.vbo0);
  glBufferData(GL_ARRAY_BUFFER, result.model.vertexCount*3*sizeof(r32), result.model.vertex, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(0);

  glGenBuffers(1, &result.vbo1);
  glBindBuffer(GL_ARRAY_BUFFER, result.vbo1);
  glBufferData(GL_ARRAY_BUFFER, result.model.vertexCount*2*sizeof(r32), result.model.texture, GL_STATIC_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(1);

  glGenBuffers(1, &result.vbo2);
  glBindBuffer(GL_ARRAY_BUFFER, result.vbo2);
  glBufferData(GL_ARRAY_BUFFER, result.model.vertexCount*3*sizeof(r32), result.model.normal, GL_STATIC_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(2);
  
  glBindVertexArray(0);

  closeFile(result.model.source);
  return result;
}

void initGame(gamestate *game) {
  game->program = createProgram("data\\3d.vs", "data\\3d.fs");
  game->shadowProgram = createProgram("data\\shadow.vs", "data\\shadow.fs");
  game->model0 = loadModel("data\\model.sobj");
  game->model1 = loadModel("data\\sphere.sobj");
  game->model2 = loadModel("data\\plane.sobj");

  glUseProgram(game->program);
  glUniform1i(glGetUniformLocation(game->program, "sampler0"), 0);
  glUniform1i(glGetUniformLocation(game->program, "sampler1"), 1);
  glUseProgram(0);
      
  bitmap bmp = loadBitmap("data\\bark_0021.bmp");

  glGenTextures(1, &game->texture);
  glBindTexture(GL_TEXTURE_2D, game->texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp.header->width, bmp.header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp.data);
  glGenerateMipmap(GL_TEXTURE_2D);

  closeFile(bmp.source);
  
  file waveFile = readFile("data\\music.wav");
  game->wavData = (wav *)waveFile.memory;
  
  // NOTE(slin): Tmp framebuffer code
  glGenFramebuffers(1, &game->depthFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, game->depthFBO);
  glDrawBuffer(0);
  glReadBuffer(0);
  
  game->shadowWidth = 1024;
  game->shadowHeight = 1024;
  
  glGenTextures(1, &game->depthTexture);
  glBindTexture(GL_TEXTURE_2D, game->depthTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, game->shadowWidth, game->shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);  
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, game->depthTexture, 0);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
}

void renderModel(smodel model) {
  glBindVertexArray(model.vao);
  glDrawElements(GL_TRIANGLES, model.model.triangleCount*3, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void renderScene(gamestate *game, matrix4 camera, matrix4 shadowProjection, matrix4 shadowTransform, r32 sinv, u32 program) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, game->texture);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, game->depthTexture);
  
  glUseProgram(program);
  glUniformMatrix4fv(glGetUniformLocation(program, "shadowProjection"), 1, 0, shadowProjection.m);
  glUniformMatrix4fv(glGetUniformLocation(program, "shadowTransform"), 1, 0, shadowTransform.m);

  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, 0, game->perspective.m);
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, 0, camera.m);

  glUniform3f(glGetUniformLocation(program, "diffuse"), 0.70f , 0.96f, 1.0f);
  glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, 0, transformMatrix(0, sinf(sinv)*3, -3, sinf(sinv)*360.0f, 0, 0, 1, 1, 1).m);
  renderModel(game->model0);
  
  glUniform3f(glGetUniformLocation(program, "diffuse"), 0.94f, 0.44f, 0.36f);
  glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, 0, transformMatrix(sinf(sinv)*5, -2, -10, 0, 0, 0, 1, 1, 1).m);
  renderModel(game->model1);

  glUniform3f(glGetUniformLocation(program, "diffuse"), 1.0f, 0.94f, 0.08f);
  glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, 0, transformMatrix(0, -1, -15, 270.0f, 0, 0, 20, 20, 20).m);
  renderModel(game->model2);
  
  glUseProgram(0);
}

void update(gamememory *memory, gameinput *input, audio *audioOut) {
  gamestate *game = (gamestate *)memory->memory;

  if (!memory->initialised) {
    memory->initialised = 1;
    initGame(game);
  }
  
  static r32 sinv = 0;
  sinv += 0.01f;

  static r32 px, py, pz = 3;
  static r32 prx, pry;

  r32 cx0 = input->lStickX;
  r32 cy0 = input->lStickY;
  r32 cx1 = input->rStickX;
  r32 cy1 = input->rStickY;
  

  prx += -cy1*5;
  pry += cx1*5;

  pz += cy0*cosf(-pry*PI_32 / 180.0f);
  px += cy0*sinf(-pry*PI_32 / 180.0f);
  
  /*
  pz += cy0*cosf(pry*PI_32 / 180.0f);
  py += cy0*sinf(pry*PI_32 / 180.0f);
  
  px += cy0*cosf(pry*PI_32 / 180.0f);
  py += cx0*sinf(pry*PI_32 / 180.0f);
  */  
  
  game->perspective = perspectiveMatrix(viewportWidth / (r32)viewportHeight, 90.0f, 1000.0f, 1.0f);
  matrix4 camera = transformMatrix(-px, -py, -pz, -prx, -pry, 0, 1, 1, 1);
  matrix4 shadowTransform = transformMatrix(0, 0, -2, 0, 0, 0, 1, 1, 1);
  matrix4 shadowProjection = orthographicMatrix(-21, 21, 21, -21, 50.0f, -10.0f);
  
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  
  // NOTE(slin): Depth buffer render
  glBindFramebuffer(GL_FRAMEBUFFER, game->depthFBO);

  glClear(GL_DEPTH_BUFFER_BIT);
  
  glViewport(0, 0, game->shadowWidth, game->shadowHeight);
  renderScene(game, camera, shadowProjection, shadowTransform, sinv, game->shadowProgram);

#ifdef SLINGAME_SHADOWDEBUG
  static u32 tmpDepth[1024*1024];
  glReadPixels(0, 0, 1024, 1024, GL_DEPTH_COMPONENT, GL_FLOAT, tmpDepth);
#endif
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // NOTE(slin): Actual rendering
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
  glViewport(0, 0, viewportWidth, viewportHeight);
  renderScene(game, camera, shadowProjection, shadowTransform, sinv, game->program);

#ifdef SLINGAME_SHADOWDEBUG
  static GLuint tex = 0;
  if (tex > 0)
    glDeleteTextures(1, &tex);
  glGenTextures(1, &tex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1024, 1024, 0, GL_LUMINANCE, GL_FLOAT, tmpDepth);
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glColor3f(1.0f, 1.0f, 1.0f);
  
  glTexCoord2i(0, 1);
  glVertex2i(-1, 0);
  glTexCoord2i(0, 0);
  glVertex2i(-1, -1);
  glTexCoord2i(1, 0);
  glVertex2i(0, -1);
  glTexCoord2i(1, 1);
  glVertex2i(0, 0);

  glEnd();
  glDisable(GL_TEXTURE_2D);
#endif

  // NOTE(slin): Audio
  // TODO(slin): Some temp code here
  static s16 *startMemory = 0;
  if (startMemory == 0)
    startMemory = &game->wavData->data;
  
  s16 *region = audioOut->memory;
  r32 tmpMemoryLoc = 0;
  r32 inc = 1 + input->lStickX;
  
  for (DWORD i = 0; i < audioOut->samples; i++) {
    s16 value = input->btnDown ? *(startMemory + (s16)tmpMemoryLoc) : 0;
    *region++ = value;
    tmpMemoryLoc += inc;
    value = input->btnDown ? *(startMemory + (s16)tmpMemoryLoc) : 0;
    *region++ = value;
    tmpMemoryLoc += inc;
  }
  startMemory += (s16)tmpMemoryLoc;  
}
