
#include <freetype/ftmodapi.h>

#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <time.h>

  typedef FT_Vector  Vec2;
  typedef FT_BBox    Box;

  #define FT_CALL(X)\
    error = X;\
    if (error != FT_Err_Ok) {\
        printf("FreeType error: %s [LINE: %d, FILE: %s]\n",\
          FT_Error_String(error), __LINE__, __FILE__);\
        goto Exit;\
    }

  typedef struct  Status_
  {
    FT_Face   face;

    FT_Int    ptsize;

    FT_Int    glyph_index;

    FT_Int    scale;

    FT_Int    spread;

    FT_Int    x_offset;

    FT_Int    y_offset;

    FT_Bool   nearest_filtering;

    float     generation_time;

    FT_Bool   reconstruct;

    FT_Bool   use_bitmap;

    FT_Bool   overlaps;

    /* params for reconstruction */

    float     width;
    float     edge;

  } Status;

  static FTDemo_Handle*   handle   = NULL;
  static FTDemo_Display*  display  = NULL;

  static Status status = { 
    /* face              */ NULL,
    /* ptsize            */ 256,
    /* glyph_index       */ 0,
    /* scale             */ 1,
    /* spread            */ 4,
    /* x_offset          */ 0,
    /* y_offset          */ 0,
    /* nearest_filtering */ 0,
    /* generation_time   */ 0.0f,
    /* reconstruct       */ 0,
    /* use_bitmap        */ 0,
    /* overlaps          */ 0,
    /* width             */ 0.0f,
    /* edge              */ 0.2f
  };

  static FT_Error
  event_font_update()
  {
    /* This event is triggered when the prooperties of the rasterizer */
    /* is updated or the glyph index / size is updated.               */
    FT_Error  error = FT_Err_Ok;
    clock_t   start, end;

    /* Set various properties of the renderers. */
    FT_CALL( FT_Property_Set( handle->library, "bsdf", "spread", &status.spread ) );
    FT_CALL( FT_Property_Set( handle->library, "sdf", "spread", &status.spread ) );
    FT_CALL( FT_Property_Set( handle->library, "sdf", "overlaps", &status.overlaps ) );

    /* Set pixel size and load the glyph index. */
    FT_CALL( FT_Set_Pixel_Sizes( status.face, 0, status.ptsize ) );
    FT_CALL( FT_Load_Glyph( status.face, status.glyph_index, FT_LOAD_DEFAULT ) );

    /* This is just to measure the generation time. */
    start = clock();

    /* Finally render the glyph. To force the `bsdf' renderer (i.e. to */
    /* generate SDF from bitmap) we must render the glyph first using  */
    /* the smooth or the monochrome FreeType rasterizer.               */
    if ( status.use_bitmap )
      FT_CALL( FT_Render_Glyph( status.face->glyph, FT_RENDER_MODE_NORMAL ) );
    FT_CALL( FT_Render_Glyph( status.face->glyph, FT_RENDER_MODE_SDF ) );

    /* Compute and print the generation time. */
    end = clock();

    status.generation_time = ( (float)( end - start ) / (float)CLOCKS_PER_SEC ) * 1000.0f;

    printf( "Generation Time: %.0f ms\n", status.generation_time );

  Exit:
    return error;
  }

  static void
  event_color_change()
  {
    /* This event is triggered when we create a new display, */
    /* in this event we set various colors for the display   */ 
    /* buffer.                                               */
    display->back_color = grFindColor( display->bitmap,  0,  0,  0, 0xff );
    display->fore_color = grFindColor( display->bitmap, 255, 255, 255, 0xff );
    display->warn_color = grFindColor( display->bitmap,  0, 255, 255, 0xff );
  }

  static void
  event_help()
  {
    /* This event is triggered when the user presses the help */
    /* screen button that is either '?' or F1. It basically   */
    /* prints the list of keys along with thier usage to the  */
    /* display window.                                        */

    grEvent  dummy;

    /* For help screen we use a slightly gray color instead of */
    /* completely black background.                            */
    display->back_color = grFindColor( display->bitmap, 30, 30, 30, 0xff );
    FTDemo_Display_Clear( display );
    display->back_color = grFindColor( display->bitmap,  0,  0,  0, 0xff );

    /* Set some properties. */
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );

    /* Set the text color. (kind of purple) */
    grGotobitmapColor( display->bitmap, 204, 153, 204, 255 );

    /* Print the keys and usage. */
    grWriteln( "Signed Distnace Field Viewer" );
    grLn();
    grWriteln( "Use the following keys:" );
    grWriteln( "-----------------------" );
    grLn();
    grWriteln( "  F1 or ? or /       : display this help screen" );
    grLn();
    grWriteln( "  b                  : Toggle between bitmap/outline to be used for generating" );
    grLn();
    grWriteln( "  z, x               : Zoom/Scale Up and Down" );
    grLn();
    grWriteln( "  Up, Down Arrow     : Adjust glyph's point size by 1" );
    grWriteln( "  PgUp, PgDn         : Adjust glyph's point size by 25" );
    grLn();
    grWriteln( "  Left, Right Arrow  : Adjust glyph index by 1" );
    grWriteln( "  F5, F6             : Adjust glyph index by 50" );
    grWriteln( "  F7, F8             : Adjust glyph index by 500" );
    grLn();
    grWriteln( "  o, l               : Adjust spread size by 1" );
    grLn();
    grWriteln( "  w, s               : Move glyph Up/Down" );
    grWriteln( "  a, d               : Move glyph Left/right" );
    grLn();
    grWriteln( "  f                  : Toggle between bilinear/nearest filtering" );
    grLn();
    grWriteln( "  m                  : Toggle overlapping support" );
    grLn();
    grWriteln( "Reconstructing Image from SDF" );
    grWriteln( "-----------------------------" );
    grWriteln( "  r                  : Toggle between reconstruction/raw view" );
    grWriteln( "  i, k               : Adjust width by 1 (makes the text bolder/thinner)" );
    grWriteln( "  u, j               : Adjust edge by 1 (makes the text smoother/sharper)" );
    grLn();
    grWriteln( "press any key to exit this help screen" );

    /* Now wait till any key press, otherwise the help screen */
    /* will only blink and disappear.                         */
    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy );
  }

  static void
  write_header()
  {
    /* This function simply prints some information to the top */
    /* left of the screen. The information contains various    */
    /* properties and values etc.                              */

    static char   header_string[512];

    sprintf( header_string, "Glyph Index: %d, Pt Size: %d, Spread: %d, Scale: %d",
             status.glyph_index, status.ptsize, status.spread, status.scale );
    grWriteCellString( display->bitmap, 0, 0, header_string, display->fore_color );

    sprintf( header_string, "Position Offset: %d,%d", status.x_offset, status.y_offset );
    grWriteCellString( display->bitmap, 0, 1 * HEADER_HEIGHT, header_string, display->fore_color );

    sprintf( header_string, "SDF Generated in: %.0f ms, From: %s", status.generation_time,
             status.use_bitmap ? "Bitmap" : "Outline" );
    grWriteCellString( display->bitmap, 0, 2 * HEADER_HEIGHT, header_string, display->fore_color );

    sprintf( header_string, "Filtering: %s, View: %s", status.nearest_filtering ? "Nearest" : "Bilinear",
                                                       status.reconstruct ? "Reconstructing": "Raw" );
    grWriteCellString( display->bitmap, 0, 3 * HEADER_HEIGHT, header_string, display->fore_color );

    if ( status.reconstruct )
    {
      /* Only print these in reconstruction mode. */
      sprintf( header_string, "Width: %.2f, Edge: %.2f", status.width, status.edge );
      grWriteCellString( display->bitmap, 0, 4 * HEADER_HEIGHT, header_string, display->fore_color );
    }
  }


/* END */
