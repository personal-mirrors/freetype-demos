/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 2019-2021 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftpngout.c - PNG printing routines for FreeType demo programs.          */
/*                                                                          */
/****************************************************************************/

#include "ftcommon.h"


#ifdef FT_CONFIG_OPTION_USE_PNG

#include <png.h>

  int
  FTDemo_Display_Print( FTDemo_Display*  display,
                        const char*      filename,
                        FT_String*       ver_str )
  {
    grBitmap*  bit    = display->bitmap;
    int        width  = bit->width;
    int        height = bit->rows;
    int        color_type;

    int   code = 1;
    FILE *fp   = NULL;

    png_structp  png_ptr  = NULL;
    png_infop    info_ptr = NULL;
    png_bytep    row      = NULL;


    /* Set color_type */
    switch ( bit-> mode )
    {
    case gr_pixel_mode_gray:
      color_type = PNG_COLOR_TYPE_GRAY;
      break;
    case gr_pixel_mode_rgb24:
    case gr_pixel_mode_rgb32:
      color_type = PNG_COLOR_TYPE_RGB;
      break;
    default:
      fprintf( stderr, "Unsupported color type\n" );
      goto Exit0;
    }

    /* Open file for writing (binary mode) */
    fp = fopen( filename, "wb" );
    if ( fp == NULL )
    {
      fprintf( stderr, "Could not open file %s for writing\n", filename );
      goto Exit0;
    }

    /* Initialize write structure */
    png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if ( png_ptr == NULL )
    {
       fprintf( stderr, "Could not allocate write struct\n" );
       goto Exit1;
    }

    /* Initialize info structure */
    info_ptr = png_create_info_struct( png_ptr );
    if ( info_ptr == NULL )
    {
      fprintf( stderr, "Could not allocate info struct\n" );
      goto Exit2;
    }

    /* Set up exception handling */
    if ( setjmp( png_jmpbuf( png_ptr ) ) )
    {
      fprintf( stderr, "Error during png creation\n" );
      goto Exit2;
    }

    png_init_io( png_ptr, fp );

    /* Write header (8 bit colour depth) */
    png_set_IHDR( png_ptr, info_ptr, width, height,
                  8, color_type, PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

    /* Record version string  */
    if ( ver_str != NULL )
    {
      png_text  text;


      text.compression = PNG_TEXT_COMPRESSION_NONE;
      text.key         = (char *)"Software";
      text.text        = ver_str;

      png_set_text( png_ptr, info_ptr, &text, 1 );
    }

    /* Set gamma */
    png_set_gAMA( png_ptr, info_ptr, 1.0 / display->gamma );

    png_write_info( png_ptr, info_ptr );

    if ( bit->mode == gr_pixel_mode_rgb32 )
    {
      const int  x = 1;


      if ( *(char*)&x )  /* little endian */
      {
        png_set_filler( png_ptr, 0, PNG_FILLER_AFTER );
        png_set_bgr( png_ptr );
      }
      else
        png_set_filler( png_ptr, 0, PNG_FILLER_BEFORE );
    }

    /* Write image rows */
    row = bit->buffer;
    if ( bit->pitch < 0 )
      row -= ( bit->rows - 1 ) * bit->pitch;
    while ( height-- )
    {
      png_write_row( png_ptr, row );
      row += bit->pitch;
    }

    /* End write */
    png_write_end( png_ptr, NULL );
    code = 0;

  Exit2:
    png_destroy_write_struct( &png_ptr, &info_ptr );
  Exit1:
    fclose( fp );
  Exit0:
    return code;
  }

#else

  int
  FTDemo_Display_Print( FTDemo_Display*  display,
                        const char*      filename,
                        FT_String*       ver_str )
  {
    return 0;
  }

#endif /* !FT_CONFIG_OPTION_USE_PNG */


/* End */
