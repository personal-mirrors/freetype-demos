/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 1996-2023 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTMulti- a simple multiple masters font viewer                          */
/*                                                                          */
/*  Press ? when running this program to have a list of key-bindings        */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include <freetype/freetype.h>

#include <freetype/ftdriver.h>
#include <freetype/ftfntfmt.h>
#include <freetype/ftmm.h>
#include <freetype/ftmodapi.h>

#include "common.h"
#include "mlgetopt.h"
#include "strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "graph.h"
#include "grfont.h"

#define  DIM_X   640
#define  DIM_Y   480

#define  HEADER_HEIGHT  12

#define  MAXPTSIZE    500               /* dtp */
#define  MAX_MM_AXES   16

  /* definitions in ftcommon.c */
  unsigned int
  FTDemo_Event_Cff_Hinting_Engine_Change( FT_Library     library,
                                          unsigned int*  current,
                                          unsigned int   delta );
  unsigned int
  FTDemo_Event_Type1_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta );
  unsigned int
  FTDemo_Event_T1cid_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta );


  static char         Header[256];
  static const char*  new_header = NULL;

  static const unsigned char*  Text = (unsigned char*)
    "The quick brown fox jumps over the lazy dog 0123456789 "
    "\342\352\356\373\364\344\353\357\366\374\377\340\371\351\350\347 "
    "&#~\"\'(-`_^@)=+\260 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
    "$\243^\250*\265\371%!\247:/;.,?<>";

  static FT_Library    library;      /* the FreeType library        */
  static FT_Face       face;         /* the font face               */
  static FT_Size       size;         /* the font size               */
  static FT_GlyphSlot  glyph;        /* the glyph slot              */

  static unsigned long  encoding = FT_ENCODING_NONE;

  static unsigned int  cff_hinting_engine;
  static unsigned int  type1_hinting_engine;
  static unsigned int  t1cid_hinting_engine;
  static unsigned int  tt_interpreter_versions[3];
  static unsigned int  num_tt_interpreter_versions;
  static unsigned int  tt_interpreter_version_idx;

  static const char*  font_format;

  static FT_Error      error;        /* error returned by FreeType? */

  static grSurface*    surface;      /* current display surface     */
  static grBitmap*     bit;          /* current display bitmap      */

  static unsigned short  width  = DIM_X;     /* window width        */
  static unsigned short  height = DIM_Y;     /* window height       */

  static int  num_glyphs;            /* number of glyphs            */
  static int  ptsize;                /* current point size          */

  static int  hinted    = 1;         /* is glyph hinting active?    */
  static int  grouping  = 1;         /* is axis grouping active?    */
  static int  antialias = 1;         /* is anti-aliasing active?    */
  static int  fillrule  = 0x0;       /* flip fill flags or not?     */
  static int  overlaps  = 0x0;       /* flip overlap flags or not?  */
  static int  Num;                   /* current first glyph index   */

  static int  res       = 72;

  static grColor  fore_color;

  static int  Fail;

  static int  render_mode = 1;

  static FT_MM_Var    *multimaster   = NULL;
  static FT_Fixed      design_pos   [MAX_MM_AXES];
  static FT_Fixed      requested_pos[MAX_MM_AXES];
  static unsigned int  requested_cnt =  0;
  static unsigned int  used_num_axis =  0;
  static double        increment     = 0.025;  /* for axes */

  /*
   * We use the following arrays to support both the display of all axes and
   * the grouping of axes.  If grouping is active, hidden axes that have the
   * same tag as a non-hidden axis are not displayed; instead, they receive
   * the same axis value as the non-hidden one.
   */
  static unsigned int  hidden[MAX_MM_AXES];
  static int           shown_axes[MAX_MM_AXES];  /* array of axis indices */
  static unsigned int  num_shown_axes;


#define DEBUGxxx

#ifdef DEBUG
#define LOG( x )  LogMessage x
#else
#define LOG( x )  /* empty */
#endif


#ifdef DEBUG
  static void
  LogMessage( const char*  fmt,
              ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }
