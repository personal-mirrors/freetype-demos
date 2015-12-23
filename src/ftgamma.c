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


  static FTDemo_Display*  display;


  static void
  do_fill( grBitmap*  bitmap,
           int        x,
           int        y,
           int        w,
           int        h,
           int        back,
           int        fore )
  {
    int     pitch = bitmap->pitch;
    int     i;
    double  b, f;

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

    return 0;
  }


  static void
  event_help( void )
  {
    grEvent  dummy_event;


    FTDemo_Display_Clear( display );
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );


    grWriteln( "FreeType Gamma Matcher" );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    grWriteln( "F1, ?       display this help screen" );
    grLn();
    grWriteln( "G           show gamma ramp" );
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_gamma_grid( void )
  {
    grEvent  dummy_event;
    int      g;
    int      yside  = 11;
    int      xside  = 10;
    int      levels = 17;
    int      gammas = 30;
    int      x_0    = ( display->bitmap->width - levels * xside ) / 2;
    int      y_0    = ( display->bitmap->rows - gammas * ( yside + 1 ) ) / 2;
    int      pitch  = display->bitmap->pitch;


    FTDemo_Display_Clear( display );
    grGotobitmap( display->bitmap );

    if ( pitch < 0 )
      pitch = -pitch;

    memset( display->bitmap->buffer,
            100,
            (unsigned int)( pitch * display->bitmap->rows ) );

    grWriteCellString( display->bitmap, 0, 0, "Gamma grid",
                       display->fore_color );


    for ( g = 1; g <= gammas; g++ )
    {
      double  ggamma = 0.1 * g;
      char    temp[6];
      int     y = y_0 + ( yside + 1 ) * ( g - 1 );
      int     nx, ny;

      unsigned char*  line = display->bitmap->buffer +
                             y * display->bitmap->pitch;


      if ( display->bitmap->pitch < 0 )
        line -= display->bitmap->pitch * ( display->bitmap->rows - 1 );

      line += x_0 * 3;

      grSetPixelMargin( x_0 - 32, y + ( yside - 8 ) / 2 );
      grGotoxy( 0, 0 );

      sprintf( temp, "%.1f", ggamma );
      grWrite( temp );

      for ( ny = 0; ny < yside; ny++, line += display->bitmap->pitch )
      {
        unsigned char*  dst = line;


        for ( nx = 0; nx < levels; nx++, dst += 3 * xside )
        {
          double  p   = nx / (double)( levels - 1 );
          int     gm  = (int)( 255.0 * pow( p, ggamma ) + 0.5 );


          memset( dst, gm, (unsigned int)( xside * 3 ) );
        }
      }
    }

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static int
  Process_Event( grEvent*  event )
  {
    int  ret = 0;

    switch ( event->key )
    {
    case grKeyEsc:
    case grKEY( 'q' ):
      ret = 1;
      break;

    case grKeyF1:
    case grKEY( '?' ):
      event_help();
      break;

    case grKEY( 'G' ):
      event_gamma_grid();
      break;

    default:
      break;
    }

    return ret;
  }


  int
  main( void )
  {
    grEvent          event;
    char             buf[4];
    int              i;

    display = FTDemo_Display_New( gr_pixel_mode_rgb24, DIM_X, DIM_Y );
    if ( !display )
    {
      PanicZ( "could not allocate display surface" );
    }

    grSetTitle( display->surface, "FreeType Gamma Matcher - press ? for help" );

    do
    {
      FTDemo_Display_Clear( display );

      Render_GammaGrid( display->bitmap );

      for ( i = 0; i <= 10; i++ )
      {
        sprintf( buf, "%.1f", 1. + .2 * i );
        grWriteCellString( display->bitmap, 9 + i * 60, 395, buf,
                           display->fore_color );
      }

      grRefreshSurface( display->surface );
      grListenSurface( display->surface, 0, &event );
    } while ( Process_Event( &event ) == 0 );

    FTDemo_Display_Done( display );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
