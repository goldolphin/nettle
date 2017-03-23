/**
 * @author caofuxiang
 *         2015-07-01 16:40:40.
 */

#ifndef NETTLE_NT_ERROR_H
#define NETTLE_NT_ERROR_H
#include <errno.h>

/**
 * Error definitions.
 */
#ifdef __MACH__
typedef int error_t;
#endif

#ifndef ELAST
#ifdef __ELASTERROR
#define ELAST __ELASTERROR
#else
#define ELAST 100000
#endif
#endif

#define E_FEC_INTERNAL_ERROR (ELAST+1)   // FEC internal error, should be bug.
#define E_FEC_INVALID_HEADER (ELAST+2)
#define E_FEC_DECODER_BUFFER_FULL (ELAST+3)
#define E_API_INVALID_HEADER (ELAST+4)

#endif //NETTLE_NT_ERROR_H
