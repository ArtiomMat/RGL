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

static float max_d;

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

// init basic uniforms
static void uniformsinit(RGL_PROGRAM program) {
  INT i;
  float pnear = 0.2f;
  i = rglGetUniformLocation(program, "RGL_p_near");
  rglUniform1f(i, pnear);
  i = rglGetUniformLocation(program, "RGL_p_far");
  rglUniform1f(i, 300.0f);
  i = rglGetUniformLocation(program, "RGL_d_max");
  rglUniform1f(i, max_d);
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

RGL_MODEL RGL_initmodel(RGL_PROGRAM program, float* vbodata, UINT verticesn, UINT* ibodata, UINT indicesn, UCHAR* texturedata, USHORT texturew, USHORT textureh) {
  RGL_MODEL modelptr = malloc(sizeof(RGL_MODELDATA));
  modelptr->program = program;
  modelptr->indicesn = indicesn;
  // vao
  rglGenVertexArrays(1, &modelptr->vao);
  rglBindVertexArray(modelptr->vao);

  // vbo
  rglGenBuffers(1, &modelptr->vbo);
  rglBindBuffer(GL_ARRAY_BUFFER, modelptr->vbo);
  // Copy to OpenGL
  rglBufferData(GL_ARRAY_BUFFER, verticesn * sizeof (GL_FLOAT) * 5, vbodata, /*TODO*/GL_STATIC_DRAW);
  // Cofigure attributes for the vertices of the model + the vertices of the texture
  rglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
  rglEnableVertexAttribArray(0);
  rglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));
  rglEnableVertexAttribArray(1);

  // ibo
  rglGenBuffers(1, &modelptr->ibo);
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelptr->ibo);
  // Copy
  rglBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesn * sizeof (UINT) * 3, ibodata, /*TODO*/GL_STATIC_DRAW);

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

RGL_MODEL RGL_loadmodel(const char* fp, RGL_PROGRAM program) {
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

  modelptr = RGL_initmodel(program, vbodata, header[0], ibodata, header[1], texturedata, header[2], header[3]);

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
    // TODO:
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

int RGL_init(UCHAR bpp, UCHAR vsync, int width, int height) {
  hInstance = GetModuleHandleA(NULL);

  RGL_width = width;
  RGL_height = height;

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

  // Load the vsync thingy and enable if we need to
  if (!wglSwapIntervalEXT)
    wglSwapIntervalEXT = (WGLSWAPINTERVAL)wglGetProcAddress("wglSwapIntervalEXT");
  
  if (wglSwapIntervalEXT)
    wglSwapIntervalEXT(vsync?1:0);
  else
    puts("RGL: Could not load wglSwapIntervalEXT, so can't VSync.");

  printf("RGL: OpenGL version is '%s'.\n", glGetString(GL_VERSION));

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);

  RGL_loadgl();

  // Setup the projection matrix:
  // Columns not rows

  max_d = tanf(1.7/2) * 0.2f;
  
  return 1;
}

void RGL_begin(char doclear) {
  if (doclear)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void drawmodel(RGL_MODEL model) {
  rglUseProgram(model->program);

  uniformsinit(model->program);

  glBindTexture(GL_TEXTURE_2D, model->to);
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
  rglBindVertexArray(model->vao);

  rglDrawElements(GL_TRIANGLES, model->indicesn*3, GL_UNSIGNED_INT, 0);
}

void RGL_drawmodels(RGL_MODEL* models, UINT _i, UINT n) {
  for (int i = 0; i < n; i++) {
    drawmodel(models[i+_i]);
  } 
}

void RGL_drawbodies(RGL_BODY* bodies, UINT _i, UINT n) {
  for (int i = 0; i < n; i++) {
    RGL_MODEL model = bodies[i+_i]->model;
    rglUseProgram(model->program);

    uniformbody(model->program, bodies[i+_i]);
    uniformsinit(model->program);

    glBindTexture(GL_TEXTURE_2D, model->to);
    rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
    rglBindVertexArray(model->vao);

    rglDrawElements(GL_TRIANGLES, model->indicesn*3, GL_UNSIGNED_INT, 0);
  }
}

void RGL_end() {
  SwapBuffers(hDC);

  static MSG Msg;

  while(PeekMessage(&(Msg), NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&(Msg));
    DispatchMessageA(&(Msg));
  }
}

void RGL_free() {
  wglMakeCurrent(hDC, NULL);
  wglDeleteContext(hGLRC);

  ReleaseDC(hWnd, hDC);
  DestroyWindow(hWnd);
  UnregisterClassA(CLASSNAME, GetModuleHandle(NULL));
}
