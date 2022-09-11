/* some utility functions */

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


  const char*
  ft_basename( const char*  name )
  {
    const char*  base;
    const char*  current;
    char         c;

    base    = name;
    current = name;

    c = *current;

    while ( c )
    {
#ifndef macintosh
      if ( c == '/' || c == '\\' )
#else
      if ( c == ':' )
#endif
        base = current + 1;

      current++;
      c = *current;
    }

    return base;
  }


  char*
  ft_strdup( const char*  str )
  {
    char*   result;
    size_t  len;


    if ( !str )
      return NULL;

    len    = strlen( str ) + 1;
    result = (char *)malloc( len );
    if ( result )
      memcpy( result, str, len );

    return result;
  }


  void
  Panic( const char*  fmt,
         ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );

    exit( 1 );
  }


  extern int
  utf8_next( const char**  pcursor,
             const char*   end )
  {
    const unsigned char*  p = (const unsigned char*)*pcursor;
    int                   ch;
    int                   mask = 0x80;  /* the first decision bit */


    if ( (const char*)p >= end || ( *p & 0xc0 ) == 0x80 )
      goto BAD_DATA;

    ch = *p++;

    if ( ch & mask )
    {
      mask = 0x40;

      do
      {
        if ( (const char*)p >= end || ( *p & 0xc0 ) != 0x80 )
          goto BAD_DATA;

        ch     = ( ch << 6 ) | ( *p++ & 0x3f );
        mask <<= 5;  /* the next decision bit after shift */
      } while ( ch & mask && mask <= 0x200000 );
    }

    *pcursor = (const char*)p;

    return ch & ( mask - 1 );  /* dropping the decision bits */

  BAD_DATA:
    return -1;
  }

/* End */
