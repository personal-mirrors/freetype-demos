/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2002-2006, 2009, 2010, 2013 by                                */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftbench: bench some common FreeType call paths                          */
/*                                                                          */
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_CACHE_CHARMAP_H
#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include FT_SYNTHESIS_H
#include FT_ADVANCES_H
#include FT_BBOX_H

#ifdef UNIX
#include <sys/time.h>
#endif

#include "common.h"


typedef struct {
  double  t0;
  double  total;
} btimer_t;

typedef int
(*bcall_t)( btimer_t*  timer,
            FT_Face    face,
            void*      user_data );

typedef struct {
  const char*  title;
  bcall_t      bench;
  int          cache_first;
  void*        user_data;
} btest_t;

typedef struct
{
  FT_Int     size;
  FT_ULong*  code;
} bcharset_t;

FT_Error
get_face( FT_Face*     face );


/*
 * Globals
 */

#define CACHE_SIZE 1024
#define BENCH_TIME 2.0f
#define FACE_SIZE  10

FT_Library        lib;
FTC_Manager       cache_man;
FTC_CMapCache     cmap_cache;
FTC_ImageCache    image_cache;
FTC_SBitCache     sbit_cache;
FTC_ImageTypeRec  font_type;

enum {
  FT_BENCH_LOAD_GLYPH,
  FT_BENCH_LOAD_ADVANCES,
  FT_BENCH_RENDER,
  FT_BENCH_GET_GLYPH,
  FT_BENCH_GET_CBOX,
  FT_BENCH_CMAP,
  FT_BENCH_CMAP_ITER,
  FT_BENCH_NEW_FACE,
  FT_BENCH_EMBOLDEN,
  FT_BENCH_GET_BBOX,
  N_FT_BENCH
};

const char* bench_desc[] = {
  "load a glyph        (FT_Load_Glyph)",
  "load advance widths (FT_Get_Advances)",
  "render a glyph      (FT_Render_Glyph)",
  "load a glyph        (FT_Get_Glyph)",
  "get glyph cbox      (FT_Glyph_Get_CBox)",
  "get glyph indices   (FT_Get_Char_Index",
  "iterate CMap        (FT_Get_{First,Next}_Char)",
  "open a new face     (FT_New_Face)",
  "embolden            (FT_GlyphSlot_Embolden)",
  "get glyph bbox      (FT_Outline_Get_BBox)",
  NULL
};

int             preload;
char*           filename;

unsigned int    first_index;

FT_Render_Mode  render_mode = FT_RENDER_MODE_NORMAL;
FT_Int32        load_flags  = FT_LOAD_DEFAULT;


/*
 * Dummy face requester (the face object is already loaded)
 */

FT_Error
face_requester( FTC_FaceID  face_id,
                FT_Library  library,
                FT_Pointer  request_data,
                FT_Face*    aface )
{
  FT_UNUSED( face_id );
  FT_UNUSED( library );

  *aface = (FT_Face)request_data;

  return 0;
}


/*
 * timer
 */

double
get_time(void)
{
#ifdef UNIX
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1E6;
#else
  /* clock() has an awful precision (~10ms) under Linux 2.4 + glibc 2.2 */
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

#define TIMER_START( timer )   ( timer )->t0 = get_time()
#define TIMER_STOP( timer )    ( timer )->total += get_time() - ( timer )->t0
#define TIMER_GET( timer )     ( timer )->total
#define TIMER_RESET( timer )   ( timer )->total = 0


/*
 * Bench code
 */

void
benchmark( FT_Face   face,
           btest_t*  test,
           int       max_iter,
           double    max_time )
{
  int       n, done;
  btimer_t  timer, elapsed;


  if ( test->cache_first )
  {
    if ( !cache_man )
    {
      printf( "%-25s : no cache manager\n", test->title );

      return;
    }

    TIMER_RESET( &timer );
    test->bench( &timer, face, test->user_data );
  }

  printf( "%-25s : ", test->title );
  fflush( stdout );

  n = done = 0;
  TIMER_RESET( &timer );
  TIMER_RESET( &elapsed );

  for ( n = 0; !max_iter || n < max_iter; n++ )
  {
    TIMER_START( &elapsed );

    done += test->bench( &timer, face, test->user_data );

    TIMER_STOP( &elapsed );

    if ( TIMER_GET( &elapsed ) > max_time )
      break;
  }

  printf("%5.3f us/op\n", TIMER_GET( &timer ) * 1E6 / (double)done);
}


/*
 * Various tests
 */

int
test_load( btimer_t*  timer,
           FT_Face    face,
           void*      user_data )
{
  int  i, done = 0;


  FT_UNUSED( user_data );

  TIMER_START( timer );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( !FT_Load_Glyph( face, i, load_flags ) )
      done++;
  }

  TIMER_STOP( timer );

  return done;
}


