/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright (C) 1996-2021 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftlint: a simple font tester. This program tries to load all the        */
/*          glyphs of a given font.                                         */
/*                                                                          */
/*  NOTE:  This is just a test program that is used to show off and         */
/*         debug the current engine.                                        */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftcommon.h"
#include "md5.h"

#ifdef UNIX
#include <unistd.h>
#else
#include "mlgetopt.h"
#endif


#define  xxTEST_PSNAMES

  static FT_Library      library;
  static FT_Face         face;
  static FT_Render_Mode  render_mode = FT_RENDER_MODE_NORMAL;
  static FT_Int32        load_flags  = FT_LOAD_DEFAULT;

  static unsigned int  num_glyphs;
  static int           ptsize;

  static int  Fail;


  static void
  Usage( char*  name )
  {
    printf( "ftlint: simple font tester -- part of the FreeType project\n" );
    printf( "----------------------------------------------------------\n" );
    printf( "\n" );
    printf( "Usage: %s [options] ppem fontname[.ttf|.ttc] [fontname2..]\n", name );
    printf( "\n" );
    printf( "  -f L      Use hex number L as load flags (see `FT_LOAD_XXX').\n" );
    printf( "  -r N      Set render mode to N\n" );

    exit( 1 );
  }


  static void
  Panic( const char*  message )
  {
    fprintf( stderr, "%s\n  error code = 0x%04x\n", message, error );
    exit(1);
  }

  static void
  Checksum( int id, FT_Face face )
  {
      FT_Bitmap  bitmap;
      FT_Error err;
      FT_Bitmap_Init( &bitmap );
      
      
      err = FT_Render_Glyph( face->glyph, render_mode);
      err = FT_Bitmap_Convert( library, &face->glyph->bitmap, &bitmap, 1 );
      if ( !err )
      {
        MD5_CTX        ctx;
        unsigned char  md5[16];
        int            i;
        int            rows  = (int)bitmap.rows;
        int            pitch = bitmap.pitch;

        MD5_Init( &ctx );
        if ( bitmap.buffer )
          MD5_Update( &ctx, bitmap.buffer,
                      (unsigned long)rows * (unsigned long)pitch );
        MD5_Final( md5, &ctx );

        printf( "#%d ", id );
        for ( i = 0; i < 16; i++ ){
          printf( "%02X", md5[i] );
        }
        printf( "\n" );
      }else{
      	printf("Error generating checksums");
      }
      FT_Bitmap_Done( library, &bitmap );
  }


  int
  main( int     argc,
        char**  argv )
  {
    int           i, file_index;
    unsigned int  id;
    char          filename[1024];
    char*         execname;
    char*         fname;
    int opt;

    execname = argv[0];
    
    if (argc < 3 )
      Usage( execname );

    while ( (opt =  getopt( argc, argv, "f:r:")) != -1)
    {

      switch ( opt )
      {

      case 'f':
        load_flags = strtol( optarg, NULL, 16 );
        break;

      case 'r':
        {
         int  rm = atoi( optarg );


         if ( rm < 0 || rm >= FT_RENDER_MODE_MAX )
            render_mode = FT_RENDER_MODE_NORMAL;
          else
            render_mode = (FT_Render_Mode)rm;
        }
        break;

      default:
        Usage( execname );
        break;
      }
    }
    
    argc -= optind;
    argv += optind;


    if( sscanf( argv[0], "%d", &ptsize) != 1)
      Usage( execname );       

    error = FT_Init_FreeType( &library );
    if (error) Panic( "Could not create library object" );


    /* Now check all files */
    for ( file_index = 1; file_index < argc; file_index++ )
    {
      fname = argv[file_index];

      /* try to open the file with no extra extension first */
      error = FT_New_Face( library, fname, 0, &face );
      if (!error)
      {
        printf( "%s: \n", fname );
        goto Success;
      }


      if ( error == FT_Err_Unknown_File_Format )
      {
        printf( "unknown format\n" );
        continue;
      }

      /* ok, we could not load the file, try to add an extension to */
      /* its name if possible..                                     */

      i = (int)strlen( fname );
      while ( i > 0 && fname[i] != '\\' && fname[i] != '/' )
      {
        if ( fname[i] == '.' )
          i = 0;
        i--;
      }

#ifndef macintosh
      snprintf( filename, sizeof ( filename ), "%s%s", fname,
                ( i >= 0 ) ? ".ttf" : "" );
#else
      snprintf( filename, sizeof ( filename ), "%s", fname );
#endif

      i     = (int)strlen( filename );
      fname = filename;

      while ( i >= 0 )
#ifndef macintosh
        if ( filename[i] == '/' || filename[i] == '\\' )
#else
        if ( filename[i] == ':' )
#endif
        {
          fname = filename + i + 1;
          i = -1;
        }
        else
          i--;

      printf( "%s: \n", fname );

      /* Load face */
      error = FT_New_Face( library, filename, 0, &face );
      if (error)
      {
        if (error == FT_Err_Unknown_File_Format)
          printf( "unknown format\n" );
        else
          printf( "could not find/open file (error: %d)\n", error );
        continue;
      }
      if (error) Panic( "Could not open file" );

  Success:
      num_glyphs = (unsigned int)face->num_glyphs;

#ifdef  TEST_PSNAMES
      {
        const char*  ps_name = FT_Get_Postscript_Name( face );

        printf( "[%s] ", ps_name ? ps_name : "." );
      }
#endif

      error = FT_Set_Char_Size( face, ptsize << 6, ptsize << 6, 72, 72 );
      if (error) Panic( "Could not set character size" );
      Fail = 0;
      {
        for ( id = 0; id < num_glyphs; id++ )
        {
          error = FT_Load_Glyph( face, id, load_flags );
          Checksum(id, face);
          
          if (error)
          {
            if ( Fail < 10 )
              printf( "glyph %4u: 0x%04x\n" , id, error );
            Fail++;
          }
        }
      }

      if ( Fail == 0 )
        printf( "OK.\n" );
      else
        if ( Fail == 1 )
          printf( "1 fail.\n" );
        else
          printf( "%d fails.\n", Fail );

      FT_Done_Face( face );
    }

    FT_Done_FreeType(library);
    exit( 0 );      /* for safety reasons */

    /* return 0; */ /* never reached */
  }

/* End */
