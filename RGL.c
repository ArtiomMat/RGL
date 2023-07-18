#include <windows.h>

#define RGL_SRC
#include "RGL.h"
#include "RGL_loader.h"

#include <GL/wgl.h>
#include <GL/wglext.h>

#include <stdio.h>
#include <math.h>

#define CLASSNAME "RGLWND"

static HINSTANCE hInstance;

static HWND hWnd = NULL;
static HDC hDC = NULL;

static HGLRC hGLRC = NULL;

typedef BOOL (*WGLSWAPINTERVAL) (int interval);

static WGLSWAPINTERVAL wglSwapIntervalEXT = NULL;

#define DMAXTABLESIZE 16
// Cached dmax-es
// A constant sent to vertex shader.
// If we look at the perspective as a fustrum, this is the size of the smaller part.
// The reason it is RGL_d_max is because there is a d parameter that is the signed distance from the center of the screen.
// We then normalize d using RGL_d_max*2
static float dmaxtable[DMAXTABLESIZE];

static void vecsub(RGL_VEC a, RGL_VEC b, RGL_VEC out) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

static void zerovec(RGL_VEC a) {
  a[0] = 0;
  a[1] = 0;
  a[2] = 0;
}

// Pass the actual opengl enum to type, like GL_VERTEX_SHADER.
RGL_SHADER RGL_loadshader(const char* fp, UINT type) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load shader '%s'.\n", fp);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char data[size+1];
  fread(data, size, 1, f);
  data[size] = '\0';

  fclose(f);

  switch (type) {
    case RGL_FRAGMENTSHADER:
    type = GL_FRAGMENT_SHADER;
    break;

    default:
    type = GL_VERTEX_SHADER;
    break;
  }

  UINT shader = rglCreateShader(type);

  if (!shader) {
    printf("RGL: Shader '%s' creation failed.\n", fp);
    return 0;
  }

  const char* ptrdata = data; // Dark magic because can't pass array pointers

  rglShaderSource(shader, 1, &ptrdata, NULL);
  rglCompileShader(shader);

  int success;
  rglGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info[512];
    info[0] = 'c';
    info[1] = 0;
    rglGetShaderInfoLog(shader, sizeof(info), NULL, info);
    printf("RGL: Compilation of shader '%s' unsuccessful.\nINFO: %s\n", fp, info);
    RGL_freeshader(shader);
    return 0;
  }

  return shader;
}

void RGL_freeshader(RGL_SHADER shader) {
  rglDeleteShader(shader);
}

static void uniformpalette(RGL_PROGRAM program) {
  INT i;
  i = rglGetUniformLocation(program, "RGL_palette");
  rglUniform4fv(i, 256, RGL_colors);
}

static void uniformbody(RGL_PROGRAM program, RGL_BODY body) {
  INT i;
  i = rglGetUniformLocation(program, "RGL_offset");
  rglUniform3f(i, body->offset[0], body->offset[1], body->offset[2]);
  i = rglGetUniformLocation(program, "RGL_angles");
  rglUniform3f(i, body->angles[0], body->angles[1], body->angles[2]);
}

RGL_PROGRAM RGL_initprogram(RGL_SHADER vertshader, RGL_SHADER fragshader) {
  UINT program = rglCreateProgram();
  if (!program) {
    printf("RGL: Creation of program failed.\n");
    return 0;
  }

  if (vertshader)
    rglAttachShader(program, vertshader);
  if (fragshader)
    rglAttachShader(program, fragshader);
  rglLinkProgram(program);

  int success;
  rglGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    printf("RGL: Program linking failed.\n");
    return 0;
  }

  return program;
}

RGL_PROGRAM RGL_loadprogram(const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load program '%s'.\n", fp);
    return 0;
  }
  
  GLsizei len;
  GLenum format;

  fread(&format, sizeof(GLenum), 1, f);
  fread(&len, sizeof(GLsizei), 1, f);

  char data[len];
  fread(data, len, 1, f);

  fclose(f);

  UINT program = rglCreateProgram();
  rglProgramBinary(program, format, data, len);

  rglValidateProgram(program);
  GLint status;
  rglGetProgramiv(program, GL_VALIDATE_STATUS, &status);
  if (!status) {
    printf("RGL: Program '%s' is invalid.\n", fp);
    RGL_freeprogram(program);
    return 0;
  }

  return program;
}

void RGL_saveprogram(RGL_PROGRAM program, const char* fp) {
  GLsizei len;
  GLenum format;
  rglGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);

  char data[len];

  rglGetProgramBinary(program, len, NULL, &format, data);

  FILE* f = fopen(fp, "wb");
  fwrite(&format, sizeof(GLenum), 1, f);
  fwrite(&len, sizeof(GLsizei), 1, f);
  fwrite(data, len, 1, f);

  fclose(f);
}

void RGL_freeprogram(RGL_PROGRAM program) {
  rglDeleteProgram(program);
}