int
test_load_advances( btimer_t*  timer,
                    FT_Face    face,
                    void*      user_data )
{
  int        done = 0;
  FT_Fixed*  advances;
  FT_ULong   flags = *((FT_ULong*)user_data);


  advances = (FT_Fixed *)calloc( sizeof ( FT_Fixed ), face->num_glyphs );

  TIMER_START( timer );

  FT_Get_Advances( face,
                   first_index, face->num_glyphs - first_index,
                   flags, advances );
  done += face->num_glyphs - first_index;

  TIMER_STOP( timer );

  free( advances );

  return done;
}


int
test_render( btimer_t*  timer,
             FT_Face    face,
             void*      user_data )
{
  int  i, done = 0;


  FT_UNUSED( user_data );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( FT_Load_Glyph( face, i, load_flags ) )
      continue;

    TIMER_START( timer );
    if ( !FT_Render_Glyph( face->glyph, render_mode ) )
      done++;
    TIMER_STOP( timer );
  }

  return done;
}

int
test_embolden( btimer_t*  timer,
             FT_Face    face,
             void*      user_data )
{
  int  i, done = 0;


  FT_UNUSED( user_data );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( FT_Load_Glyph( face, i, load_flags ) )
      continue;

    TIMER_START( timer );
    /*FT_GlyphSlot_Embolden ( face->glyph );*/
    done++;
    TIMER_STOP( timer );
  }

  return done;
}


int
test_get_glyph( btimer_t*  timer,
                FT_Face    face,
                void*      user_data )
{
  FT_Glyph  glyph;
  int       i, done = 0;


  FT_UNUSED( user_data );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( FT_Load_Glyph( face, i, load_flags ) )
      continue;

    TIMER_START( timer );
    if ( !FT_Get_Glyph( face->glyph, &glyph ) )
    {
      FT_Done_Glyph( glyph );
      done++;
    }
    TIMER_STOP( timer );
  }

  return done;
}


int
test_get_cbox( btimer_t*  timer,
               FT_Face    face,
               void*      user_data )
{
  FT_Glyph  glyph;
  FT_BBox   bbox;
  int       i, done = 0;


  FT_UNUSED( user_data );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( FT_Load_Glyph( face, i, load_flags ) )
      continue;

    if ( FT_Get_Glyph( face->glyph, &glyph ) )
      continue;

    TIMER_START( timer );
    FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_PIXELS, &bbox );
    TIMER_STOP( timer );

    FT_Done_Glyph( glyph );
    done++;
  }

  return done;
}


int
test_get_bbox( btimer_t*  timer,
               FT_Face    face,
               void*      user_data )
{
  FT_BBox  bbox;
  int      i, done = 0;


  FT_UNUSED( user_data );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    FT_Outline*  outline;


    if ( FT_Load_Glyph( face, i, load_flags ) )
      continue;

    outline = &face->glyph->outline;

    TIMER_START( timer );
    FT_Outline_Get_BBox( outline, &bbox );
    TIMER_STOP( timer );

    done++;
  }

  return done;
}


int
test_get_char_index( btimer_t*  timer,
                     FT_Face    face,
                     void*      user_data )
{
  bcharset_t*  charset = (bcharset_t*)user_data;
  int          i, done = 0;


  TIMER_START( timer );

  for ( i = 0; i < charset->size; i++ )
  {
    if ( FT_Get_Char_Index(face, charset->code[i]) )
      done++;
  }

  TIMER_STOP( timer );

  return done;
}


