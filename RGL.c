#include <windows.h>

#define RGL_SRC
#include "RGL.h"
#include "RGL_loader.h"

#include <GL/wgl.h>
#include <GL/wglext.h>

#include <stdio.h>
#include <math.h>

#define CLASSNAME "RGLWND"

// TODO: HOW ABOUT YOU GET YOUR SHIT TOGETHER AND MAKE ALL THE NAMES IN THE SHADERS AND PROGRAM TO MATCH AH?
enum {
  EYEUBOBINDING,
  LIGHTSUBOBINDING,
  SUNUBOBINDING,
  PALETTEUBOBINDING,
};

static HINSTANCE hInstance;

static HWND hWnd = NULL;
static HDC hDC = NULL;

static HGLRC hGLRC = NULL;

static char cursorcaptured = 0;

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

static void setvec(RGL_VEC a, float v) {
  a[0] = v;
  a[1] = v;
  a[2] = v;
}


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

  const char* ptrdata = data; // Doing some dark magic because can't pass array pointers
  rglShaderSource(shader, 1, &ptrdata, NULL);
  
  rglCompileShader(shader);
  int success;
  rglGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info[512];
    info[0] = 'c';
    info[1] = 0;
    rglGetShaderInfoLog(shader, sizeof(info), NULL, info);
    printf("RGL: Compilation of shader '%s' unsuccessful.\nOpenGL Log:\n%s\n", fp, info);
    RGL_freeshader(shader);
    return 0;
  }

  return shader;
}

void RGL_freeshader(RGL_SHADER shader) {
  rglDeleteShader(shader);
}

// TODO: Make the program store the locations of uniforms, cache the mfs.
// Make it a more complex structure.
// Used to send all uniform information about the body to the shader
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

  rglAttachShader(program, vertshader);
  rglAttachShader(program, fragshader);

  rglLinkProgram(program);
  int success;
  rglGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    printf("RGL: Program linking failed. Not a common occurance(ERROR: %d).\n", 
    glGetError());
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
    printf("RGL: Loaded program '%s' is invalid.\n", fp);
    RGL_freeprogram(program);
    return 0;
  }

  return program;
}

int RGL_saveprogram(RGL_PROGRAM program, const char* fp) {
  GLsizei len;
  GLenum format;
  rglGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);

  char data[len];

  rglGetProgramBinary(program, len, NULL, &format, data);

  FILE* f = fopen(fp, "wb");
  if (!f) {
    printf("RGL: Could not save program '%s'.\n", fp);
    return 0;
  }

  fwrite(&format, sizeof(GLenum), 1, f);
  fwrite(&len, sizeof(GLsizei), 1, f);
  fwrite(data, len, 1, f);

  fclose(f);
  return 1;
}

void RGL_freeprogram(RGL_PROGRAM program) {
  rglDeleteProgram(program);
}

// Calculates rdx/y_max for the eye.
static void calcrd_max(RGL_EYE eye) {
  float ratio = (1.0f*RGL_height)/RGL_width;
  float rd_max = 1 / (tanf(eye->fov/2) * eye->info.p_near);
  eye->info.rdx_max = ratio * rd_max;
  eye->info.rdy_max = 1 * rd_max;
}

// Apart from actually using the program it sets up the eye's UBOs and stuff.
// This should be called only in RGL_begin, only when the program begins drawing a new frame.
static void useprogram(RGL_EYE eye) {
  calcrd_max(eye);
  rglUseProgram(eye->program);
  // Bind and copy ubo data.
  rglBindBuffer(GL_UNIFORM_BUFFER, eye->ubo);
  rglBufferSubData(GL_UNIFORM_BUFFER, EYEUBOBINDING, sizeof(eye->info), &eye->info);

  // lights
  rglBindBuffer(GL_UNIFORM_BUFFER, eye->lightsubo);
  for (int i = 0; i < RGL_MAXLIGHTSN; i++) {
    if (!eye->lights[i]) { // The array is terminated with (RGL_LIGHT)0
      eye->sun.lightsn = i;
      break;
    }
    rglBufferSubData(GL_UNIFORM_BUFFER, i*sizeof(RGL_LIGHTDATA), sizeof(RGL_LIGHTDATA), eye->lights[i]);
  }

  // sun
  // NOTE: WE SET UP LIGHTSN IN THE LOOP
  rglBindBuffer(GL_UNIFORM_BUFFER, eye->sunubo);
  rglBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(eye->sun), &eye->sun);
}

