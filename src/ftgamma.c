/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2004, 2005, 2012 by                                           */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftgamma - gamma matcher                                                 */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include <math.h>


static void
do_fill( grBitmap*  bitmap,
         int        x,
         int        y,
         int        w,
         int        h,
         int        back,
         int        fore )
{
  int           pitch = bitmap->pitch;
  unsigned int  i;
  double        b, f;

  unsigned char*  line = bitmap->buffer + y*pitch + 3*x;
  unsigned char*  dst;


  if ( back == 0 || back == 255 )
    for ( i = 0; i < w; i++ )
    {
      dst = line + 3 * i + ( i & 1 ) * pitch;
      dst[0] = dst[1] = dst[2] = back;
    }
  else
    for ( b = back / 255., i = 0; i < w; i++ )
    {
      dst = line + 3 * i + ( i & 1 ) * pitch;
      dst[0] = dst[1] = dst[2] = 0.5 +
                                 255. * pow ( b, 1. / (1. + 2. * i / w ) );
    }

  if ( fore == 0 || fore == 255 )
    for ( i = 0; i < w; i++ )
    {
      dst = line + 3 * i + ( ~i & 1 ) * pitch;
      dst[0] = dst[1] = dst[2] = fore;
    }
  else
    for ( f = fore / 255., i = 0; i < w; i++ )
    {
      dst = line + 3 * i + ( ~i & 1 ) * pitch;
      dst[0] = dst[1] = dst[2] = 0.5 +
                                 255. * pow ( f, 1. / (1. + 2. * i / w ) );
    }

  for ( i = 2; i < h; i += 2 )
  {
    memcpy( line + i * pitch, line, 3 * w );
    memcpy( line + i * pitch + pitch, line + pitch, 3 * w );
  }
}


static FT_Error
Render_GammaGrid( grBitmap*  bitmap )
{
  int  x = 20;
  int  y = 90;
  int  h = ( bitmap->rows - 2 * y ) / 15;
  int  w = bitmap->width - 2 * x;

  int     i;
  char    buf[4];
  grColor color = grFindColor( bitmap, 0x00, 0x00, 0x00, 0xff );


  do_fill( bitmap, x,    y, w, h,  85, 255 );
  do_fill( bitmap, x, y+=h, w, h, 170, 170 );
  do_fill( bitmap, x, y+=h, w, h,  85, 255 );
  do_fill( bitmap, x, y+=h, w, h, 170, 170 );
  do_fill( bitmap, x, y+=h, w, h,  85, 255 );

  do_fill( bitmap, x, y+=h, w, h,   0, 255 );
  do_fill( bitmap, x, y+=h, w, h, 127, 127 );
  do_fill( bitmap, x, y+=h, w, h,   0, 255 );
  do_fill( bitmap, x, y+=h, w, h, 127, 127 );
  do_fill( bitmap, x, y+=h, w, h,   0, 255 );

  do_fill( bitmap, x, y+=h, w, h,   0, 170 );
  do_fill( bitmap, x, y+=h, w, h,  85,  85 );
  do_fill( bitmap, x, y+=h, w, h,   0, 170 );
  do_fill( bitmap, x, y+=h, w, h,  85,  85 );
  do_fill( bitmap, x, y+=h, w, h,   0, 170 );

  for ( i = 0; i <= 10; i++ )
  {
    sprintf( buf, "%.1f", 1. + .2 * i );
    grWriteCellString( bitmap, 9 + i * w / 10, 395, buf, color );
  }

  return 0;
}



int
main( void )
{
  FTDemo_Display*  display;
  grEvent          dummy;

  display = FTDemo_Display_New( gr_pixel_mode_rgb24, DIM_X, DIM_Y );
  if ( !display )
  {
    PanicZ( "could not allocate display surface" );
  }

  grSetTitle( display->surface, "FreeType Gamma Matcher" );

  FTDemo_Display_Clear( display );

  Render_GammaGrid( display->bitmap );

  grRefreshSurface( display->surface );
  grListenSurface( display->surface, 0, &dummy );

  exit( 0 );      /* for safety reasons */
  /* return 0; */ /* never reached */
}


/* End */