static void calcd_max(RGL_EYE eye) {
  eye->info.d_max = tanf(eye->fov/2) * eye->info.p_near;
}

// Assumes program is currently in use
static void useprogram(RGL_EYE eye) {
  // Bind and copy ubo data.
  calcd_max(eye);
  rglUseProgram(eye->program);
  rglBindBuffer(GL_UNIFORM_BUFFER, eye->ubo);
  rglBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(eye->info), &eye->info);
}

RGL_EYE RGL_initeye(RGL_PROGRAM program, float fov) {
  RGL_EYE eyeptr = malloc(sizeof(RGL_EYEDATA));
  eyeptr->program = program;

  zerovec(eyeptr->info.offset);
  // eyeptr->info.offset[2] = -10;
  zerovec(eyeptr->info.angles);
  eyeptr->fov = fov;
  eyeptr->info.p_far = 500.0f;
  eyeptr->info.p_near = 0.01f;
  calcd_max(eyeptr);
  
  // Setup the ubo that holds the eye info, which is in eyeptr->info
  rglUseProgram(eyeptr->program);
  rglGenBuffers(1, &eyeptr->ubo);
  rglBindBuffer(GL_UNIFORM_BUFFER, eyeptr->ubo);
  rglBufferData(GL_UNIFORM_BUFFER, sizeof(eyeptr->info), 0, GL_DYNAMIC_DRAW);
  
  int i = rglGetUniformBlockIndex(program, "RGL_eye");
  rglUniformBlockBinding(program, i, 4);
  rglBindBufferBase(GL_UNIFORM_BUFFER, 4, eyeptr->ubo);


  if (!RGL_usedeye)
    RGL_usedeye = eyeptr;

  return eyeptr;
}
void RGL_freeeye(RGL_EYE eye) {
  if (RGL_usedeye == eye)
    RGL_usedeye = NULL;

  rglDeleteBuffers(1, &eye->ubo);
  free(eye);
}

RGL_MODEL RGL_initmodel(float* vbodata, UINT verticesn, UINT* ibodata, UINT indicesn, UCHAR* texturedata, USHORT texturew, USHORT textureh) {
  RGL_MODEL modelptr = malloc(sizeof(RGL_MODELDATA));
  modelptr->indicesn = indicesn;
  // vao
  rglGenVertexArrays(1, &modelptr->vao);
  rglBindVertexArray(modelptr->vao);

  // vbo
  rglGenBuffers(1, &modelptr->vbo);
  rglBindBuffer(GL_ARRAY_BUFFER, modelptr->vbo);
  // Copy to OpenGL
  rglBufferData(GL_ARRAY_BUFFER, verticesn * sizeof (GL_FLOAT) * 5, vbodata, GL_STATIC_DRAW);
  // Cofigure attributes for the vertices of the model + the vertices of the texture
  rglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
  rglEnableVertexAttribArray(0);
  rglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));
  rglEnableVertexAttribArray(1);

  // ibo
  rglGenBuffers(1, &modelptr->ibo);
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelptr->ibo);
  // Copy
  rglBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesn * sizeof (UINT) * 3, ibodata, GL_STATIC_DRAW);

  // to
  glGenTextures(1, &modelptr->to);
  glBindTexture(GL_TEXTURE_2D, modelptr->to);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texturew, textureh, 0, GL_RGB, GL_UNSIGNED_BYTE, texturedata);
  rglGenerateMipmap(GL_TEXTURE_2D);


  return modelptr;
}

RGL_MODEL RGL_loadmodel(const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load model '%s'.\n", fp);
    return 0;
  }
  
  RGL_MODEL modelptr;

  UINT header[4];

  // TODO: little and big endian
  // HEADER
  fread(header, sizeof (UINT), 4, f);
  float vbodata[header[0]];
  UINT ibodata[header[1]];
  UCHAR texturedata[header[2]*header[3]];

  fread(vbodata, header[0] * 5, sizeof (float), f);
  fread(ibodata, header[1] * 3, sizeof (UINT), f);
  fread(texturedata, header[2]*header[3], sizeof (UCHAR), f);

  fclose(f);

  modelptr = RGL_initmodel(vbodata, header[0], ibodata, header[1], texturedata, header[2], header[3]);

  return modelptr;
}

void RGL_freemodel(RGL_MODEL model) {
  rglDeleteBuffers(1, &model->vbo);
  rglDeleteBuffers(1, &model->ibo);
  rglDeleteVertexArrays(1, &model->vao);
  glDeleteTextures(1, &model->to);
  
  free(model);
}

