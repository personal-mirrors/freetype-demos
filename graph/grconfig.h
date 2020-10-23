#ifndef GRCONFIG_H_
#define GRCONFIG_H_

#define GR_MAX_SATURATIONS  8
#define GR_MAX_CONVERSIONS  16

#if defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L

#include <stdint.h>

#ifndef UINT32_MAX
#error  "could not find a 32-bit integer type"
#endif

#else  /* old trick to determine 32-bit integer type */

#include <limits.h>

  /* The number of bytes in an `int' type.  */
#if   UINT_MAX == 0xFFFFFFFFUL
#define GR_SIZEOF_INT  4
#elif UINT_MAX == 0xFFFFU
#define GR_SIZEOF_INT  2
#elif UINT_MAX > 0xFFFFFFFFU && UINT_MAX == 0xFFFFFFFFFFFFFFFFU
#define GR_SIZEOF_INT  8
#else
#error "Unsupported number of bytes in `int' type!"
#endif

  /* The number of bytes in a `long' type.  */
#if   ULONG_MAX == 0xFFFFFFFFUL
#define GR_SIZEOF_LONG  4
#elif ULONG_MAX > 0xFFFFFFFFU && ULONG_MAX == 0xFFFFFFFFFFFFFFFFU
#define GR_SIZEOF_LONG  8
#else
#error "Unsupported number of bytes in `long' type!"
#endif

#if GR_SIZEOF_INT == 4
typedef  int             int32_t;
typedef  unsigned int    uint32_t;
#elif GR_SIZEOF_LONG == 4
typedef  long            int32_t;
typedef  unsigned long   uint32_t;
#else
#error  "could not find a 32-bit integer type"
#endif

#endif  /* old trick to determine 32-bit integer type */

#endif /* GRCONFIG_H_ */
