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

// Ratio factor should be 1 for the d_max on the x axis.
static float calcrd_max(RGL_EYE eye) {
  float ratio = (1.0f*RGL_height)/RGL_width;
  float rd_max = 1 / (tanf(eye->fov/2) * eye->info.p_near);
  eye->info.rdx_max = ratio * rd_max;
  eye->info.rdy_max = 1 * rd_max;
}

// Assumes program is currently in use
static void useprogram(RGL_EYE eye) {
  // Bind and copy ubo data.
  calcrd_max(eye);
  rglUseProgram(eye->program);
  rglBindBuffer(GL_UNIFORM_BUFFER, eye->ubo);
  rglBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(eye->info), &eye->info);
  
  // rglBindBuffer(GL_UNIFORM_BUFFER, eye->colorsubo);
  // rglBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RGL_colors), RGL_colors);
}

RGL_EYE RGL_initeye(RGL_PROGRAM program, float fov) {
  RGL_EYE eyeptr = malloc(sizeof(RGL_EYEDATA));
  eyeptr->program = program;

  zerovec(eyeptr->info.offset);
  // eyeptr->info.offset[2] = -10;
  zerovec(eyeptr->info.angles);
  eyeptr->fov = fov;
  eyeptr->info.p_far = 500.0f;
  eyeptr->info.p_near = 0.0001f;
  
  // Setup the ubo that holds the eye info, which is in eyeptr->info
  rglUseProgram(eyeptr->program);
  rglGenBuffers(1, &eyeptr->ubo);
  rglBindBuffer(GL_UNIFORM_BUFFER, eyeptr->ubo);
  rglBufferData(GL_UNIFORM_BUFFER, sizeof(eyeptr->info), 0, GL_DYNAMIC_DRAW);
  
  int i = rglGetUniformBlockIndex(program, "RGL_eye");
  rglUniformBlockBinding(program, i, 0);
  rglBindBufferBase(GL_UNIFORM_BUFFER, 0, eyeptr->ubo);

  // Setup the colors palette
  rglGenBuffers(1, &eyeptr->colorsubo);
  rglBindBuffer(GL_UNIFORM_BUFFER, eyeptr->colorsubo);
  rglBufferData(GL_UNIFORM_BUFFER, sizeof(RGL_colors), RGL_colors, GL_STATIC_DRAW);
  i = rglGetUniformBlockIndex(program, "RGL_palette");
  rglUniformBlockBinding(program, i, 1);
  rglBindBufferBase(GL_UNIFORM_BUFFER, 1, eyeptr->colorsubo);

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

RGL_MODEL RGL_initmodel(float* vbodata, UINT verticesn, UINT* fbodata, UINT facesn, RGL_TEXTURE texture) {
  RGL_MODEL modelptr = malloc(sizeof(RGL_MODELDATA));
  modelptr->facesn = facesn;
  // vao
  rglGenVertexArrays(1, &modelptr->vao);
  rglBindVertexArray(modelptr->vao);

  // vbo
  rglGenBuffers(1, &modelptr->vbo);
  rglBindBuffer(GL_ARRAY_BUFFER, modelptr->vbo);
  // Copy to OpenGL
  rglBufferData(GL_ARRAY_BUFFER, verticesn * sizeof (float) * 8, vbodata, GL_STATIC_DRAW);
  
  // Vertices
  rglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
  rglEnableVertexAttribArray(0);
  // Vertex normals
  rglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  rglEnableVertexAttribArray(1);
  // UV Vertices
  rglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  rglEnableVertexAttribArray(2);

  // fbo
  rglGenBuffers(1, &modelptr->fbo);
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelptr->fbo);
  // Copy
  rglBufferData(GL_ELEMENT_ARRAY_BUFFER, facesn * sizeof (UINT) * 3, fbodata, GL_STATIC_DRAW);

  // to
  modelptr->to = texture;

  return modelptr;
}

