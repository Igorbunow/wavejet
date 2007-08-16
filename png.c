#include "png.h"

typedef struct {
	unsigned char *data;
	int offset;
} png_t;

void png_read(png_structp _pngptr, png_bytep _data, png_size_t _len)
{
	png_t *png;

	/* Get input */
	png = png_get_io_ptr(_pngptr);

	/* Copy data from input */
	memcpy(_data, png->data + png->offset, _len);
	png->offset += _len;
}

raw_t *png_raw_new(unsigned char *_pngdata)
{
	png_structp pngptr;
	png_infop pnginfo;
	unsigned long width, height;
	png_t png;
	raw_t *raw;
	png_bytepp rows;
	int i;

	png.data = _pngdata;
	png.offset = 0;
	
	pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	pnginfo = png_create_info_struct(pngptr);
	png_set_read_fn(pngptr, &png, png_read);
	png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
	rows = png_get_rows(pngptr, pnginfo);

	width = png_get_image_width(pngptr, pnginfo);
	height = png_get_image_height(pngptr, pnginfo);

	raw = malloc(sizeof(raw_t));
	raw->w = width;
	raw->h = height;

	raw->data = malloc(width * height * 4);
	for (i = 0; i < height; i++) {
		memcpy(raw->data + i * width * 4, rows[i], width * 4);
	}

	return raw;
}