#endif


  /* PanicZ */
  static void
  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  static unsigned long
  make_tag( char  *s )
  {
    int            i;
    unsigned long  l = 0;


    for ( i = 0; i < 4; i++ )
    {
      if ( !s[i] )
        break;
      l <<= 8;
      l  += (unsigned long)s[i];
    }

    return l;
  }


  static void
  parse_design_coords( char  *s )
  {
    for ( requested_cnt = 0;
          requested_cnt < MAX_MM_AXES && *s;
          requested_cnt++ )
    {
      requested_pos[requested_cnt] = (FT_Fixed)( strtod( s, &s ) * 65536.0 );

      while ( *s==' ' )
        ++s;
    }
  }


  static void
  set_up_axes( void )
  {
    if ( grouping )
    {
      int  i, j, idx;


      /*
       * `ftmulti' is a diagnostic tool that should be able to handle
       * pathological situations also; for this reason the looping code
       * below is a bit more complicated in comparison to normal
       * applications.
       *
       * In particular, the loop handles the following cases gracefully,
       * avoiding grouping.
       *
       * . multiple non-hidden axes have the same tag
       *
       * . multiple hidden axes have the same tag without a corresponding
       *   non-hidden axis
       */

      idx = -1;
      for ( i = 0; i < (int)used_num_axis; i++ )
      {
        int            do_skip;
        unsigned long  tag = multimaster->axis[i].tag;


        do_skip = 0;
        if ( hidden[i] )
        {
          /* if axis is hidden, check whether an already assigned */
          /* non-hidden axis has the same tag; if yes, skip it    */
          for ( j = 0; j <= idx; j++ )
            if ( !hidden[shown_axes[j]]                      &&
                 multimaster->axis[shown_axes[j]].tag == tag )
            {
              do_skip = 1;
              break;
            }
        }
        else
        {
          /* otherwise check whether we have already assigned this axis */
          for ( j = 0; j <= idx; j++ )
            if ( shown_axes[j] == i )
            {
              do_skip = 1;
              break;
            }
        }
        if ( do_skip )
          continue;

        /* we have a new axis to display */
        shown_axes[++idx] = i;

        /* if axis is hidden, use a non-hidden axis */
        /* with the same tag instead if available   */
        if ( hidden[i] )
        {
          for ( j = i + 1; j < (int)used_num_axis; j++ )
            if ( !hidden[j]                      &&
                 multimaster->axis[j].tag == tag )
              shown_axes[idx] = j;
        }
      }

      num_shown_axes = (unsigned int)( idx + 1 );
    }
    else
    {
      unsigned int  i;


      /* show all axes */
      for ( i = 0; i < used_num_axis; i++ )
        shown_axes[i] = (int)i;

      num_shown_axes = used_num_axis;
    }
  }


  /* Clear `bit' bitmap/pixmap */
  static void
  Clear_Display( void )
  {
    /* fast black background */
    memset( bit->buffer, 0, (size_t)bit->rows *
                            (size_t)( bit->pitch < 0 ? -bit->pitch
                                                     : bit->pitch ) );
  }


  /* Initialize the display bitmap named `bit' */
  static void
  Init_Display( void )
  {
    grBitmap  bitmap = { (int)height, (int)width, 0,
                         gr_pixel_mode_none, 256, NULL };


    grInitDevices();

    surface = grNewSurface( 0, &bitmap );
    if ( !surface )
      PanicZ( "could not allocate display surface\n" );

    bit = (grBitmap*)surface;

    fore_color = grFindColor( bit, 255, 255, 255, 255 );  /* white */
  }


  /* Render a single glyph with the `grays' component */
  static FT_Error
  Render_Glyph( int  x_offset,
                int  y_offset )
  {
    grBitmap  bit3;
    FT_Pos    x_top, y_top;


    /* first, render the glyph image into a bitmap */
    if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
    {
      /* overlap flags mitigate AA rendering artifacts in overlaps */
      /* by oversampling; even-odd fill rule reveals the overlaps; */
      /* toggle these flag to test the effects                     */
      glyph->outline.flags ^= overlaps | fillrule;

      error = FT_Render_Glyph( glyph, antialias ? FT_RENDER_MODE_NORMAL
                                                : FT_RENDER_MODE_MONO );
      if ( error )
        return error;
    }

    /* now blit it to our display screen */
    bit3.rows   = (int)glyph->bitmap.rows;
    bit3.width  = (int)glyph->bitmap.width;
    bit3.pitch  = glyph->bitmap.pitch;
    bit3.buffer = glyph->bitmap.buffer;

    switch ( glyph->bitmap.pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      bit3.mode  = gr_pixel_mode_mono;
      bit3.grays = 0;
      break;

    case FT_PIXEL_MODE_GRAY:
      bit3.mode  = gr_pixel_mode_gray;
      bit3.grays = glyph->bitmap.num_grays;
    }

    /* Then, blit the image to the target surface */
    x_top = x_offset + glyph->bitmap_left;
    y_top = y_offset - glyph->bitmap_top;

    grBlitGlyphToSurface( surface, &bit3,
                          x_top, y_top, fore_color );

    return 0;
  }


  static void
  Reset_Scale( int  pointSize )
  {
    (void)FT_Set_Char_Size( face,
                            pointSize << 6, pointSize << 6,
                            (FT_UInt)res, (FT_UInt)res );
  }


  static FT_Error
  LoadChar( unsigned int  idx,
            int           hint )
  {
    int  flags = FT_LOAD_NO_BITMAP;


    if ( !hint )
      flags |= FT_LOAD_NO_HINTING;

    return FT_Load_Glyph( face, idx, flags );
  }


  static FT_Error
  Render_All( int  first_glyph )
  {
    int  start_x = 18 * 8;
    int  start_y = size->metrics.y_ppem * 4 / 5 + HEADER_HEIGHT * 3;
    int  step_y  = size->metrics.y_ppem + 10;
    int  x, y, w, i;


    x = start_x;
    y = start_y;

    i = first_glyph;

    while ( i < num_glyphs )
    {
      if ( !( error = LoadChar( i, hinted ) ) )
      {
#ifdef DEBUG
        if ( i <= first_glyph + 6 )
        {
          LOG(( "metrics[%02d] = [%x %x]\n",
                i,
                glyph->metrics.horiBearingX,
                glyph->metrics.horiAdvance ));

          if ( i == first_glyph + 6 )
            LOG(( "-------------------------\n" ));
        }
#endif

        w = ( ( glyph->metrics.horiAdvance + 32 ) >> 6 ) + 1;
        if ( x + w > bit->width - 4 )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit->rows - size->metrics.y_ppem / 5 )
            return FT_Err_Ok;
        }

        Render_Glyph( x, y );
        x += w;
      }
      else
        Fail++;

      i++;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Text( int  first_glyph )
  {
    int  start_x = 18 * 8;
    int  start_y = size->metrics.y_ppem * 4 / 5 + HEADER_HEIGHT * 3;
    int  step_y  = size->metrics.y_ppem + 10;
    int  x, y, i;

    const unsigned char*  p;


    x = start_x;
    y = start_y;

    i = first_glyph;
    p = Text;
    while ( i > 0 && *p )
    {
      p++;
      i--;
    }

    while ( *p )
    {
      if ( !( error = LoadChar( FT_Get_Char_Index( face,
                                                   (unsigned char)*p ),
                                hinted ) ) )
      {
#ifdef DEBUG
        if ( i <= first_glyph + 6 )
        {
          LOG(( "metrics[%02d] = [%x %x]\n",
                i,
                glyph->metrics.horiBearingX,
                glyph->metrics.horiAdvance ));

          if ( i == first_glyph + 6 )
          LOG(( "-------------------------\n" ));
        }
#endif

        Render_Glyph( x, y );

        x += ( ( glyph->metrics.horiAdvance + 32 ) >> 6 ) + 1;

        if ( x + size->metrics.x_ppem > bit->width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit->rows - size->metrics.y_ppem / 5 )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
      p++;
    }

    return FT_Err_Ok;
  }


  static void
  Help( void )
  {
    char  buf[256];
    char  version[64];

    FT_Int  major, minor, patch;

    grEvent  dummy_event;


    FT_Library_Version( library, &major, &minor, &patch );

    if ( patch )
      snprintf( version, sizeof ( version ),
                "%d.%d.%d", major, minor, patch );
    else
      snprintf( version, sizeof ( version ),
                "%d.%d", major, minor );

    Clear_Display();
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( bit );

    snprintf( buf, sizeof ( buf ),
              "FreeType MM Glyph Viewer -"
                " part of the FreeType %s test suite",
              version );

    grWriteln( buf );
    grLn();
    grWriteln( "This program displays all glyphs from one or several" );
    grWriteln( "Multiple Masters, GX, or OpenType Variation font files." );
    grLn();
    grWriteln( "Use the following keys:");
    grLn();
    grWriteln( "F1, ?       display this help screen" );
    grWriteln( "q, ESC      quit ftmulti" );
    grLn();
    grWriteln( "F2          toggle axis grouping" );
    grWriteln( "F3          toggle fill rule flags" );
    grWriteln( "F4          toggle overlap flags" );
    grWriteln( "F5          toggle outline hinting" );
    grWriteln( "F6          cycle through hinting engines (if available)" );
    grLn();
    grWriteln( "Tab         toggle anti-aliasing" );
    grWriteln( "Space       toggle rendering mode" );
    grLn();
    grWriteln( ", .         previous/next font" );
    grLn();
    grWriteln( "Up, Down    change pointsize by 1 unit" );
    grWriteln( "PgUp, PgDn  change pointsize by 10 units" );
    grLn();
    grWriteln( "Left, Right adjust index by 1" );
    grWriteln( "F7, F8      adjust index by 16" );
    grWriteln( "F9, F10     adjust index by 256" );
    grWriteln( "F11, F12    adjust index by 4096" );
    grLn();
    grWriteln( "a, A        adjust axis 0" );
    grWriteln( "b, B        adjust axis 1" );
    grWriteln( "..." );
    grWriteln( "p, P        adjust axis 16" );
    grLn();
    grWriteln( "-, +        adjust axis range increment" );
    grLn();
    grWriteln( "Axes marked with an asterisk are hidden." );
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( surface );
    grListenSurface( surface, gr_event_key, &dummy_event );
  }


  static void
  tt_interpreter_version_change( void )
  {
    tt_interpreter_version_idx += 1;
    tt_interpreter_version_idx %= num_tt_interpreter_versions;

    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version",
                     &tt_interpreter_versions[tt_interpreter_version_idx] );
  }


  static int
  Process_Event( void )
  {
    grEvent       event;
    double        i;
    unsigned int  axis;


    grRefreshSurface( surface );
    grListenSurface( surface, 0, &event );

    if ( event.type == gr_event_resize )
      return 1;

    switch ( event.key )
    {
    case grKeyEsc:            /* ESC or q */
    case grKEY( 'q' ):
      return 0;

    case grKeyF1:
    case grKEY( '?' ):
      Help();
      break;

    case grKEY( ',' ):
    case grKEY( '.' ):
      return (int)event.key;

    /* mode keys */

    case grKeyF2:
      grouping = !grouping;
      new_header = grouping ? "axis grouping is now on"
                            : "axis grouping is now off";
      set_up_axes();
      break;

    case grKeyF3:
      fillrule  ^= FT_OUTLINE_EVEN_ODD_FILL;
      new_header = fillrule
                     ? "fill rule flags are flipped"
                     : "fill rule flags are unchanged";
      break;

    case grKeyF4:
      overlaps  ^= FT_OUTLINE_OVERLAP;
      new_header = overlaps
                     ? "overlap flags are flipped"
                     : "overlap flags are unchanged";
      break;

    case grKeyF5:
      hinted     = !hinted;
      new_header = hinted ? "glyph hinting is now active"
                          : "glyph hinting is now ignored";
      break;

    case grKeyF6:
      if ( !strcmp( font_format, "CFF" ) )
        FTDemo_Event_Cff_Hinting_Engine_Change( library,
                                                &cff_hinting_engine,
                                                1);
      else if ( !strcmp( font_format, "Type 1" ) )
        FTDemo_Event_Type1_Hinting_Engine_Change( library,
                                                  &type1_hinting_engine,
                                                  1);
      else if ( !strcmp( font_format, "CID Type 1" ) )
        FTDemo_Event_T1cid_Hinting_Engine_Change( library,
                                                  &t1cid_hinting_engine,
                                                  1);
      else if ( !strcmp( font_format, "TrueType" ) )
        tt_interpreter_version_change();
      break;

    case grKeyTab:
      antialias  = !antialias;
      new_header = antialias ? "anti-aliasing is now on"
                             : "anti-aliasing is now off";
      break;

    case grKEY( ' ' ):
      render_mode ^= 1;
      new_header   = render_mode ? "rendering all glyphs in font"
                                 : "rendering test text string";
      break;

    /* MM-related keys */

    case grKEY( '+' ):
      if ( increment < 0.1 )
        increment *= 2.0;
      break;

    case grKEY( '-' ):
      if ( increment > 0.01 )
        increment *= 0.5;
      break;

    case grKEY( 'a' ):
    case grKEY( 'b' ):
    case grKEY( 'c' ):
    case grKEY( 'd' ):
    case grKEY( 'e' ):
    case grKEY( 'f' ):
    case grKEY( 'g' ):
    case grKEY( 'h' ):
    case grKEY( 'i' ):
    case grKEY( 'j' ):
    case grKEY( 'k' ):
    case grKEY( 'l' ):
    case grKEY( 'm' ):
    case grKEY( 'n' ):
    case grKEY( 'o' ):
    case grKEY( 'p' ):
      i = -increment;
      axis = event.key - 'a';
      goto Do_Axis;

    case grKEY( 'A' ):
    case grKEY( 'B' ):
    case grKEY( 'C' ):
    case grKEY( 'D' ):
    case grKEY( 'E' ):
    case grKEY( 'F' ):
    case grKEY( 'G' ):
    case grKEY( 'H' ):
    case grKEY( 'I' ):
    case grKEY( 'J' ):
    case grKEY( 'K' ):
    case grKEY( 'L' ):
    case grKEY( 'M' ):
    case grKEY( 'N' ):
    case grKEY( 'O' ):
    case grKEY( 'P' ):
      i = increment;
      axis = event.key - 'A';
      goto Do_Axis;

    /* scaling related keys */

    case grKeyPageUp:
      i = 10;
      goto Do_Scale;

    case grKeyPageDown:
      i = -10;
      goto Do_Scale;

    case grKeyUp:
      i = 1;
      goto Do_Scale;

    case grKeyDown:
      i = -1;
      goto Do_Scale;

    /* glyph index related keys */

    case grKeyLeft:
      i = -1;
      goto Do_Glyph;

    case grKeyRight:
      i = 1;
      goto Do_Glyph;

    case grKeyF7:
      i = -16;
      goto Do_Glyph;

    case grKeyF8:
      i = 16;
      goto Do_Glyph;

    case grKeyF9:
      i = -256;
      goto Do_Glyph;

    case grKeyF10:
      i = 256;
      goto Do_Glyph;

    case grKeyF11:
      i = -4096;
      goto Do_Glyph;

    case grKeyF12:
      i = 4096;
      goto Do_Glyph;

    default:
      ;
    }
    return 1;

  Do_Axis:
    if ( axis < num_shown_axes )
    {
      FT_Var_Axis*  a;
      FT_Fixed      pos, rng;
      unsigned int  n;


      /* convert to real axis index */
      axis = (unsigned int)shown_axes[axis];

      a   = multimaster->axis + axis;
      rng = a->maximum - a->minimum;
      pos = design_pos[axis];

      /*
       * Normalize i.  Changing by 20 is all very well for PostScript fonts,
       * which tend to have a range of ~1000 per axis, but it's not useful
       * for mac fonts, which have a range of ~3.  And it's rather extreme
       * for optical size even in PS.
       */
      pos += (FT_Fixed)( i * rng );
      if ( pos < a->minimum )
        pos = a->maximum;
      if ( pos > a->maximum )
        pos = a->minimum;

      /* for MM fonts or large ranges, round the design coordinates      */
      /* otherwise round to two decimal digits to make the PS name short */
      if ( !FT_IS_SFNT( face ) || rng > 0x200000 )
        pos = i > 0 ? FT_CeilFix( pos )
                    : FT_FloorFix( pos );
      else
      {
        double  x;


        x  = pos / 65536.0 * 100.0;
        x += x < 0.0 ? -0.5 : 0.5;
        x  = (int)x;
        x  = x / 100.0 * 65536.0;
        x += x < 0.0 ? -0.5 : 0.5;

        pos = (int)x;
      }

      design_pos[axis] = pos;

      if ( grouping )
      {
        /* synchronize hidden axes with visible axis */
        for ( n = 0; n < used_num_axis; n++ )
          if ( hidden[n]                          &&
               multimaster->axis[n].tag == a->tag )
            design_pos[n] = pos;
      }

      FT_Set_Var_Design_Coordinates( face, used_num_axis, design_pos );
    }
    return 1;

  Do_Scale:
    ptsize += i;
    if ( ptsize < 1 )
      ptsize = 1;
    if ( ptsize > MAXPTSIZE )
      ptsize = MAXPTSIZE;
    return 1;

  Do_Glyph:
    Num += i;
    if ( Num < 0 )
      Num = 0;
    if ( Num >= num_glyphs )
      Num = num_glyphs - 1;
    return 1;
  }


  static void
  usage( const char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftmulti: multiple masters font viewer - part of FreeType\n"
      "--------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] [pt] font ...\n"
      "\n",
             execname );
    fprintf( stderr,
      "  pt           The point size for the given resolution.\n"
      "               If resolution is 72dpi, this directly gives the\n"
      "               ppem value (pixels per EM).\n" );
    fprintf( stderr,
      "  font         The font file(s) to display.\n"
      "\n" );
    fprintf( stderr,
      "  -d WxH       Set window dimentions (default: %ux%u).\n",
             DIM_X, DIM_Y );
    fprintf( stderr,
      "  -e encoding  Specify encoding tag (default: no encoding).\n"
      "               Common values: `unic' (Unicode), `symb' (symbol),\n"
      "               `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
      "  -r R         Use resolution R dpi (default: 72dpi).\n"
      "  -f index     Specify first glyph index to display.\n"
      "  -a \"axis1 axis2 ...\"\n"
      "               Specify the design coordinates for each\n"
      "               variation axis at start-up.\n"
      "\n"
      "  -v           Show version."
      "\n" );

    exit( 1 );
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    int    old_ptsize, orig_ptsize, file;
    int    first_glyph = 0;
    int    XisSetup = 0;
    int    option;
    int    file_loaded;

    unsigned int  n;

    unsigned int  dflt_tt_interpreter_version;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };
    const char*   execname = ft_basename( argv[0] );


    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "Could not initialize FreeType library" );

    /* get the default value as compiled into FreeType */
    FT_Property_Get( library,
                     "cff",
                     "hinting-engine", &cff_hinting_engine );
    FT_Property_Get( library,
                     "type1",
                     "hinting-engine", &type1_hinting_engine );
    FT_Property_Get( library,
                     "t1cid",
                     "hinting-engine", &t1cid_hinting_engine );

    /* collect all available versions, then set again the default */
    FT_Property_Get( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );
    for ( n = 0; n < 3; n++ )
    {
      error = FT_Property_Set( library,
                               "truetype",
                               "interpreter-version", &versions[n] );
      if ( !error )
        tt_interpreter_versions[
          num_tt_interpreter_versions++] = versions[n];
      if ( versions[n] == dflt_tt_interpreter_version )
        tt_interpreter_version_idx = n;
    }
    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    while ( 1 )
    {
      option = getopt( argc, argv, "d:e:f:h:r:vw:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'a':
        parse_design_coords( optarg );
        break;

      case 'd':
        if ( sscanf( optarg, "%hux%hu", &width, &height ) != 2 )
          usage( execname );
        break;

      case 'e':
        encoding = make_tag( optarg );
        break;

      case 'f':
        sscanf( optarg, "%i", &first_glyph );
        break;

      case 'r':
        res = atoi( optarg );
        if ( res < 1 )
          usage( execname );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( library, &major, &minor, &patch );

          printf( "ftmulti (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      default:
        usage( execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc == 0 )
      usage( execname );

    if ( argc > 1 && sscanf( argv[0], "%d", &orig_ptsize ) == 1 )
    {
      argc--;
      argv++;
    }
    else
      orig_ptsize = 64;

    file = 0;

  NewFile:
    ptsize      = orig_ptsize;
    hinted      = 1;
    file_loaded = 0;

    /* Load face */
    error = FT_New_Face( library, argv[file], 0, &face );
    if ( error )
    {
      face = NULL;
      goto Display_Font;
    }

    font_format = FT_Get_Font_Format( face );

    if ( encoding != FT_ENCODING_NONE )
    {
      error = FT_Select_Charmap( face, (FT_Encoding)encoding );
      if ( error )
        goto Display_Font;
    }

    /* retrieve multiple master information */
    FT_Done_MM_Var( library, multimaster );
    error = FT_Get_MM_Var( face, &multimaster );
    if ( error )
    {
      multimaster = NULL;
      goto Display_Font;
    }

    /* if the user specified a position, use it, otherwise  */
    /* set the current position to the default of each axis */
    if ( multimaster->num_axis > MAX_MM_AXES )
    {
      fprintf( stderr, "only handling first %u variation axes (of %u)\n",
                       MAX_MM_AXES, multimaster->num_axis );
      used_num_axis = MAX_MM_AXES;
    }
    else
      used_num_axis = multimaster->num_axis;

    for ( n = 0; n < MAX_MM_AXES; n++ )
      shown_axes[n] = -1;

    for ( n = 0; n < used_num_axis; n++ )
    {
      unsigned int  flags;


      (void)FT_Get_Var_Axis_Flags( multimaster, n, &flags );
      hidden[n] = flags & FT_VAR_AXIS_FLAG_HIDDEN;
    }

    set_up_axes();

    for ( n = 0; n < used_num_axis; n++ )
    {
      design_pos[n] = n < requested_cnt ? requested_pos[n]
                                        : multimaster->axis[n].def;
      if ( design_pos[n] < multimaster->axis[n].minimum )
        design_pos[n] = multimaster->axis[n].minimum;
      else if ( design_pos[n] > multimaster->axis[n].maximum )
        design_pos[n] = multimaster->axis[n].maximum;

      /* for MM fonts, round the design coordinates to integers */
      if ( !FT_IS_SFNT( face ) )
        design_pos[n] = FT_RoundFix( design_pos[n] );
    }

    error = FT_Set_Var_Design_Coordinates( face, used_num_axis, design_pos );
    if ( error )
      goto Display_Font;

    file_loaded++;

    Reset_Scale( ptsize );

    num_glyphs = face->num_glyphs;
    glyph      = face->glyph;
    size       = face->size;

  Display_Font:
    /* initialize graphics if needed */
    if ( !XisSetup )
    {
      XisSetup = 1;
      Init_Display();
    }

    grSetTitle( surface, "FreeType Glyph Viewer - press ? for help" );
    old_ptsize = ptsize;

    if ( file_loaded >= 1 )
    {
      Fail = 0;
      Num  = first_glyph;

      if ( Num >= num_glyphs )
        Num = num_glyphs - 1;

      if ( Num < 0 )
        Num = 0;
    }

    for ( ;; )
    {
      int     key;
      StrBuf  header[1];


      Clear_Display();

      strbuf_init( header, Header, sizeof ( Header ) );
      strbuf_reset( header );

      if ( file_loaded >= 1 )
      {
        switch ( render_mode )
        {
        case 0:
          Render_Text( Num );
          break;

        default:
          Render_All( Num );
        }

        strbuf_format( header, "%.50s %.50s (file %.100s)",
                       face->family_name,
                       face->style_name,
                       ft_basename( argv[file] ) );

        if ( !new_header )
          new_header = Header;

        grWriteCellString( bit, 0, 0, new_header, fore_color );
        new_header = NULL;

        strbuf_reset( header );
        strbuf_format( header, "PS name: %s",
                       FT_Get_Postscript_Name( face ) );
        grWriteCellString( bit, 0, 2 * HEADER_HEIGHT, Header, fore_color );

        strbuf_reset( header );
        strbuf_format( header, "axes (\361 %.1f%%):", 100.0 * increment );
        grWriteCellString( bit, 0, 4 * HEADER_HEIGHT, Header, fore_color );
        for ( n = 0; n < num_shown_axes; n++ )
        {
          int  axis = shown_axes[n];


          strbuf_reset( header );
          strbuf_format( header, "%c %.50s%s:",
                         n + 'A',
                         multimaster->axis[axis].name,
                         hidden[axis] ? "*" : "" );
          if ( design_pos[axis] & 0xFFFF )
            strbuf_format( header, "% .2f", design_pos[axis] / 65536.0 );
          else
            strbuf_format( header,   "% d", design_pos[axis] / 65536 );
          grWriteCellString( bit, 0, (int)( n + 5 ) * HEADER_HEIGHT,
                             Header, fore_color );
        }

        {
          unsigned int  tt_ver = tt_interpreter_versions[
                                   tt_interpreter_version_idx];

          const char*  format_str = NULL;


          if ( !strcmp( font_format, "CFF" ) )
            format_str = ( cff_hinting_engine == FT_HINTING_FREETYPE
                         ? "CFF (FreeType)"
                         : "CFF (Adobe)" );
          else if ( !strcmp( font_format, "Type 1" ) )
            format_str = ( type1_hinting_engine == FT_HINTING_FREETYPE
                         ? "Type 1 (FreeType)"
                         : "Type 1 (Adobe)" );
          else if ( !strcmp( font_format, "CID Type 1" ) )
            format_str = ( t1cid_hinting_engine == FT_HINTING_FREETYPE
                         ? "CID Type 1 (FreeType)"
                         : "CID Type 1 (Adobe)" );
          else if ( !strcmp( font_format, "TrueType" ) )
            format_str = ( tt_ver == TT_INTERPRETER_VERSION_35
                                   ? "TrueType (v35)"
                                   : ( tt_ver == TT_INTERPRETER_VERSION_38
                                       ? "TrueType (v38)"
                                       : "TrueType (v40)" ) );

          strbuf_reset( header );
          strbuf_format(
            header,
            "size: %dpt, first glyph: %d, format: %s",
            ptsize,
            Num,
            format_str );
        }
      }
      else
        strbuf_format( header,
                       "%.100s: not an MM font file, or could not be opened",
                       ft_basename( argv[file] ) );

      grWriteCellString( bit, 0, HEADER_HEIGHT, Header, fore_color );

      if ( !( key = Process_Event() ) )
        goto End;

      if ( key == grKEY( '.' ) )
      {
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        if ( file < argc - 1 )
          file++;

        goto NewFile;
      }

      if ( key == grKEY( ',' ) )
      {
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        if ( file > 0 )
          file--;

        goto NewFile;
      }

      if ( key == grKeyF6 )
      {
        /* enforce reloading */
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        goto NewFile;
      }

      if ( ptsize != old_ptsize )
      {
        Reset_Scale( ptsize );

        old_ptsize = ptsize;
      }
    }

  End:
    grDoneSurface( surface );
    grDoneDevices();

    free            ( multimaster );
    FT_Done_Face    ( face        );
    FT_Done_FreeType( library     );

    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", Fail );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
