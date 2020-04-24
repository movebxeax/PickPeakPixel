#include <stdio.h>
#include <memory>

#include "libjpeg-turbo/jpeglib.h"

#pragma warning(disable:4996)

#ifdef _M_X64
#ifdef _DEBUG
#pragma comment(lib, "lib/jpeg-static_x64d.lib")
#else
#pragma comment(lib, "lib/jpeg-static_x64.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib,"lib/jpeg-static_x86d.lib")
#else
#pragma comment(lib,"lib/jpeg-static_x86.lib")
#endif
#endif

typedef jpeg_decompress_struct JPEG_DECOMPRESS_STRUCT, *PJPEG_DECOMPRESS_STRUCT;
typedef jpeg_compress_struct JPEG_COMPRESS_STRUCT, *PJPEG_COMPRESS_STRUCT;

int read_jpeg(PJPEG_DECOMPRESS_STRUCT cinfo, unsigned char **buffer, wchar_t *path)
{
	FILE *ifp;
	JSAMPARRAY jsamp_array;
	int img_ptr = 0;

	if(!(ifp = _wfopen(path, L"rb")))
		return 0;

	struct jpeg_error_mgr jerr;

	cinfo->err = jpeg_std_error(&jerr);

	jpeg_create_decompress(cinfo);

	jpeg_stdio_src(cinfo, ifp);
	jpeg_read_header(cinfo, TRUE);
	jpeg_start_decompress(cinfo);

	int row_stride = cinfo->output_width * cinfo->output_components;
	int image_size = cinfo->output_width * cinfo->output_height * cinfo->output_components;
	unsigned char *image = (unsigned char *)malloc(image_size+1);
	*buffer = image;

	if(!image)
		return NULL;

	jsamp_array = (cinfo->mem->alloc_sarray)((j_common_ptr)cinfo, JPOOL_IMAGE, row_stride, 1);

	while(cinfo->output_scanline < cinfo->output_height)
	{
		jpeg_read_scanlines(cinfo, jsamp_array, 1);

		memcpy(image + img_ptr, jsamp_array[0], cinfo->output_width*cinfo->num_components);
		img_ptr += cinfo->output_width*cinfo->num_components;
	}

	jpeg_finish_decompress(cinfo);
	jpeg_destroy_decompress(cinfo);

	fclose(ifp);

	return 1;
}

int write_jpeg(unsigned char *buffer, int width, int height, int quality, wchar_t *path)
{
	FILE *ofp;

	if(!(ofp = _wfopen(path, L"wb")))
		return 0;

	JPEG_COMPRESS_STRUCT cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, ofp);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_ptr;
	while(cinfo.next_scanline < cinfo.image_height)
	{
		row_ptr = (JSAMPROW)buffer + (cinfo.next_scanline * cinfo.image_width * cinfo.input_components);
		jpeg_write_scanlines(&cinfo, &row_ptr, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(ofp);

	return 1;
}

void
PickPeakPixel(unsigned char *image, unsigned int image_size, unsigned char threshold, bool inverse)
{
#pragma pack(1)
	typedef struct _BGR
	{
		unsigned char b;
		unsigned char g;
		unsigned char r;
	} BGR, *PBGR;
#pragma pack()

	const unsigned long pixel_count = image_size;
	PBGR pixel_ptr = (PBGR)image;


	for(pixel_ptr; pixel_ptr < (PBGR)image + pixel_count; pixel_ptr++)
	{
		if(pixel_ptr->r < threshold || pixel_ptr->g < threshold || pixel_ptr->b < threshold)
		{
			pixel_ptr->r = 0;
			pixel_ptr->g = 0;
			pixel_ptr->b = 0;
		}

		if(inverse)
		{
			pixel_ptr->r = ~pixel_ptr->r;
			pixel_ptr->g = ~pixel_ptr->g;
			pixel_ptr->b = ~pixel_ptr->b;
		}
	}
}

int wmain(int argc, wchar_t *argv[])
{
	if(argc < 5)
	{
		printf("%ls input.jpg output.jpg threshold[0-255] quality[0-100] inverse[0,1]\n", argv[0]);
		return -1;
	}

	wchar_t *input_path = argv[1];
	wchar_t *output_path = argv[2];
	int threshold = _wtoi(argv[3]);
	int quality = _wtoi(argv[4]);
	bool inverse = _wtoi(argv[5]);

	unsigned char *buffer = NULL;
	JPEG_DECOMPRESS_STRUCT decomp_info;

	if(!read_jpeg(&decomp_info, &buffer, input_path))
	{
		printf("Error occured while reading image\n");
		return -1;
	}

	const int width = decomp_info.output_width;
	const int height = decomp_info.output_height;
	const int image_size = width*height;

	if(threshold < 0 || threshold > 0xff)
	{
		printf("threshold must be in range [0-255]\n");
		return -1;
	}

	PickPeakPixel(buffer, image_size, threshold, inverse);

	if(threshold < 0 || threshold > 0xff)
	{
		printf("quality must be in range [0-100]\n");
		return -1;
	}

	if(!write_jpeg(buffer, width, height, quality, output_path))
	{
		printf("Error occured while writing image\n");
		return -1;
	}

	return 0;
}