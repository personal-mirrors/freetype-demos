
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

  static int
  Process_Event()
  {
    /* The function listens to the events on the display */
    /* and processes them if any event is received.      */

    grEvent  event;
    int      ret = 0;
    int      speed = 10 * status.scale;

    grListenSurface( display->surface, 0, &event );

    switch (event.key) {
    case grKEY( 'q' ):
    case grKeyEsc:
      ret = 1;
      break;
    case grKEY( 'z' ):
      status.scale++;
      break;
    case grKEY( 'x' ):
      status.scale--;
      if ( status.scale < 1 )
        status.scale = 1;
      break;
    case grKeyPageUp:
      status.ptsize += 24;
    case grKeyUp:
      status.ptsize++;
      if ( status.ptsize > 512 )
        status.ptsize = 512;
      event_font_update();
      break;
    case grKeyPageDown:
      status.ptsize -= 24;
    case grKeyDown:
      status.ptsize--;
      if ( status.ptsize < 8 )
        status.ptsize = 8;
      event_font_update();
      break;
    case grKEY( 'o' ):
      status.spread++;
      if ( status.spread > 32 )
        status.spread = 32;
      event_font_update();
      break;
    case grKEY( 'l' ):
      status.spread--;
      if ( status.spread < 2 )
        status.spread = 2;
      event_font_update();
      break;
    case grKeyF8:
      status.glyph_index += 450;
    case grKeyF6:
      status.glyph_index += 49;
    case grKeyRight:
      status.glyph_index++;
      event_font_update();
      break;
    case grKeyF7:
      status.glyph_index -= 450;
    case grKeyF5:
      status.glyph_index -= 49;
    case grKeyLeft:
      status.glyph_index--;
      if ( status.glyph_index < 0 )
        status.glyph_index = 0;
      event_font_update();
      break;
    case grKEY( 'b' ):
      status.use_bitmap = !status.use_bitmap;
      event_font_update();
      break;
    case grKEY( 'f' ):
      status.nearest_filtering = !status.nearest_filtering;
      break;
    case grKEY( 'r' ):
      status.reconstruct = !status.reconstruct;
      break;
    case grKEY( 'i' ):
      status.width += 0.5f;
      break;
    case grKEY( 'k' ):
      status.width -= 0.5f;
      break;
    case grKEY( 'u' ):
      status.edge += 0.2f;
      break;
    case grKEY( 'j' ):
      status.edge -= 0.2f;
      break;
    case grKEY( 'd' ):
      status.x_offset += speed;
      break;
    case grKEY( 'a' ):
      status.x_offset -= speed;
      break;
    case grKEY( 's' ):
      status.y_offset -= speed;
      break;
    case grKEY( 'w' ):
      status.y_offset += speed;
      break;
    case grKEY( 'm' ):
      status.overlaps = !status.overlaps;
      event_font_update();
      break;
    case grKEY( '?' ):
    case grKEY( '/' ):
    case grKeyF1:
      event_help();
      break;
    default:
        break;
    }

    return ret;
  }

  float
  clamp(float x, float lower_limit, float upper_limit) {
    /* The function clamps the value `x' between `lowerlimit' */
    /* and `upperlimit'.                                      */
    if (x < lower_limit)
      x = lower_limit;
    if (x > upper_limit)
      x = upper_limit;
    return x;
  }

  float
  smoothstep(float edge0, float edge1, float x) {
    /* The function interpolate the value of `x' between */
    /* `edge0' and `edge1' smoothly using a polynomial   */
    /* function.                                         */

    /* This implementation is taken from Wikipedia.      */
    /* https://en.wikipedia.org/wiki/Smoothstep          */

    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); 
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
  }

  static FT_Error
  draw()
  {
    /* The function draws a SDF image to the display. */

    FT_Bitmap*  bitmap = &status.face->glyph->bitmap;
    Box         draw_region;
    Box         sample_region;
    Vec2        center;
    FT_Short*   buffer;


    if ( !bitmap || !bitmap->buffer )
      return FT_Err_Invalid_Argument;

    /* compute the center of the display */
    center.x = display->bitmap->width / 2;
    center.y = display->bitmap->rows  / 2;

    /* compute the draw reigon around the `center' */
    draw_region.xMin = center.x - ( bitmap->width * status.scale) / 2;
    draw_region.xMax = center.x + ( bitmap->width * status.scale) / 2;
    draw_region.yMin = center.y - ( bitmap->rows  * status.scale) / 2;
    draw_region.yMax = center.y + ( bitmap->rows  * status.scale) / 2;

    /* add the position offset so that we can move the image */
    draw_region.xMin += status.x_offset;
    draw_region.xMax += status.x_offset;
    draw_region.yMin += status.y_offset;
    draw_region.yMax += status.y_offset;

    /* Sample region is the region of the bitmap that will */
    /* be sampled to the display buffer.                   */
    sample_region.xMin = 0;
    sample_region.xMax = bitmap->width * status.scale;
    sample_region.yMin = 0;
    sample_region.yMax = bitmap->rows * status.scale;

    /* Adjust the sample region in case our draw region */
    /* goes outiside the dislpay.                       */

    /* adjust in -y */
    if ( draw_region.yMin < 0 )
    {
      sample_region.yMax -= draw_region.yMin;
      draw_region.yMin = 0;
    }

    /* adjust in +y */
    if ( draw_region.yMax > display->bitmap->rows )
    {
      sample_region.yMin += draw_region.yMax - display->bitmap->rows;
      draw_region.yMax = display->bitmap->rows;
    }

    /* adjust in -x */
    if ( draw_region.xMin < 0 )
    {
      sample_region.xMin -= draw_region.xMin;
      draw_region.xMin = 0;
    }

    /* adjust in +x */
    if ( draw_region.xMax > display->bitmap->width )
    {
      sample_region.xMax += draw_region.xMax - display->bitmap->width;
      draw_region.xMax = display->bitmap->width;
    }

    buffer = (FT_Short*)bitmap->buffer;

    /* Finally loop the pixels inside the draw region and copy the */
    /* pixel from the sample region to the draw region.            */
    for ( FT_Int j = draw_region.yMax - 1, y = sample_region.yMin; j >= draw_region.yMin; j--, y++ )
    {
      for ( FT_Int i = draw_region.xMin, x = sample_region.xMin; i < draw_region.xMax; i++, x++ )
      {
        FT_UInt   display_index = j * display->bitmap->width + i;
        float     min_dist;

        if ( status.nearest_filtering )
        {
          FT_UInt   bitmap_index  = ( y / status.scale ) * bitmap->width + ( x / status.scale );
          FT_Short  pixel_value   = buffer[bitmap_index];
          
          /* If nearest filtering then simply take the value of the */
          /* nearest sampling pixel.                                */
          min_dist = (float)pixel_value / 1024.0f;
        }
        else
        {
          /* for simplicity use floats */
          float  bi_x;
          float  bi_y;

          float  nbi_x;
          float  nbi_y;

          int    indc[4]; /* [0,0] [0,1] [1,0] [1,1] */
          float  dist[4];

          float  m1, m2;

          /* If bilinear filtering then compute the bilinear */
          /* interpolation of the current draw pixel using   */
          /* the nearby sampling pixel values.               */

          /* Again the concept is taken from Wikipedia.      */
          /* https://en.wikipedia.org/wiki/Bilinear_interpolation */

          bi_x = (float)x / (float)status.scale;
          bi_y = (float)y / (float)status.scale;

          nbi_x = bi_x - (int)bi_x;
          nbi_y = bi_y - (int)bi_y;

          indc[0] = (int)bi_y * bitmap->width + (int)bi_x;
          indc[1] = ( (int)bi_y + 1 ) * bitmap->width + (int)bi_x;
          indc[2] = (int)bi_y * bitmap->width + (int)bi_x + 1;
          indc[3] = ( (int)bi_y + 1 ) * bitmap->width + (int)bi_x + 1;

          dist[0] = (float)buffer[indc[0]] / 1024.0f;

          if ( indc[1] >= bitmap->width * bitmap->rows )
            dist[1] = -status.spread;
          else
            dist[1] = (float)buffer[indc[1]] / 1024.0f;

          if ( indc[2] >= bitmap->width * bitmap->rows )
            dist[2] = -status.spread;
          else
            dist[2] = (float)buffer[indc[2]] / 1024.0f;

          if ( indc[3] >= bitmap->width * bitmap->rows )
            dist[3] = -status.spread;
          else
            dist[3] = (float)buffer[indc[3]] / 1024.0f;

          m1 = dist[0] * ( 1.0f - nbi_y ) + dist[1] * nbi_y;
          m2 = dist[2] * ( 1.0f - nbi_y ) + dist[3] * nbi_y;

          /* This is our final display after bilinear interpolation. */
          min_dist = ( 1.0f - nbi_x ) * m1 +
                     ( nbi_x ) * m2;
        }


        if ( status.reconstruct )
        {
          float alpha;


          /* If we are reconstructing then discard the values outside the */
          /* range defined by `status.width' and use the `status.edge' to */
          /* make smooth anti-aliased edges.                              */

          /* This is exactly similar to a OpenGL implementation to draw SDF. */
          alpha  = 1.0f - smoothstep( status.width, status.width + status.edge, -min_dist );
          alpha *= 255;

          /* finally copy the target value to the display buffer */
          display_index *= 3;
          display->bitmap->buffer[display_index + 0] = (unsigned char)alpha;
          display->bitmap->buffer[display_index + 1] = (unsigned char)alpha;
          display->bitmap->buffer[display_index + 2] = (unsigned char)alpha;
        }
        else
        {
          float final_dist = min_dist;


          /* If not reconstructing then normalize the values between [0, 255] */
          /* and copy to the display buffer.                                  */

          /* normalize using `status.spread' */
          final_dist = final_dist < 0 ? -final_dist : final_dist;
          final_dist /= (float)status.spread;

          /* invert the values */
          final_dist = 1.0f - final_dist;
          final_dist *= 255;

          /* finally copy the target value to the display buffer */
          display_index *= 3;
          display->bitmap->buffer[display_index + 0] = (unsigned char)final_dist;
          display->bitmap->buffer[display_index + 1] = (unsigned char)final_dist;
          display->bitmap->buffer[display_index + 2] = (unsigned char)final_dist;
        }
      }
    }

    return FT_Err_Ok;
  }

  int
  main( int     argc,
        char**  argv )
  {
    FT_Error  error = FT_Err_Ok;


    if ( argc != 3 )
    {
      printf( "Usage: [ptsize] [font file]\n" );
      exit( -1 );
    }

    status.ptsize = atoi( argv[1] );
    handle = FTDemo_New();

    if ( !handle )
    {
      printf( "Failed to create FTDemo_Handle\n" );
      goto Exit;
    }

    display = FTDemo_Display_New( NULL, "800x600" );

    if ( !display )
    {
      printf( "Failed to create FTDemo_Display\n" );
      goto Exit;
    }

#ifdef __linux__
    int flip_y = 1;
    FT_CALL( FT_Property_Set( handle->library, "sdf", "flip_y", &flip_y ) );
    FT_CALL( FT_Property_Set( handle->library, "bsdf", "flip_y", &flip_y ) );
#endif

    grSetTitle( display->surface, "Signed Distance Field Viewer" );
    event_color_change();

    FT_CALL( FT_New_Face( handle->library, argv[2], 0, &status.face ) );
    FT_CALL( event_font_update() );

    do 
    {
      FTDemo_Display_Clear( display );

      draw();

      write_header();

      grRefreshSurface( display->surface );
    } while ( !Process_Event() );

  Exit:
    if ( status.face )
      FT_Done_Face( status.face );
    if ( display )
      FTDemo_Display_Done( display );
    if ( handle )
      FTDemo_Done( handle );
    exit( error );
  }

/* END */