// Returns the UBO
static UINT initubo(RGL_PROGRAM program, int index, int bufsize, const char* uniformname) {  
  UINT ubo;

  rglUseProgram(program);
  rglGenBuffers(1, &ubo);
  rglBindBuffer(GL_UNIFORM_BUFFER, ubo);
  rglBufferData(GL_UNIFORM_BUFFER, bufsize, 0, GL_DYNAMIC_DRAW);
  
  // Actually bind the UBO to the uniform
  int i = rglGetUniformBlockIndex(program, uniformname);
  rglUniformBlockBinding(program, i, index);
  rglBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);

  return ubo;
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
  // rglUseProgram(eyeptr->program);
  // rglGenBuffers(1, &eyeptr->ubo);
  // rglBindBuffer(GL_UNIFORM_BUFFER, eyeptr->ubo);
  // rglBufferData(GL_UNIFORM_BUFFER, sizeof(eyeptr->info), 0, GL_DYNAMIC_DRAW);
  // // Actually bind the UBO to the uniform
  // int i = rglGetUniformBlockIndex(program, "RGL_eye");
  // rglUniformBlockBinding(program, i, 0);
  // rglBindBufferBase(GL_UNIFORM_BUFFER, 0, eyeptr->ubo);
  eyeptr->ubo = initubo(eyeptr->program, EYEUBOBINDING, sizeof(eyeptr->info), "RGL_eye");
  eyeptr->lightsubo = initubo(eyeptr->program, LIGHTSUBOBINDING, sizeof(RGL_LIGHTDATA)*RGL_MAXLIGHTSN, "RGL_lights");
  eyeptr->sunubo = initubo(eyeptr->program, SUNUBOBINDING, sizeof(eyeptr->sun), "RGL_sun");

  zerovec(eyeptr->sun.suncolor);
  zerovec(eyeptr->sun.sundir);

  eyeptr->sun.suncolor[0] = 2;
  eyeptr->sun.suncolor[1] = 2;
  eyeptr->sun.suncolor[2] = 2;
  eyeptr->sun.sundir[0] = -1;

  if (!RGL_usedeye)
    RGL_usedeye = eyeptr;

  return eyeptr;
}

int RGL_loadcolors(RGL_EYE eyeptr, const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Could not load color palette '%s'.\n", fp);
    return 0;
  }

  int n;
  fread(&n, sizeof(n), 1, f);

  static float colors[256*4];
  for (int i = 0; i < n*4; i+=4) {
    colors[i+0] = ((UCHAR)fgetc(f))/255.0f;
    colors[i+1] = ((UCHAR)fgetc(f))/255.0f;
    colors[i+2] = ((UCHAR)fgetc(f))/255.0f;
    colors[i+3] = 1.0f;
  }

  fclose(f);

  // Setup the colors palette
  // TODO: Make RGL_COLORS, and make it just a UBO. then in RGL_initeye make it bind to the binding block.
  rglGenBuffers(1, &eyeptr->colorsubo);
  rglBindBuffer(GL_UNIFORM_BUFFER, eyeptr->colorsubo);
  rglBufferData(GL_UNIFORM_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
  int i = rglGetUniformBlockIndex(eyeptr->program, "RGL_palette");
  rglUniformBlockBinding(eyeptr->program, i, PALETTEUBOBINDING);
  rglBindBufferBase(GL_UNIFORM_BUFFER, PALETTEUBOBINDING, eyeptr->colorsubo);

  // Clear the mf
  glClearColor(colors[0], colors[1], colors[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return 1;
}

void RGL_freeeye(RGL_EYE eye) {
  if (RGL_usedeye == eye)
    RGL_usedeye = NULL;

  rglDeleteBuffers(1, &eye->ubo);
  rglDeleteBuffers(1, &eye->colorsubo);
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
  rglBufferData(GL_ARRAY_BUFFER, verticesn * sizeof (float) * 8, vbodata, GL_STATIC_DRAW);
  
  // Adding the attributes to the vbo
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

RGL_TEXTURE RGL_loadtexture(const char* fp, char mipmap) {
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
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap?GL_NEAREST_MIPMAP_NEAREST:GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
  if (mipmap)
    rglGenerateMipmap(GL_TEXTURE_2D);

  return to;
}
void RGL_freetexture(RGL_TEXTURE texture) {
  glDeleteTextures(1, &texture);
}

RGL_LIGHT RGL_initlight(float strength) {
  RGL_LIGHT light = malloc(sizeof(RGL_LIGHTDATA));
  zerovec(light->offset);
  setvec(light->color, 1);
  return light;
}
void RGL_freelight(RGL_LIGHT light) {
  free(light);
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

      if (cursorcaptured)
        SetCursorPos(RGL_width>>1, RGL_height>>1);
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

void RGL_setcursor(char captured) {
  cursorcaptured = captured;
  ShowCursor(captured);
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

  return 1;
}

void RGL_settitle(const char* title) {
  SetWindowTextA(hWnd, title);
}

void RGL_begin() {
  if (!RGL_usedeye) {
    puts("RGL: NO USE EYES!");
    return;
  }
  
  useprogram(RGL_usedeye);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
}

void RGL_free() {
  wglMakeCurrent(hDC, NULL);
  wglDeleteContext(hGLRC);

  ReleaseDC(hWnd, hDC);
  DestroyWindow(hWnd);
  UnregisterClassA(CLASSNAME, GetModuleHandle(NULL));
}