int
test_cmap_cache( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
{
  bcharset_t*  charset = (bcharset_t*)user_data;
  int          i, done = 0;


  FT_UNUSED( face );

  if ( !cmap_cache )
  {
    if ( FTC_CMapCache_New(cache_man, &cmap_cache) )
      return 0;
  }

  TIMER_START( timer );

  for ( i = 0; i < charset->size; i++ )
  {
    if ( FTC_CMapCache_Lookup( cmap_cache, font_type.face_id, 0, charset->code[i] ) )
      done++;
  }

  TIMER_STOP( timer );

  return done;
}


int
test_image_cache( btimer_t*  timer,
                  FT_Face    face,
                  void*      user_data )
{
  FT_Glyph  glyph;
  int       i, done = 0;


  FT_UNUSED( user_data );

  if ( !image_cache )
  {
    if ( FTC_ImageCache_New(cache_man, &image_cache) )
      return 0;
  }

  TIMER_START( timer );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( !FTC_ImageCache_Lookup(image_cache, &font_type, i, &glyph, NULL) )
      done++;
  }

  TIMER_STOP( timer );

  return done;
}


int
test_sbit_cache( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
{
  FTC_SBit  glyph;
  int       i, done = 0;


  FT_UNUSED( user_data );

  if ( !sbit_cache )
  {
    if ( FTC_SBitCache_New(cache_man, &sbit_cache) )
      return 0;
  }

  TIMER_START( timer );

  for ( i = first_index; i < face->num_glyphs; i++ )
  {
    if ( !FTC_SBitCache_Lookup(sbit_cache, &font_type, i, &glyph, NULL) )
      done++;
  }

  TIMER_STOP( timer );

  return done;
}


int
test_cmap_iter( btimer_t*  timer,
                FT_Face    face,
                void*      user_data )
{
  FT_UInt   idx;
  FT_ULong  charcode;


  FT_UNUSED( user_data );

  TIMER_START( timer );

  charcode = FT_Get_First_Char( face, &idx );
  while ( idx != 0 )
    charcode = FT_Get_Next_Char( face, charcode, &idx );

  TIMER_STOP( timer );

  return 1;
}


int
test_new_face( btimer_t*  timer,
               FT_Face    face,
               void*      user_data )
{
  FT_Face  bench_face;


  FT_UNUSED( face );
  FT_UNUSED( user_data );

  TIMER_START( timer );

  if ( !get_face( &bench_face ) )
    FT_Done_Face( bench_face );

  TIMER_STOP( timer );

  return 1;
}


/*
 * main
 */

void
get_charset( FT_Face      face,
             bcharset_t*  charset )
{
  FT_ULong  charcode;
  FT_UInt   gindex;
  int i;


  charset->code = (FT_ULong*)calloc( face->num_glyphs, sizeof( FT_ULong ) );
  if ( !charset->code )
    return;

  if ( face->charmap )
  {
    i = 0;
    charcode = FT_Get_First_Char(face, &gindex);

    /* certain fonts contain a broken charmap that will map character codes */
    /* to out-of-bounds glyph indices.  Take care of that here.             */
    /*                                                                      */
    while ( gindex && i < face->num_glyphs )
    {
      if ( gindex >= first_index )
        charset->code[i++] = charcode;
      charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

  }
  else
  {
    int  j;


    /* no charmap, do an identity mapping */
    for ( i = 0, j = first_index; j < face->num_glyphs; i++, j++ )
      charset->code[i] = j;
  }

  charset->size = i;
}


FT_Error
get_face( FT_Face*     face )
{
  static unsigned char*  memory_file = NULL;
  static size_t          memory_size;
  int                    face_index = 0;
  FT_Error               error;


  if ( preload )
  {
    if ( !memory_file )
    {
      FILE*  file = fopen( filename, "rb" );


      if ( file == NULL )
      {
        fprintf( stderr, "couldn't find or open `%s'\n", filename );

        return 1;
      }

      fseek( file, 0, SEEK_END );
      memory_size = ftell( file );
      fseek( file, 0, SEEK_SET );

      memory_file = (FT_Byte*)malloc( memory_size );
      if ( memory_file == NULL )
      {
        fprintf( stderr, "couldn't allocate memory to pre-load font file\n" );

        return 1;
      }

      if ( fread( memory_file, 1, memory_size, file ) != memory_size )
      {
        fprintf( stderr, "read error\n" );
        free( memory_file );
        memory_file = NULL;

        return 1;
      }
    }

    error = FT_New_Memory_Face( lib, memory_file, memory_size, face_index, face );
  }
  else
    error = FT_New_Face(lib, filename, face_index, face);

  if ( error )
    fprintf( stderr, "couldn't load font resource\n");

  return error;
}


void usage(void)
{
  int  i;


  fprintf( stderr,
    "\n"
    "ftbench: bench some common FreeType paths\n"
    "-----------------------------------------\n"
    "\n"
    "Usage: ftbench [options] fontname\n"
    "\n"
    "  -C        Compare with cached version (if available).\n"
    "  -c N      Use at most N iterations for each test\n"
    "            (0 means time limited).\n"
    "  -f L      Use hex number L as load flags.\n"
    "  -i IDX    Start with index IDX (default is 0).\n"
    "  -m M      Set maximum cache size to M KByte (default is %d).\n",
           CACHE_SIZE );
  fprintf( stderr,
    "  -p        Preload font file in memory.\n"
    "  -r N      Set render mode to N\n"
    "              0: normal, 1: light, 2: mono, 3: LCD, 4: LCD vertical\n"
    "            (default is 0).\n"
    "  -s S      Use S ppem as face size (default is %dppem).\n",
           FACE_SIZE );
  fprintf( stderr,
    "  -t T      Use at most T seconds per bench (default is %.0f).\n"
    "\n"
    "  -b tests  Perform chosen tests (default is all):\n",
           BENCH_TIME );

  for ( i = 0; i < N_FT_BENCH; i++ )
  {
    if ( !bench_desc[i] )
      break;

    fprintf( stderr,
      "              %c  %s\n", 'a' + i, bench_desc[i] );
  }

  fprintf( stderr,
    "\n"
    "  -v        Show version.\n"
    "\n" );

  exit( 1 );
}


#define TEST(x) (!test_string || strchr(test_string, (x)))

int
main(int argc,
     char** argv)
{
  FT_Face     face;
  long        max_bytes = CACHE_SIZE * 1024;
  char*       test_string = NULL;
  int         size = FACE_SIZE;
  int         max_iter = 0;
  double      max_time = BENCH_TIME;
  int         compare_cached = 0;
  int         i;


  if ( FT_Init_FreeType( &lib ) )
  {
    fprintf( stderr, "could not initialize font library\n" );

    return 1;
  }

  while ( 1 )
  {
    int  opt;


    opt = getopt( argc, argv, "b:Cc:f:i:m:pr:s:t:v" );

    if ( opt == -1 )
      break;

    switch ( opt )
    {
    case 'b':
      test_string = optarg;
      break;

    case 'C':
      compare_cached = 1;
      break;

    case 'c':
      max_iter = atoi( optarg );
      break;

    case 'f':
      load_flags = strtol( optarg, NULL, 16 );
      break;

    case 'i':
      first_index = atoi( optarg );
      break;

    case 'm':
      max_bytes  = atoi( optarg );
      max_bytes *= 1024;
      break;

    case 'p':
      preload = 1;
      break;

    case 'r':
      render_mode = (FT_Render_Mode)atoi( optarg );
      if ( render_mode >= FT_RENDER_MODE_MAX )
        render_mode = FT_RENDER_MODE_NORMAL;
      break;

    case 's':
      size = atoi( optarg );
      if ( size <= 0 )
        size = 1;
      else if ( size > 500 )
        size = 500;
      break;

    case 't':
      max_time = atof( optarg );
      break;

    case 'v':
      {
        FT_Int  major, minor, patch;


        FT_Library_Version( lib, &major, &minor, &patch );

        printf( "ftbench (FreeType) %d.%d", major, minor );
        if ( patch )
          printf( ".%d", patch );
        printf( "\n" );
        exit( 0 );
      }
      break;

    default:
      usage();
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if ( argc != 1 )
    usage();

  filename = *argv;

  if ( get_face( &face ) )
    goto Exit;

  if ( FT_IS_SCALABLE( face ) )
  {

    if ( FT_Set_Pixel_Sizes( face, size, size ) )
    {
      fprintf( stderr, "failed to set pixel size to %d\n", size );

      return 1;
    }
  }
  else
    size = face->available_sizes[0].width;

  FTC_Manager_New( lib, 0, 0, max_bytes, face_requester, face, &cache_man );

  font_type.face_id = (FTC_FaceID) 1;
  font_type.width   = (short) size;
  font_type.height  = (short) size;
  font_type.flags   = load_flags;

  for ( i = 0; i < N_FT_BENCH; i++ )
  {
    btest_t   test;
    FT_ULong  flags;


    if ( !TEST( 'a' + i ) )
      continue;

    test.title       = NULL;
    test.bench       = NULL;
    test.cache_first = 0;
    test.user_data   = NULL;

    switch ( i )
    {
    case FT_BENCH_LOAD_GLYPH:
      test.title = "Load";
      test.bench = test_load;
      benchmark( face, &test, max_iter, max_time );

      if ( compare_cached )
      {
        test.cache_first = 1;

        test.title = "Load (image cached)";
        test.bench = test_image_cache;
        benchmark( face, &test, max_iter, max_time );

        test.title = "Load (sbit cached)";
        test.bench = test_sbit_cache;
        benchmark( face, &test, max_iter, max_time );
      }
      break;

    case FT_BENCH_LOAD_ADVANCES:
      test.user_data = &flags;

      test.title = "Load_Advances (Normal)";
      test.bench = test_load_advances;
      flags      = FT_LOAD_DEFAULT;
      benchmark( face, &test, max_iter, max_time );

      test.title  = "Load_Advances (Fast)";
      test.bench  = test_load_advances;
      flags       = FT_LOAD_TARGET_LIGHT;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_RENDER:
      test.title = "Render";
      test.bench = test_render;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_GET_GLYPH:
      test.title = "Get_Glyph";
      test.bench = test_get_glyph;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_GET_CBOX:
      test.title = "Get_CBox";
      test.bench = test_get_cbox;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_GET_BBOX:
      test.title = "Get_BBox";
      test.bench = test_get_bbox;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_CMAP:
      {
        bcharset_t  charset;


        get_charset( face, &charset );
        if ( charset.code )
        {
          test.user_data = (void*)&charset;


          test.title = "Get_Char_Index";
          test.bench = test_get_char_index;

          benchmark( face, &test, max_iter, max_time );

          if ( compare_cached )
          {
            test.cache_first = 1;

            test.title = "Get_Char_Index (cached)";
            test.bench = test_cmap_cache;
            benchmark( face, &test, max_iter, max_time );
          }

          free( charset.code );
        }
      }
      break;

    case FT_BENCH_CMAP_ITER:
      test.title = "Iterate CMap";
      test.bench = test_cmap_iter;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_NEW_FACE:
      test.title = "New_Face";
      test.bench = test_new_face;
      benchmark( face, &test, max_iter, max_time );
      break;

    case FT_BENCH_EMBOLDEN:
      test.title = "Embolden";
      test.bench = test_embolden;
      benchmark( face, &test, max_iter, max_time );
      break;
    }
  }

Exit:
  /* The following is a bit subtle: When we call FTC_Manager_Done, this
   * normally destroys all FT_Face objects that the cache might have created
   * by calling the face requester.
   *
   * However, this little benchmark uses a tricky face requester that
   * doesn't create a new FT_Face through FT_New_Face but simply passes a
   * pointer to the one that was previously created.
   *
   * If the cache manager has been used before, the call to FTC_Manager_Done
   * discards our single FT_Face.
   *
   * In the case where no cache manager is in place, or if no test was run,
   * the call to FT_Done_FreeType releases any remaining FT_Face object
   * anyway.
   */
  if ( cache_man )
    FTC_Manager_Done( cache_man );

  FT_Done_FreeType( lib );

  return 0;
}


/* End */
