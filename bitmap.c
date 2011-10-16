#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <png.h>

#include "bitmap.h"

bool
create_gs_bitmap(const char *filename, bitmap *bm)
{
    bool ret = false;
    int hdr_len = 8;
    FILE *fp; /* input file pointer */
    png_byte header[hdr_len]; /* input firl header bytes to check it is a png */
    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    png_uint_32 width;
    png_uint_32 height;
    int channels;
    png_structp png_ptr; /* png read context */
    png_infop info_ptr;/* png information before decode */
    png_infop end_info; /* png info after decode */
    png_bytep *row_pointers; /* storage for row pointers */
    int row_loop; /* loop to initialise row pointers */

    fp = fopen(filename, "rb");
    if (!fp) {
        return ret;
    }

    if (fread(header, 1, hdr_len, fp) != hdr_len) {
        fclose(fp);
        return ret;
    }

    if (png_sig_cmp(header, 0, hdr_len) != 0) {
        fclose(fp);
        return ret;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return ret;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)  {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fclose(fp);
        return ret;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        fclose(fp);
        return ret;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return ret;
    }

    png_init_io(png_ptr, fp);

    png_set_sig_bytes(png_ptr, hdr_len);

    png_read_info(png_ptr, info_ptr);


    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, &interlace_method,
                 &compression_method, &filter_method);

    /* set up input filtering */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
        bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);

    png_read_update_info(png_ptr, info_ptr);

    /* check filters result in single 8bit channel */
    channels = png_get_channels(png_ptr, info_ptr);

    if (channels != 1) {
        fclose(fp);
        return ret;
    }
    
    bm->data = malloc(width *height);
    if (bm->data != NULL) {
        row_pointers = malloc(sizeof(png_bytep) * height);
        if (row_pointers != NULL) {
            ret = true;

            bm->width = width;
            bm->height = height;

            for (row_loop = 0; row_loop <height; row_loop++) {
                *(row_pointers + row_loop) = bm->data + (row_loop * width);
            }

            png_read_image(png_ptr, row_pointers);
    
            png_read_end(png_ptr, end_info);

            free(row_pointers);
        } else {
            free(bm->data);
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    fclose(fp);

    return ret;
}