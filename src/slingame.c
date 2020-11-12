#include "slingame.h"
#include <math.h>
#include "math.c"

static u32 viewportWidth, viewportHeight;

#define ENTITY_PLAYER 0x1
#define ENTITY_THING 0x2
#define ENTITY_THING2 0x3

typedef struct {
  u32 type;
  r32 tx, ty, tz;
  r32 vx, vy, vz;
  r32 rx, ry, rz;
} entity;

typedef struct {
  u32 program, shadowProgram;
  smodel model0, model1, model2;
  u32 texture;
  matrix4 perspective;
  wav *wavData;

  u32 depthFBO, depthTexture;
  u32 shadowWidth, shadowHeight;

  u32 entityCount;
  entity entities[1000];
  entity *player;

  u32 shadowProjection0, shadowTransform0, transform0, projection0, view0;
  u32 shadowProjection1, shadowTransform1, transform1;
} gamestate;

entity *createEntity(gamestate *game, u32 type) {
  entity ety = {0};
  ety.type = type;

  entity *result = &game->entities[game->entityCount++];
  *result = ety;

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

u32 createBitmapTexture(char *source) {
  u32 result;
  bitmap bmp = loadBitmap(source);
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp.header->width, bmp.header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp.data);
  glGenerateMipmap(GL_TEXTURE_2D);

  closeFile(bmp.source);
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
  game->model0 = loadModel("data\\sus.sobj");
  game->model1 = loadModel("data\\sphere.sobj");
  game->model2 = loadModel("data\\plane.sobj");

  game->player = createEntity(game, ENTITY_PLAYER);
  entity *ety0 = createEntity(game, ENTITY_THING);  
  entity *ety1 = createEntity(game, ENTITY_THING2);
  ety1->tz = -3;
  
  glUseProgram(game->program);
  glUniform1i(glGetUniformLocation(game->program, "sampler0"), 0);
  glUniform1i(glGetUniformLocation(game->program, "sampler1"), 1);
  glUseProgram(0);
  
  //bitmap bmp = loadBitmap("data\\bark_0021.bmp");
  
  file waveFile = readFile("data\\music.wav");
  game->wavData = (wav *)waveFile.memory;
  
  glGenFramebuffers(1, &game->depthFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, game->depthFBO);
  glDrawBuffer(0);
  glReadBuffer(0);
  
  game->shadowWidth = 512;
  game->shadowHeight = 512;
  
  glGenTextures(1, &game->depthTexture);
  glBindTexture(GL_TEXTURE_2D, game->depthTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, game->shadowWidth, game->shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);  
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, game->depthTexture, 0);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glUseProgram(game->program);
  game->shadowProjection0 = glGetUniformLocation(game->program, "shadowProjection");
  game->shadowTransform0 = glGetUniformLocation(game->program, "shadowTransform");
  game->transform0 = glGetUniformLocation(game->program, "transform");
  game->projection0 = glGetUniformLocation(game->program, "projection");
  game->view0 = glGetUniformLocation(game->program, "view");

  glUseProgram(game->shadowProgram);
  game->shadowProjection1 = glGetUniformLocation(game->shadowProgram, "shadowProjection");
  game->shadowTransform1 = glGetUniformLocation(game->shadowProgram, "shadowTransform");
  game->transform1 = glGetUniformLocation(game->shadowProgram, "transform");
  glUseProgram(0);
}