RGL_BODY RGL_initbody(RGL_MODEL model, UCHAR flags) {
  RGL_BODY bodyptr = malloc(sizeof(RGL_BODYDATA));
  
  bodyptr->model = model;
  bodyptr->flags = flags;
  
  bodyptr->offset[0] = bodyptr->offset[1] = bodyptr->offset[2] = 0;
  bodyptr->angles[0] = bodyptr->angles[1] = bodyptr->angles[2] = 0;
  
  return bodyptr;
}
void RGL_freebody(RGL_BODY bodyptr) {
  free(bodyptr);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Using this for Wine compatability. other shit wont work.
    case WM_SYSCOMMAND:
    if (wParam == SC_CLOSE)
      ExitProcess(0);
    else
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    break;

    case WM_MOUSEMOVE:
    {
      // Extract the mouse coordinates from the lParam parameter
      RGL_mousex = LOWORD(lParam);
      RGL_mousey = HIWORD(lParam);
      break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    int down = (WM_KEYDOWN == uMsg);
    // TODO
    break;

    case WM_LBUTTONDOWN:
    // TODO:
    break;
    
    case WM_LBUTTONUP:
    // TODO:
    break;

    default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}

int RGL_init(UCHAR vsync, int width, int height) {
  hInstance = GetModuleHandleA(NULL);

  RGL_width = width;
  RGL_height = height;
  RGL_usedeye = NULL;

  // Create the class
  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = CLASSNAME;
  if (!RegisterClassEx(&wc)) {
    puts("RGL: Failed to create class!");
    return 0;
  }

  // Adjusting the window size
  // Thank you so much Id for making Quake open source.
  RECT wndrect;
  int truew, trueh;
  
  wndrect.left = 0;
  wndrect.top = 0;
  wndrect.right = width;
  wndrect.bottom = height;
  
  DWORD dwStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX;
  
  AdjustWindowRect (&wndrect, dwStyle, 0);
  truew = wndrect.right - wndrect.left;
  trueh = wndrect.bottom - wndrect.top;

  hWnd = CreateWindowExA(0, CLASSNAME, NULL,
    dwStyle,
    CW_USEDEFAULT, CW_USEDEFAULT,
    truew, trueh,
    NULL, NULL, hInstance, NULL);
  if (!hWnd) {
    puts("RGL: Failed to create window!");
    UnregisterClassA(CLASSNAME, hInstance);
    return 0;
  }

  // Setup the opengl
  hDC = GetDC(hWnd);
  int bpp = GetDeviceCaps(hDC, BITSPIXEL);

  PIXELFORMATDESCRIPTOR pfd = 
  {
    sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
    1,								// version number
    PFD_DRAW_TO_WINDOW |			// support window
    PFD_SUPPORT_OPENGL |			// support OpenGL
    PFD_DOUBLEBUFFER,				// double buffered
    PFD_TYPE_RGBA,					// RGBA type
    bpp,								// 24-bit color depth
    0, 0, 0, 0, 0, 0,				// color bits ignored
    0,								// no alpha buffer
    0,								// shift bit ignored
    0,								// no accumulation buffer
    0, 0, 0, 0, 					// accum bits ignored
    24,								// 24-bit z-buffer	
    0,								// no stencil buffer
    0,								// no auxiliary buffer
    PFD_MAIN_PLANE,		 			// main layer
    0,								// reserved
    0, 0, 0							// layer masks ignored
  };

  int pf = ChoosePixelFormat(hDC, &pfd);
  if (!SetPixelFormat(hDC, pf, &pfd)) {
    puts("RGL: Failed to set pixel format, possibly the bpp parameter was incorrect try 16,24,32.");
    RGL_free();
    return 0;
  }

  hGLRC = wglCreateContext(hDC);
  if (!wglMakeCurrent(hDC, hGLRC)) {
    puts("RGL: Failed to OpenGL.");
    RGL_free();
    return 0;
  }

  RGL_loadgl();

  if (rglSwapInterval)
    rglSwapInterval(vsync?1:0);
  else
    puts("RGL: Could not load glSwapInterval, so can't VSync.");

  printf("RGL: Using OpenGL %s.\n", glGetString(GL_VERSION));

  // Some basic OpenGL setup.
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  // Clear for the start
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  return 1;
}

void RGL_drawbodies(RGL_BODY* bodies, UINT _i, UINT n) {
  if (!RGL_usedeye)
    puts("RGL: NO USED EYES!");

  for (int i = 0; i < n; i++) {
    RGL_MODEL model = bodies[i+_i]->model;
    useprogram(RGL_usedeye);

    uniformbody(RGL_usedeye->program, bodies[i+_i]);

     glBindTexture(GL_TEXTURE_2D, model->to);
    rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
    rglBindVertexArray(model->vao);

    rglDrawElements(GL_TRIANGLES, model->indicesn*3, GL_UNSIGNED_INT, 0);
  }
}

void RGL_refresh() {
  SwapBuffers(hDC);

  static MSG Msg;

  while(PeekMessage(&(Msg), NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&(Msg));
    DispatchMessageA(&(Msg));
  }
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RGL_free() {
  wglMakeCurrent(hDC, NULL);
  wglDeleteContext(hGLRC);

  ReleaseDC(hWnd, hDC);
  DestroyWindow(hWnd);
  UnregisterClassA(CLASSNAME, GetModuleHandle(NULL));
}
