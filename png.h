#ifndef WJ_PNG
#define WJ_PNG

#include <stdlib.h>
#include <png.h>

typedef struct {
	unsigned char *data;
	int w, h;
} raw_t;

raw_t *png_raw_new(unsigned char *_png);
#endif
