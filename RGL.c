#include <windows.h>

#define RGL_SRC
#include "RGL.h"
#include "RGL_loader.h"

#include <GL/wgl.h>
#include <GL/wglext.h>

#include <stdio.h>

#define CLASSNAME "RGLWND"

static HINSTANCE hInstance;

static HWND hWnd = NULL;
static HDC hDC = NULL;

static HGLRC hGLRC = NULL;

typedef BOOL (*WGLSWAPINTERVAL) (int interval);

static WGLSWAPINTERVAL wglSwapIntervalEXT = NULL;

// Pass the actual opengl enum to type, like GL_VERTEX_SHADER.
UINT RGL_loadshader(const char* fp, UINT type) {
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
  
  puts(data);

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
    printf("RGL: Compilation of shader '%s' unsuccessful.\nINFO: '%s'\n", fp, info);
    RGL_freeshader(shader);
    return 0;
  }

  return shader;
}

void RGL_freeshader(UINT shader) {
  rglDeleteShader(shader);
}

UINT RGL_initprogram(UINT vertshader, UINT fragshader) {
  UINT program = rglCreateProgram();
  if (!program) {
    printf("RGL: Creation of program failed.");
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
    printf("RGL: Program linking failed.");
    return 0;
  }

  return program;
}

UINT RGL_loadprogram(const char* fp) {
  FILE* f = fopen(fp, "rb");
  if (!f) {
    printf("RGL: Program loading failed.");
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

  return program;
}

void RGL_saveprogram(UINT program, const char* fp) {
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

void RGL_freeprogram(UINT program) {
  rglDeleteProgram(program);
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
  SetPixelFormat(hDC, pf, &pfd);

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

  RGL_loadgl();

  return 1;
}

void RGL_refresh() {
  static MSG Msg;

  while(PeekMessage(&(Msg), NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&(Msg));
    DispatchMessageA(&(Msg));
  }

  glClear(GL_COLOR_BUFFER_BIT);

  glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(1, 1, 0);
    glColor3f(0, 1, 0);
    glVertex3f(1, 0, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 1, 0);
  glEnd();

  SwapBuffers(hDC);
}

void RGL_free() {
  wglMakeCurrent(hDC, NULL);
  wglDeleteContext(hGLRC);


  ReleaseDC(hWnd, hDC);
  DestroyWindow(hWnd);
  UnregisterClassA(CLASSNAME, GetModuleHandle(NULL));
}