void renderModel(smodel model) {
  glBindVertexArray(model.vao);
  glDrawElements(GL_TRIANGLES, model.model.triangleCount*3, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void renderScene(gamestate *game, u32 transformLocation) {
  //glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_2D, game->texture);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, game->depthTexture);
  
  for (u32 i = 0; i < game->entityCount; i++) {
    entity *ety = &game->entities[i];
    if (ety->type == 0)
      continue;

    glUniformMatrix4fv(transformLocation, 1, 0, transformMatrix(ety->tx, ety->ty, ety->tz, ety->rx, ety->ry, ety->rz, 1, 1, 1).m);

    if (ety->type == ENTITY_THING)
      renderModel(game->model1);
    else if (ety->type == ENTITY_THING2)
      renderModel(game->model0);
  }
 
  glUseProgram(0);
}

void debug_renderShadowDepthBuffer(u32 *tmpDepth) {
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
}

void update(gamememory *memory, gameinput *input, gameaudio *audio) {
  gamestate *game = (gamestate *)memory->memory;
  if (!memory->initialised) {
    memory->initialised = 1;
    initGame(game);
  }

  if (input->rShoulder) {
    entity *ety = createEntity(game, ENTITY_THING2);
    ety->vz = -cosf(radians(game->player->rx))*cosf(radians(game->player->ry));
    ety->vx = -cosf(radians(game->player->rx))*sinf(radians(game->player->ry));
    ety->vy = sinf(radians(game->player->rx));
    ety->tx = game->player->tx;
    ety->ty = game->player->ty;
    ety->tz = game->player->tz;
  }
  
  r32 cx0 = input->lStickX;
  r32 cy0 = input->lStickY;
  r32 cx1 = input->rStickX;
  r32 cy1 = input->rStickY;

  game->player->ry += -cx1*6;
  game->player->rx += -cy1*6;

  static const r32 ay = -0.05f;
  
  if (game->player->rx > 90.0f)
    game->player->rx = 90.0f;
  else if (game->player->rx < -90.0f)
    game->player->rx = -90.0f;

  r32 speed = 0.2f;
  r32 dx = (cx0*cosf(radians(game->player->ry)) + cy0*sinf(radians(-game->player->ry)))*speed;
  r32 dz = (cx0*sinf(radians(game->player->ry)) + cy0*cosf(radians(-game->player->ry)))*speed;
  game->player->tx += dx;
  game->player->tz -= dz;

  game->player->vy += ay;

  for (u32 i = 0; i < game->entityCount; i++) {
    entity *ety = &game->entities[i];
    ety->tx += ety->vx;
    ety->ty += ety->vy;
    ety->tz += ety->vz;
  }
  
  if (game->player->ty < 3)
    game->player->ty = 3;
  
  if (input->btnDown && game->player->ty == 3)
    game->player->vy = 1.0f;
  
  game->perspective = perspectiveMatrix(viewportWidth / (r32)viewportHeight, radians(90.0f), 1000.0f, 0.001f);
  matrix4 camera = transformMatrix(game->player->tx, game->player->ty, game->player->tz, game->player->rx, game->player->ry, 0, 1, 1, 1);
  matrix4 shadowTransform = transformMatrix(0, 0, -2, 0, 0, 0, 1, 1, 1);
  matrix4 shadowProjection = orthographicMatrix(-21, 21, 21, -21, 50.0f, -10.0f);
  
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

  glUseProgram(game->shadowProgram);
  glUniformMatrix4fv(game->shadowProjection1, 1, 0, shadowProjection.m);
  glUniformMatrix4fv(game->shadowTransform1, 1, 0, shadowTransform.m);

  // NOTE(slin): Depth buffer render
  glBindFramebuffer(GL_FRAMEBUFFER, game->depthFBO);

  glClear(GL_DEPTH_BUFFER_BIT);
  
  glViewport(0, 0, game->shadowWidth, game->shadowHeight);
  renderScene(game, game->transform1);

#ifdef SLINGAME_SHADOWDEBUG
  static u32 tmpDepth[1024*1024];
  glReadPixels(0, 0, 1024, 1024, GL_DEPTH_COMPONENT, GL_FLOAT, tmpDepth);
#endif
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glUseProgram(game->program);
  glUniformMatrix4fv(game->shadowProjection0, 1, 0, shadowProjection.m);
  glUniformMatrix4fv(game->shadowTransform0, 1, 0, shadowTransform.m);
  glUniformMatrix4fv(game->projection0, 1, 0, game->perspective.m);
  glUniformMatrix4fv(game->view0, 1, 0, camera.m);
  
  // NOTE(slin): Actual rendering
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
  glViewport(0, 0, viewportWidth, viewportHeight);
  renderScene(game, game->transform0);

#ifdef SLINGAME_SHADOWDEBUG
  debug_renderShadowDepthBuffer(tmpDepth);
#endif

  // NOTE(slin): Audio
  // TODO(slin): Some temp code here
#if 0
  static s16 *startMemory = 0;
  if (startMemory == 0)
    startMemory = &game->wavData->data;
  
  s16 *region = audio->memory;
  r32 tmpMemoryLoc = 0;
  r32 inc = 1 + input->lStickX;
  
  for (DWORD i = 0; i < audio->samples; i++) {
    s16 value = input->btnDown ? *(startMemory + (s16)tmpMemoryLoc) : 0;
    *region++ = value;
    tmpMemoryLoc += inc;
    value = input->btnDown ? *(startMemory + (s16)tmpMemoryLoc) : 0;
    *region++ = value;
    tmpMemoryLoc += inc;
  }
  startMemory += (s16)tmpMemoryLoc;
#endif
}
