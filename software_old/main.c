#include "TWE.h"
#include "D3D.h"

#include <stdlib.h>
#include <windows.h>

#define LEFT 37
#define UP 38
#define RIGHT 39
#define DOWN 40

int main() {
	TWE_init(1, 16, 16);
	D3D_init(TWE_pixels, 16*8, 16*8);

	D3D_MODEL mdl = {0};

	D3D_VEC p[] = {
    {0.5+10, 0.5, 0.5},    // Vertex 0
    {-0.5+10, 0.5, 0.5},   // Vertex 1

    {-0.5+10, -0.5, 0.5},  // Vertex 2
    {0.5+10, -0.5, 0.5},   // Vertex 3

    {0.5+10, 0.5, -0.5},   // Vertex 4
    {-0.5+10, 0.5, -0.5},  // Vertex 5

    {-0.5+10, -0.5, -0.5}, // Vertex 6
    {0.5+10, -0.5, -0.5}   // Vertex 7
	};

	D3D_TRI t[] = {
	    {0, 1, 2},  // Front face triangle 1
	    {0, 2, 3},  // Front face triangle 2
	    {0, 3, 4},  // Right face triangle 1
	    {4, 3, 7},  // Right face triangle 2
	    {4, 7, 6},  // Back face triangle 1
	    {4, 6, 5},  // Back face triangle 2
	    {4, 5, 0},  // Left face triangle 1
	    {0, 5, 1},  // Left face triangle 2
	    {1, 5, 6},  // Top face triangle 1
	    {1, 6, 2},  // Top face triangle 2
	    {3, 2, 7},  // Bottom face triangle 1
	    {7, 2, 6}   // Bottom face triangle 2
	};

	mdl.tris = t;
	mdl.vecs = p;
	mdl.vecsn = 8;
	mdl.trisn = 12;
	mdl.next = NULL;

	D3D_models = &mdl;

	while (1) {
		DWORD start = GetTickCount();

		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				TWE_col = j;
				TWE_row = i;
				TWE_erase();
			}
		}
		
		D3D_draw();
		for (int i = 0; i < 3; i++) {
			D3D_drawpoint(p[i]);
		}

		for (int i = 0; TWE_inputs[i].code; i++) {
			if (!TWE_inputs[i].down) {
				switch (TWE_inputs[i].code) {
					case UP:
					D3D_cam.obj.angles[1]-=0.1f;
					break;
					case DOWN:
					D3D_cam.obj.angles[1]+=0.1f;
					break;
					case RIGHT:
					D3D_cam.obj.angles[2]-=0.1f;
					break;
					case LEFT:
					D3D_cam.obj.angles[2]+=0.1f;
					break;

					case 'W':
					printf("S ");
					D3D_cam.obj.offset[0] += 0.6f;
					break;

					case 'S':
					D3D_cam.obj.offset[0] -= 0.6f;
					break;

					case 'A':
					D3D_cam.obj.offset[1] -= 0.6f;
					break;

					case 'D':
					D3D_cam.obj.offset[1] += 0.6f;
					break;
				}
			}

		}
		
    TWE_print();
		// Sleep(15);
		DWORD end = GetTickCount();
		printf("%d\n",end-start);
	}

	return 0;
}
