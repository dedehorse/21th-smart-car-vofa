#ifndef __LQ_JPEG_ENCODER_H
#define __LQ_JPEG_ENCODER_H

#include "lq_include.h"

unsigned int JPEG_Encode_RGB888(unsigned char *rgb_data, int width, int height,
                                unsigned char *jpeg_buf, unsigned int jpeg_buf_size);

#endif
