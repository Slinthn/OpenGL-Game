#include "win32_slingame.h"

typedef struct {
  LPDIRECTSOUNDBUFFER secondaryBuffer;
  u32 bufferSize;
  u32 currentCursor;
} win32_audio;

static win32_audio winaudio;
static b32 running;

file readFile(char *filename) {
  file result = {0};
  HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return result;

  result.size = GetFileSize(file, 0);
  
  result.memory = VirtualAlloc(0, result.size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE); 
  ReadFile(file, result.memory, result.size, 0, 0);

  CloseHandle(file);
  return result;
}

void closeFile(file f) {
  VirtualFree(f.memory, 0, MEM_RELEASE);
}

void writeFile(char *filename, char *buffer, u32 size) {
  HANDLE file = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return;

  DWORD written;
  WriteFile(file, buffer, size, &written, 0);
  CloseHandle(file);
}

#include "slingame.c"

r32 win32_processStick(s16 value) {
  if (value < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && value > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    return 0;

  return value / 32767.0f;
}

HDC win32_setupOpenGL(HWND wnd) {
  HDC dc = GetDC(wnd);
  int pixelFormat;
  PIXELFORMATDESCRIPTOR pd = {0};
  pd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pd.nVersion = 1;
  pd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  pd.iPixelType = PFD_TYPE_RGBA;
  pd.cColorBits = 24;
  pd.cAlphaBits = 8;
  pd.cDepthBits = 8;
  pd.iLayerType = PFD_MAIN_PLANE;
  
  pixelFormat = ChoosePixelFormat(dc, &pd);
  SetPixelFormat(dc, pixelFormat, &pd);
  
  HGLRC glrc = wglCreateContext(dc);
  wglMakeCurrent(dc, glrc);  

#pragma warning(disable:4113; disable:4133)
  glCreateShader = wglGetProcAddress("glCreateShader");
  glCreateProgram = wglGetProcAddress("glCreateProgram");
  glShaderSource = wglGetProcAddress("glShaderSource");
  glCompileShader = wglGetProcAddress("glCompileShader");
  glAttachShader = wglGetProcAddress("glAttachShader");
  glLinkProgram = wglGetProcAddress("glLinkProgram");
  glUseProgram = wglGetProcAddress("glUseProgram");
  glGetUniformLocation = wglGetProcAddress("glGetUniformLocation");
  glUniform1i = wglGetProcAddress("glUniform1i");
  glUniform3f = wglGetProcAddress("glUniform3f");
  glUniformMatrix4fv = wglGetProcAddress("glUniformMatrix4fv");
  
  glGenVertexArrays = wglGetProcAddress("glGenVertexArrays");
  glBindVertexArray = wglGetProcAddress("glBindVertexArray");
  glGenBuffers = wglGetProcAddress("glGenBuffers");
  glBindBuffer = wglGetProcAddress("glBindBuffer");
  glBufferData = wglGetProcAddress("glBufferData");
  glVertexAttribPointer = wglGetProcAddress("glVertexAttribPointer");
  glEnableVertexAttribArray = wglGetProcAddress("glEnableVertexAttribArray");
  glDisableVertexAttribArray = wglGetProcAddress("glDisableVertexAttribArray");
  glGetShaderInfoLog = wglGetProcAddress("glGetShaderInfoLog");

  glActiveTexture = wglGetProcAddress("glActiveTexture");
  glGenerateMipmap = wglGetProcAddress("glGenerateMipmap");

  glGenFramebuffers = wglGetProcAddress("glGenFramebuffers");
  glBindFramebuffer = wglGetProcAddress("glBindFramebuffer");
  glFramebufferTexture = wglGetProcAddress("glFramebufferTexture");
  glCheckFramebufferStatus = wglGetProcAddress("glCheckFramebufferStatus");
#pragma warning(default:4113; default:4133)

  return dc;
}

win32_audio win32_setupDirectSound8(HWND wnd, u32 samplesPerSec, s16 channels, s16 bytesPerSample) {
  win32_audio result = {0};
  IDirectSound8 *ds;
  DirectSoundCreate8(0, &ds, 0);
  ds->lpVtbl->SetCooperativeLevel(ds, wnd, DSSCL_PRIORITY);

  DSBUFFERDESC bd = {0};
  bd.dwSize = sizeof(DSBUFFERDESC);
  bd.dwFlags = DSBCAPS_PRIMARYBUFFER;

  LPDIRECTSOUNDBUFFER primaryBuffer;
  ds->lpVtbl->CreateSoundBuffer(ds, &bd, &primaryBuffer, 0);

  WAVEFORMATEX fmt = {0};
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = channels;
  fmt.nSamplesPerSec = samplesPerSec;
  fmt.wBitsPerSample = bytesPerSample*8;
  fmt.nBlockAlign = fmt.nChannels*(fmt.wBitsPerSample / 8);
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec*fmt.nBlockAlign;
  primaryBuffer->lpVtbl->SetFormat(primaryBuffer, &fmt);

  bd.dwFlags = 0;
  bd.dwBufferBytes = fmt.nSamplesPerSec*fmt.nBlockAlign;
  bd.lpwfxFormat = &fmt;

  ds->lpVtbl->CreateSoundBuffer(ds, &bd, &result.secondaryBuffer, 0);
  result.bufferSize = samplesPerSec*channels*bytesPerSample;
  return result;
}

s64 win32_getCounter(void) {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result.QuadPart;
}

s64 win32_getFrequency(void) {
  LARGE_INTEGER result;
  QueryPerformanceFrequency(&result);
  return result.QuadPart;
}

r32 win32_getTimeDifference(s64 time0, s64 time1, s64 frequency) {
  return (time1 - time0) / (r32)frequency;
}

void win32_writeAudioToBuffer(win32_audio audio, u32 cursor, u32 bytesToWrite, s16 *memory) {
  void *ap0, *ap1;
  DWORD as0, as1;

  audio.secondaryBuffer->lpVtbl->Lock(audio.secondaryBuffer, cursor, bytesToWrite, &ap0, &as0, &ap1, &as1, 0);

#define audioLoop(size, pointer)                \
  region = (s16 *)pointer;                      \
  for (DWORD i = 0; i < size / 4; i++) {        \
    *region++ = *memory++;                      \
    *region++ = *memory++;                      \
  }                                             \
  
  s16 *region = 0;
  audioLoop(as0, ap0);
  audioLoop(as1, ap1);
  
  audio.secondaryBuffer->lpVtbl->Unlock(audio.secondaryBuffer, ap0, as0, ap1, as1);
}

void keyboardToInput(gameinput *input, u32 key, b32 down) {
  switch (key) {
  case 'W':
  case VK_UP: {
    input->lStickY = down ? 1.0f : 0;
  } break;

  case 'A':
  case VK_LEFT: {
    input->lStickX = down ? -1.0f : 0;
  } break;

  case 'S':
  case VK_DOWN: {
    input->lStickY = down ? -1.0f : 0;
  } break;

  case 'D':
  case VK_RIGHT: {
    input->lStickX = down ? 1.0f : 0;
  } break;

  case ' ': {
    input->btnDown = down ? 1 : 0;
  } break;
  }
}

LRESULT win32_windowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT res = 0;
  switch (msg) {
  case WM_DESTROY:
  case WM_QUIT: {
    running = 0;
  } break;
    
  case WM_SIZE: {
    viewportWidth = LOWORD(lParam);
    viewportHeight = HIWORD(lParam);
    glViewport(0, 0, viewportWidth, viewportHeight);
  }; // NOTE(slin): on purpose no break

  case WM_MOUSEMOVE: {
    RECT client, window;
    GetWindowRect(wnd, &window);
    GetClientRect(wnd, &client);

    POINT point = {0};
    point.x = client.right / 2;
    point.y = client.bottom / 2;
    ClientToScreen(wnd, &point);
    
    SetCursorPos(point.x, point.y);
    ClipCursor(&window);
  } break;
    
  case WM_ACTIVATEAPP: {
    if (winaudio.secondaryBuffer == 0)
      break;
    
    if (wParam)
      winaudio.secondaryBuffer->lpVtbl->Play(winaudio.secondaryBuffer, 0, 0, DSBPLAY_LOOPING);
    else
      winaudio.secondaryBuffer->lpVtbl->Stop(winaudio.secondaryBuffer);
  } break;
    
  default: {
    res = DefWindowProc(wnd, msg, wParam, lParam);
  } break;
  }

  return res;
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int show) {
  timeBeginPeriod(1);

  WNDCLASSA wc = {0};
  wc.lpfnWndProc = win32_windowProc;
  wc.hInstance = instance;
  wc.lpszClassName = "SlingameOpenGLWindow";
  if (!RegisterClassA(&wc))
    return -1;
  
  HWND wnd = CreateWindowA(wc.lpszClassName, "Slingame OpenGL", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

  HDC dc = win32_setupOpenGL(wnd);
  winaudio = win32_setupDirectSound8(wnd, 48000, 2, 2);
  winaudio.secondaryBuffer->lpVtbl->Play(winaudio.secondaryBuffer, 0, 0, DSBPLAY_LOOPING);
  s16 audioMemory[2*2*48000];
  
  gamememory memory = {0};
  memory.memory = VirtualAlloc(0, 1*1024*1024, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

  gameinput input = {0};  
  gameaudio audioOut = {0};
  
#ifdef SLINGAME_DEBUG
  r32 targetFrequency = 1.0f / 60.0f;
  s64 frequency = win32_getFrequency();
  s64 prevCounter = win32_getCounter();
#endif
  
  running = 1;
  while (running) {
    input.rStickX = 0;
    input.rStickY = 0;
    
    MSG msg;
    while (PeekMessage(&msg, wnd, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);

      switch (msg.message) {
      case WM_KEYDOWN: {
        keyboardToInput(&input, (u32)msg.wParam, 1);
      } break;
      case WM_KEYUP: {
        keyboardToInput(&input, (u32)msg.wParam, 0);
      } break;

      case WM_MOUSEMOVE: {
        RECT window;
        GetClientRect(wnd, &window);
        s32 originX = window.right / 2;
        s32 originY = window.bottom / 2;
        
        s32 dx = GET_X_LPARAM(msg.lParam) - originX;
        s32 dy = GET_Y_LPARAM(msg.lParam) - originY;

        r32 speed = 0.04f;
        input.rStickX += dx*speed;
        input.rStickY += dy*speed;

        ShowCursor(0);
      } break;
      }

      DispatchMessage(&msg);
    }

    //input.rStickY = 0;
    
    // NOTE(slin): Input cod
    XINPUT_STATE state;
    XINPUT_GAMEPAD *gp;
    XInputGetState(0, &state);
    gp = &state.Gamepad;
    
#if 0 // TODO(slin): Temporarily disabled so i can work on keyboard and mouse
    input.lStickX = win32_processStick(gp->sThumbLX);
    input.lStickY = win32_processStick(gp->sThumbLY);
    input.rStickX = win32_processStick(gp->sThumbRX);
    input.rStickY = win32_processStick(gp->sThumbRY);
    input.btnDown = gp->wButtons & XINPUT_GAMEPAD_A;
#endif
    
    // NOTE(slin): Audio code
    u32 toWrite;
    DWORD playCursor, writeCursor;
    winaudio.secondaryBuffer->lpVtbl->GetCurrentPosition(winaudio.secondaryBuffer, &playCursor, &writeCursor);
      
    u32 delay = 5000; // TODO(slin): Should this be increased? If I set it to 0 I think it's actually
    // writing behind the playcursor.
    u32 shouldBeAt = (writeCursor + delay) % winaudio.bufferSize;
    
    if (winaudio.currentCursor < shouldBeAt)
      toWrite = shouldBeAt - winaudio.currentCursor;
    else if (winaudio.currentCursor > shouldBeAt)
      toWrite = (shouldBeAt) + (winaudio.bufferSize - winaudio.currentCursor);
    else
      toWrite = 0;

    audioOut.samples = toWrite / 4;
    audioOut.memory = audioMemory;

    // TODO(slin): If update() takes too long, audio cursor will have moved and audio will be fucked.
    // Increase delay or query new cursor after and just write from there? 2nd a bit unreliable but
    // may work.

    // NOTE(slin): Game updating code
    update(&memory, &input, &audioOut);
    win32_writeAudioToBuffer(winaudio, winaudio.currentCursor, toWrite, audioMemory);
    wglSwapLayerBuffers(dc, WGL_SWAP_MAIN_PLANE);

    winaudio.currentCursor += toWrite;
    winaudio.currentCursor %= winaudio.bufferSize;

    // NOTE(slin): Debug game analysing code
#ifdef SLINGAME_DEBUG
    s64 nowCounter = win32_getCounter();
      
    // TODO(slin): I don't think this is stopping the program for *exactly* 16ms
    // NOTE it probably is
    r32 work = win32_getTimeDifference(prevCounter, nowCounter, frequency);
    u32 ms = (u32)(work*1000.0f);
    while (work < targetFrequency) {
      Sleep((s32)(targetFrequency - work));
      nowCounter = win32_getCounter();
      work = win32_getTimeDifference(prevCounter, nowCounter, frequency);
    }
    
    s8 msgBuffer[64];
    wsprintfA(msgBuffer, "work: %i, passed %i\n", (s32)ms, (s32)(work*1000.0f));
    OutputDebugString(msgBuffer);
        
    prevCounter = nowCounter;
#endif
  }  
  return 0;
}
