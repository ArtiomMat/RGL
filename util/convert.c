// This program converts png files to RG2 files and obj files to RG3.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>

// A face.
typedef struct {
  // Please note these indices, start from 1 like in obj not 0.
  int v, vt, vn;
} OBJ_FELEMENT;

typedef float VEC2[2], VEC3[3];

typedef OBJ_FELEMENT OBJ_F[3];
typedef int INT3[3];

typedef struct {
  VEC3* v;
  VEC3* vn;
  VEC2* vt;
  OBJ_F* f;
  int v_n, vn_n, vt_n, f_n;
} OBJ;

typedef struct {
  VEC3 o; // Offset
  VEC3 n; // Normal
  VEC2 t; // Texture coordinate
} RG3_VERTEX;

typedef struct {
  RG3_VERTEX* v;
  INT3* f;
  int v_n, f_n;
} RG3;

typedef struct {
  int w, h;
  char* pixels;
} RG2;

// Returns line length, not including \n in this case \0
// Replaces \n with \0
int freadl(char* str, int maxsize, FILE* f) {
  for (int i = 0; i < maxsize; i++) {
    int c = fgetc(f);
    if (c == '\n' || c == EOF)
      c = 0;
    
    str[i] = c;
    if (!c)
      return i;
  }
  return maxsize;
}

void loadobj(const char* fp, OBJ* obj) {
  FILE* f = fopen(fp, "rb");

  #define LINEMAXSIZE 128
  char line[LINEMAXSIZE] = {1};

  // Count the attributes
  obj->f_n = obj->v_n = obj->vn_n = obj->vt_n = 0;
  while (1) {
    freadl(line, LINEMAXSIZE, f);
    if (!line[0])
      break;
    
    if (line[0] == 'v') {
      if (line[1] == ' ')
        obj->v_n++;
      else if (line[1] == 't')
        obj->vt_n++;
      else if (line[2] == 'n')
        obj->vn_n++;
    }
    else if (line[0] == 'f') {
      obj->f_n++;
    }
  }

  // Allocate
  obj->f = malloc(sizeof(*obj->f) * obj->f_n);
  obj->v = malloc(sizeof(*obj->v) * obj->v_n);
  obj->vn = malloc(sizeof(*obj->vn) * obj->vn_n);
  obj->vt = malloc(sizeof(*obj->vt) * obj->vt_n);

  rewind(f);

  // Copy the vertices and shit
  int fi=0, vi=0, vni=0, vti=0;
  while (1) {
    freadl(line, LINEMAXSIZE, f);
    if (!line[0])
      break;
        if (!line[0])
      break;
    
    if (line[0] == 'v') {
      if (line[1] == ' ') {
        sscanf(line, "v %f %f %f", &obj->v[vi][0], &obj->v[vi][1], &obj->v[vi][2]);
        vi++;
      }
      else if (line[1] == 't') {
        sscanf(line, "vt %f %f", obj->vt[vti], obj->vt[vti]+1);
        vti++;
      }
      else if (line[1] == 'n') {
        sscanf(line, "vn %f %f %f", obj->vn[vni], obj->vn[vni]+1, obj->vn[vni]+2);
        vni++;
      }
    }
    else if (line[0] == 'f') {
      sscanf(
        line, 
        "f %d/%d/%d %d/%d/%d %d/%d/%d\n", 
        &obj->f[fi][0].v, &obj->f[fi][0].vt, &obj->f[fi][0].vn,
        &obj->f[fi][1].v, &obj->f[fi][1].vt, &obj->f[fi][1].vn,
        &obj->f[fi][2].v, &obj->f[fi][2].vt, &obj->f[fi][2].vn
      );
      fi++;
    }
  }

  fclose(f);
}

// ei is the elemnt index, from 0 to 2
// fi is the face index for both the rg3 and obj
// vi is the vertex index for rg3, essentially the index where the normals vertex offsets, etc is copied from the obj
void vtov(OBJ* obj, RG3* rg3, int ei, int vi, int fi) {
    rg3->f[fi][ei] = vi; // Set the index of the face's elemnt in rg3
    float x = obj->v[obj->f[0][0].v-1][0];
    // Copy the attributes to their respective vi
    memcpy(rg3->v[vi].o, obj->v[obj->f[fi][ei].v-1], sizeof(VEC3));
    // printf("%f\n", obj->v[obj->f[fi][ei].v-1][0]);
    memcpy(rg3->v[vi].n, obj->vn[obj->f[fi][ei].vn-1], sizeof(VEC3));
    memcpy(rg3->v[vi].t, obj->vt[obj->f[fi][ei].vt-1], sizeof(VEC2));
}

void printv(RG3* rg3, int vi) {
  printf("o=%f %f %f; n=%f %f %f; t=%f %f\n", 
  rg3->v[vi].o[0], rg3->v[vi].o[1], rg3->v[vi].o[2],
  rg3->v[vi].n[0], rg3->v[vi].n[1], rg3->v[vi].n[2],
  rg3->v[vi].t[0], rg3->v[vi].t[1]);
}

void objtorg3(OBJ* obj, RG3* rg3) {
  // At first we assume there are no duplicates in the face elements, and assume every single element has a unique v/vt/vn. duplicate removal will come later.
  rg3->v_n = obj->f_n * 3;
  rg3->v = malloc(sizeof(*rg3->v) * rg3->v_n);

  rg3->f_n = obj->f_n;
  rg3->f = malloc(sizeof(*rg3->f) * obj->f_n);

  int vi = 0;
  for (int fi = 0; fi < obj->f_n; fi++) {
    vtov(obj, rg3, 0, vi++, fi);
    printv(rg3, vi-1);
    vtov(obj, rg3, 1, vi++, fi);
    printv(rg3, vi-1);
    vtov(obj, rg3, 2, vi++, fi);
    printv(rg3, vi-1);
  }
}

void saverg3(RG3* rg3, const char* fp) {
  FILE* f = fopen(fp, "wb");

  fwrite(&rg3->v_n, sizeof(rg3->v_n), 1, f);
  fwrite(&rg3->f_n, sizeof(rg3->f_n), 1, f);
  
  fwrite(rg3->v, sizeof(RG3_VERTEX), rg3->v_n, f);
  fwrite(rg3->f, sizeof(INT3), rg3->f_n, f);

  fclose(f);
}

int main(int argsn, char** args) {
  OBJ obj;
  RG3 rg3;
  ftoobj(args[1], &obj);
  objtorg3(&obj, &rg3);
  return 0;
}
