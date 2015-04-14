/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2015 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  aux.h - auxiliary routines for the FreeType demo programs.              */
/*                                                                          */
/****************************************************************************/


#ifndef _AUX_H_
#define _AUX_H_


#include <ft2build.h>
#include FT_FREETYPE_H


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    STRING OUTPUT FUNCTIONS                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  void
  put_ascii_string( char*     out,
                    FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent );

  FT_UInt
  put_ascii_string_size( FT_Byte*  string,
                         FT_UInt   string_len,
                         FT_UInt   indent );

  void
  put_ascii( FT_Byte*  string,
             FT_UInt   string_len,
             FT_UInt   indent );


  void
  put_unicode_be16_string( char*     out,
                           FT_Byte*  string,
                           FT_UInt   string_len,
                           FT_UInt   indent,
                           FT_Int    as_utf8 );

  FT_UInt
  put_unicode_be16_string_size( FT_Byte*  string,
                                FT_UInt   string_len,
                                FT_UInt   indent,
                                FT_Int    as_utf8 );

  void
  put_unicode_be16( FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent,
                    FT_Int    as_utf8 );


#endif /* _AUX_H_ */

/* End */
