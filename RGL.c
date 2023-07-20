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

  // UCHAR rgb[] = {0, 0, 4, 8, 8, 8, 24, 8, 12, 8, 28, 20, 40, 0, 0, 32, 8, 28, 12, 8, 32, 24, 28, 20, 36, 8, 40, 60, 56, 56, 44, 40, 40, 68, 56, 56, 108, 92, 92, 80, 72, 68, 120, 112, 108, 92, 84, 80, 116, 104, 96, 128, 120, 108, 140, 132, 116, 132, 128, 116, 116, 112, 96, 192, 192, 184, 96, 96, 84, 104, 104, 92, 212, 212, 208, 184, 184, 176, 68, 68, 60, 80, 84, 72, 136, 140, 128, 120, 128, 108, 204, 208, 200, 72, 76, 68, 60, 64, 56, 148, 156, 140, 160, 164, 156, 128, 140, 120, 104, 116, 100, 96, 108, 92, 216, 220, 216, 196, 200, 196, 84, 96, 84, 172, 180, 172, 224, 228, 224, 136, 148, 136, 148, 160, 152, 48, 56, 52, 116, 124, 120, 156, 172, 164, 144, 152, 148, 136, 152, 152, 184, 188, 188, 176, 180, 180, 132, 140, 140, 144, 156, 160, 152, 160, 164, 60, 60, 64, 168, 168, 172, 88, 88, 96, 128, 128, 132, 80, 80, 84, 72, 72, 76, 108, 108, 116, 100, 96, 104, 68, 60, 68, 56, 40, 40, 56, 24, 24, 128, 84, 80, 44, 8, 8, 164, 88, 84, 160, 60, 60, 132, 52, 36, 196, 116, 116, 44, 28, 24, 196, 96, 88, 200, 104, 96, 124, 12, 8, 208, 108, 100, 208, 100, 88, 212, 140, 128, 212, 124, 112, 56, 48, 44, 108, 80, 68, 216, 156, 144, 140, 108, 88, 44, 24, 8, 192, 144, 112, 220, 180, 164, 184, 52, 20, 56, 40, 24, 132, 116, 100, 140, 124, 104, 128, 104, 76, 148, 128, 100, 156, 120, 88, 168, 132, 100, 180, 148, 116, 180, 144, 100, 192, 164, 128, 196, 160, 112, 204, 176, 140, 64, 56, 44, 76, 68, 56, 104, 96, 84, 92, 84, 64, 100, 92, 72, 116, 108, 84, 124, 116, 96, 112, 100, 76, 148, 140, 124, 148, 140, 116, 140, 128, 96, 136, 120, 88, 148, 132, 108, 124, 116, 88, 148, 136, 100, 204, 172, 120, 136, 132, 108, 144, 136, 108, 152, 140, 108, 156, 140, 116, 160, 148, 128, 44, 40, 24, 156, 148, 116, 164, 156, 132, 164, 148, 116, 188, 180, 160, 180, 160, 120, 196, 188, 168, 192, 176, 144, 196, 176, 128, 204, 188, 152, 208, 184, 132, 104, 104, 84, 128, 128, 100, 44, 40, 8, 176, 168, 144, 180, 168, 132, 204, 196, 168, 216, 208, 176, 212, 196, 148, 216, 204, 160, 112, 116, 88, 204, 204, 188, 172, 176, 156, 192, 196, 172, 192, 200, 144, 40, 44, 28, 80, 84, 64, 72, 80, 60, 84, 92, 72, 96, 104, 72, 208, 212, 196, 88, 100, 76, 76, 96, 56, 64, 84, 44, 100, 116, 80, 52, 72, 32, 140, 152, 128, 140, 160, 112, 136, 172, 100, 156, 176, 124, 148, 180, 108, 172, 192, 152, 164, 188, 132, 216, 220, 208, 92, 104, 84, 80, 104, 68, 88, 116, 72, 104, 128, 84, 128, 148, 112, 112, 140, 88, 120, 148, 100, 124, 160, 96, 104, 132, 96, 92, 128, 80, 100, 140, 88, 116, 156, 104, 112, 152, 92, 128, 168, 112, 156, 176, 144, 136, 176, 116, 148, 188, 124, 72, 92, 68, 108, 144, 100, 76, 96, 76, 80, 108, 80, 96, 120, 92, 88, 116, 84, 120, 160, 116, 136, 172, 132, 16, 68, 8, 60, 76, 64, 68, 84, 72, 40, 84, 56, 24, 36, 40, 20, 48, 52, 148, 164, 172, 156, 168, 176, 164, 176, 184, 8, 36, 48, 152, 172, 184, 0, 56, 92, 44, 40, 56, 20, 12, 48, 32, 20, 80, 36, 28, 40, 60, 48, 68, 88, 76, 100, 76, 60, 84, 44, 12, 56, 56, 44, 56, 56, 40, 56, 68, 48, 68, 60, 20, 64, 100, 48, 96, 44, 28, 40, 52, 28, 48, 44, 20, 40, 132, 56, 108, 48, 8, 40, 56, 8, 48, 88, 16, 76, 72, 12, 52, 64, 44, 56, 84, 68, 76, 68, 36, 52, 56, 20, 40, 80, 56, 64, 56, 28, 40, 96, 32, 56, 48, 8, 24, 44, 20, 24, 112, 52, 64, 128, 20, 56, 108, 12, 48, 148, 8, 52, 188, 8, 88, 80, 48, 52, 136, 64, 68, 152, 72, 76, 128, 48, 52, 68, 12, 16, 132, 40, 60, 168, 68, 76, 184, 72, 80, 156, 44, 68, 188, 56, 72, 196, 80, 84, 168, 36, 60, 96, 8, 20, 212, 124, 128, 192, 28, 36};
  // int rgbn = 252;

  // for (int i = 0; i < 252; i++) {
  //   int ci = i*4;
  //   int ri = i*3;
  //   RGL_colors[ci] = rgb[ri]/255.0f;
  //   RGL_colors[ci+1] = rgb[ri+1]/255.0f;
  //   RGL_colors[ci+2] = rgb[ri+2]/255.0f;
  //   RGL_colors[ci+3] = 1;
  // }

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

  // ibo
  rglGenBuffers(1, &modelptr->ibo);
  rglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelptr->ibo);
  // Copy
  rglBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesn * sizeof (UINT) * 3, ibodata, GL_STATIC_DRAW);

  // to
  glGenTextures(1, &modelptr->to);
  glBindTexture(GL_TEXTURE_2D, modelptr->to);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texturew, textureh, 0, GL_RGB, GL_UNSIGNED_BYTE, texturedata);
  // rglGenerateMipmap(GL_TEXTURE_2D);


  return modelptr;
}

RGL_MODEL RGL_loadmodel(const char* fp, const char* texturefp) {
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

  // Texture
  f = fopen(texturefp, "rb");
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

  RGL_MODEL modelptr = RGL_initmodel(v, vn, faces, fn, texture, tw, th);

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

  printf("RGL: Using OpenGL %s.\n", glGetString(GL_VERSION));

  // Some basic OpenGL setup.
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  // Clear for the start
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return 1;
}

void RGL_settitle(const char* title) {
  SetWindowTextA(hWnd, title);
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
