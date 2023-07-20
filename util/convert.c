// This program converts png files to RGT files and obj files to RGM.

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
} RGM_VERTEX;

typedef struct {
  RGM_VERTEX* v;
  INT3* f;
  int v_n, f_n;
} RGM;

typedef char RGB[3];

typedef struct {
  int w, h;
  RGB* pixels;
} RGT;

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
      else if (line[1] == 'n')
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
// fi is the face index for both the rgm and obj
// vi is the vertex index for rgm, essentially the index where the normals vertex offsets, etc is copied from the obj
void vtov(OBJ* obj, RGM* rgm, int ei, int vi, int fi) {
    rgm->f[fi][ei] = vi; // Set the index of the face's elemnt in rgm
    float x = obj->v[obj->f[0][0].v-1][0];
    // Copy the attributes to their respective vi
    memcpy(rgm->v[vi].o, obj->v[obj->f[fi][ei].v-1], sizeof(VEC3));
    // printf("%f\n", obj->v[obj->f[fi][ei].v-1][0]);
    memcpy(rgm->v[vi].n, obj->vn[obj->f[fi][ei].vn-1], sizeof(VEC3));
    memcpy(rgm->v[vi].t, obj->vt[obj->f[fi][ei].vt-1], sizeof(VEC2));
}

void printv(RGM* rgm, int vi) {
  printf("o=%f %f %f; n=%f %f %f; t=%f %f\n", 
  rgm->v[vi].o[0], rgm->v[vi].o[1], rgm->v[vi].o[2],
  rgm->v[vi].n[0], rgm->v[vi].n[1], rgm->v[vi].n[2],
  rgm->v[vi].t[0], rgm->v[vi].t[1]);
}

void objtorgm(OBJ* obj, RGM* rgm) {
  // At first we assume there are no duplicates in the face elements, and assume every single element has a unique v/vt/vn. duplicate removal will come later.
  rgm->v_n = obj->f_n * 3;
  printf("%d\n", sizeof(*rgm->v) * rgm->v_n);
  fflush(stdout);
  rgm->v = malloc(sizeof(*rgm->v) * rgm->v_n);

  rgm->f_n = obj->f_n;
  rgm->f = malloc(sizeof(*rgm->f) * obj->f_n);

  int vi = 0;
  for (int fi = 0; fi < obj->f_n; fi++) {
    vtov(obj, rgm, 0, vi++, fi);
    printv(rgm, vi-1);
    vtov(obj, rgm, 1, vi++, fi);
    printv(rgm, vi-1);
    vtov(obj, rgm, 2, vi++, fi);
    printv(rgm, vi-1);
  }
}

void pngtorgt(const char* fp, RGT* rgt) {
  FILE *file = fopen(fp, "rb");
  if (!file) {
      fprintf(stderr, "Error opening PNG file %s\n", fp);
      return;
  }

  // Create PNG read struct and info struct
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
      fclose(file);
      fprintf(stderr, "Error creating PNG read struct\n");
      return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      fclose(file);
      fprintf(stderr, "Error creating PNG info struct\n");
      return;
  }

  // Set up error handling
  if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(file);
      fprintf(stderr, "Error during PNG read\n");
      return;
  }

  // Initialize PNG IO
  png_init_io(png_ptr, file);

  // Read header information
  png_read_info(png_ptr, info_ptr);

  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  // Ensure RGB format
  if (color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA) {
      fprintf(stderr, "Input PNG must be in RGB format\n");
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(file);
      return;
  }

  // Set transformations if needed
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png_ptr);
  }
  if (bit_depth == 16) {
      png_set_strip_16(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
      png_set_gray_to_rgb(png_ptr);
  }

  // Allocate memory for the pixels
  rgt->w = width;
  rgt->h = height;
  rgt->pixels = (RGB *)malloc(width * height * sizeof(RGB));

  // Read image data
  png_bytep *row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
  for (int y = 0; y < height; y++) {
      row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
  }
  png_read_image(png_ptr, row_pointers);

  // Convert pixel data to RGB format and store it in RGT structure
  for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
          png_bytep row = row_pointers[y];
          png_bytep px = &(row[x * 3]);
          rgt->pixels[y * width + x][0] = px[0];
          rgt->pixels[y * width + x][1] = px[1];
          rgt->pixels[y * width + x][2] = px[2];
      }
  }

  // Clean up and release resources
  for (int y = 0; y < height; y++) {
      free(row_pointers[y]);
  }
  free(row_pointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(file);
}

void savergt(RGT* rgt, const char* fp) {
  FILE* f = fopen(fp, "wb");

  fwrite(&rgt->w, sizeof(int), 1, f);
  fwrite(&rgt->h, sizeof(int), 1, f);
  
  fwrite(rgt->pixels, sizeof(char), rgt->w*rgt->h, f);

  fclose(f);
}

void savergm(RGM* rgm, const char* fp) {
  FILE* f = fopen(fp, "wb");

  fwrite(&rgm->v_n, sizeof(rgm->v_n), 1, f);
  fwrite(&rgm->f_n, sizeof(rgm->f_n), 1, f);
  
  fwrite(rgm->v, sizeof(RGM_VERTEX), rgm->v_n, f);
  fwrite(rgm->f, sizeof(INT3), rgm->f_n, f);

  fclose(f);
}

int main(int argsn, const char** args) {
  if (argsn < 4) {
    puts("convert <type> <input> <output>\ntypes:\n\tc: gpl -> RGL colors\n\tt: png -> RGL texture\n\tm: obj -> RGL model\n");
    return 1;
  }

    puts("convert <type> <input> <output>\ntypes:\n\tc: gpl -> RGL colors\n\tt: png -> RGL texture\n\tm: obj -> RGL model\n");
  malloc(7680);
    puts("convert <type> <input> <output>\ntypes:\n\tc: gpl -> RGL colors\n\tt: png -> RGL texture\n\tm: obj -> RGL model\n");

  char type = args[1][0];
  const char* input = args[2];
  const char* output = args[3];

  switch(type) {
    case 't':
    RGT rgt;
    pngtorgt(input, &rgt);
    savergt(&rgt, output);
    break;

    case 'm':
    OBJ obj;
    RGM rgm;
    loadobj(input, &obj);
    objtorgm(&obj, &rgm);
    savergm(&rgm, output);
    break;
  }

  return 0;
}