RGL_MODEL RGL_loadmodel(const char* fp, RGL_TEXTURE texture) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load model '%s'.\n", fp);
    return 0;
  }

  UINT vn, fn;

  fread(&vn, sizeof(vn), 1, f);
  fread(&fn, sizeof(fn), 1, f);
  float v[vn*8];
  UINT faces[fn*3];
  
  fread(v, sizeof(float), vn*8, f);
  fread(faces, sizeof(UINT), fn*3, f);

  fclose(f);

  RGL_MODEL modelptr = RGL_initmodel(v, vn, faces, fn, texture);

  return modelptr;
}

void RGL_freemodel(RGL_MODEL model) {
  rglDeleteBuffers(1, &model->vbo);
  rglDeleteBuffers(1, &model->fbo);
  rglDeleteVertexArrays(1, &model->vao);
  
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

RGL_TEXTURE RGL_loadtexture(const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load texture '%s'.\n", fp);
    return 0;
  }

  UINT tw, th;
  fread(&tw, sizeof(tw), 1, f);
  fread(&th, sizeof(th), 1, f);

  UCHAR texture[tw*th*3];

  fread(texture, tw*th, 3, f);
  fclose(f);

  // OpenGL
  RGL_TEXTURE to;

  glGenTextures(1, &to);
  glBindTexture(GL_TEXTURE_2D, to);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

  return to;
}
void RGL_freetexture(RGL_TEXTURE texture) {
  glDeleteTextures(1, &texture);
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
    float speed = 0.1f;
    if (down) {
      if (wParam == 'W') {
        RGL_usedeye->info.offset[2] += speed;
      }
      else if (wParam == 'S') {
        RGL_usedeye->info.offset[2] -= speed;
      }
      else if (wParam == 'A') {
        RGL_usedeye->info.offset[0] -= speed;
      }
      else if (wParam == 'D') {
        RGL_usedeye->info.offset[0] += speed;
      }
      else if (wParam == 'Q') {
        RGL_usedeye->info.angles[1] += speed;
      }
      else if (wParam == 'E') {
        RGL_usedeye->info.angles[1] -= speed;
      }
    }
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

void RGL_setcursor(char shown, char captured) {
  if (captured) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    MapWindowPoints(hWnd, NULL, (POINT*)&rc, 2);
    ClipCursor(&rc);
  }
  else
    ClipCursor(NULL);
  
  ShowCursor(shown);
}

int RGL_loadcolors(const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load colors file '%s'.\n", fp);
    return 0;
  }

  int n;
  fread(&n, sizeof(n), 1, f);

  UCHAR colors[n*3];
  fread(colors, 3, n, f);

  fclose(f);

  for (int i = 0; i < n; i++) {
    int rci = i*4;
    int ci  = i*3;
    RGL_colors[rci+0] = colors[ci+0]/255.0f;
    RGL_colors[rci+1] = colors[ci+1]/255.0f;
    RGL_colors[rci+2] = colors[ci+2]/255.0f;
    RGL_colors[rci+3] = 1;
  }

  glClearColor(RGL_colors[0], RGL_colors[1], RGL_colors[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return 1;
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

  printf("RGL: Successful initialization.\nRGL: Using OpenGL %s.\n", glGetString(GL_VERSION));

  // Some basic OpenGL setup.
  glEnable(GL_DEPTH_TEST);
  // Clear for the start


  return 1;
}

void RGL_settitle(const char* title) {
  SetWindowTextA(hWnd, title);
}

void RGL_begin() {
  if (!RGL_usedeye)
    puts("RGL: NO USED EYES!");
  
  useprogram(RGL_usedeye);
}

void RGL_drawbody(RGL_BODY body) {
  RGL_MODEL model = body->model;

  uniformbody(RGL_usedeye->program, body);

  glBindTexture(GL_TEXTURE_2D, model->to);
  rglBindVertexArray(model->vao);
  // HISTORICAL NOTE: This fucking call caused me so much pain, apparently OpenGL reads the vao the moment you bind the EBO, hence biinding the VAO after the EBO causes OpenGL to not actually read that VAO we just bound, but reather the last one, GG.
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->fbo);

  rglDrawElements(GL_TRIANGLES, model->facesn*3, GL_UNSIGNED_INT, 0);
}

void RGL_drawbodies(RGL_BODY* bodies, UINT _i, UINT n) {
  for (int i = 0; i < n; i++) {
    RGL_drawbody(bodies[i+_i]);
  }
}

void RGL_end() {
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
