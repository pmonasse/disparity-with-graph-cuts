#ifndef _IO_TIFF_H
#define _IO_TIFF_H

#ifdef __cplusplus
extern "C" {
#endif

#define IO_TIFF_VERSION "0.20110402"

#include <stddef.h>

float *io_tiff_read_f32_gray(const char *fname, size_t *nx, size_t *ny);
int io_tiff_write_f32(const char *fname, const float *data, size_t nx, size_t ny, size_t nc);

#ifdef __cplusplus
}
#endif

#endif /* !_IO_TIFF_H */
