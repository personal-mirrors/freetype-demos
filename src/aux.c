/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2015 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  aux.c - auxiliary routines for the FreeType demo programs.              */
/*                                                                          */
/****************************************************************************/


#include "aux.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    STRING OUTPUT FUNCTIONS                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  void
  put_ascii( FT_Byte*  string,
             FT_UInt   string_len,
             FT_UInt   indent )
  {
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      putchar( ' ' );
    putchar( '"' );

    for ( i = 0; i < string_len; i++ )
    {
      switch ( string[i] )
      {
      case '\n':
        fputs( "\\n\"", stdout );
        if ( i + 1 < string_len )
        {
          putchar( '\n' );
          for ( j = 0; j < indent; j++ )
            putchar( ' ' );
          putchar( '"' );
        }
        break;
      case '\r':
        fputs( "\\r", stdout );
        break;
      case '\t':
        fputs( "\\t", stdout );
        break;
      case '\\':
        fputs( "\\\\", stdout );
        break;
      case '"':
        fputs( "\\\"", stdout );
        break;

      default:
        if ( string[i] < 0x80 )
          putchar( string[i] );
        else
          printf( "\\x%02X", string[i] );
        break;
      }
    }
    if ( string[i - 1] != '\n' )
      putchar( '"' );
  }


  void
  put_unicode_be16( FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent,
                    FT_Int    as_utf8 )
  {
    FT_Int   ch = 0;
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      putchar( ' ' );
    putchar( '"' );

    for ( i = 0; i < string_len; i += 2 )
    {
      ch = ( string[i] << 8 ) | string[i + 1];

      switch ( ch )
      {
      case '\n':
        fputs( "\\n\"", stdout );
        if ( i + 2 < string_len )
        {
          putchar( '\n' );
          for ( j = 0; j < indent; j++ )
            putchar( ' ' );
          putchar( '"' );
        }
        continue;
      case '\r':
        fputs( "\\r", stdout );
        continue;
      case '\t':
        fputs( "\\t", stdout );
        continue;
      case '\\':
        fputs( "\\\\", stdout );
        continue;
      case '"':
        fputs( "\\\"", stdout );
        continue;
      default:
        break;
      }

      if ( as_utf8 )
      {
        /*
         * UTF-8 encoding
         *
         *   0x00000080 - 0x000007FF:
         *        110xxxxx 10xxxxxx
         *
         *   0x00000800 - 0x0000FFFF:
         *        1110xxxx 10xxxxxx 10xxxxxx
         */

        if ( ch < 0x80 )
          putchar( ch );
        else if ( ch < 0x800 )
        {
          putchar( 0xC0 | ( (FT_UInt)ch >> 6 ) );
          putchar( 0x80 | ( (FT_UInt)ch & 0x3F ) );
        }
        else
        {
          /* we don't handle surrogates */
          putchar( 0xE0 | ( (FT_UInt)ch >> 12 ) );
          putchar( 0x80 | ( ( (FT_UInt)ch >> 6 ) & 0x3F ) );
          putchar( 0x80 | ( (FT_UInt)ch & 0x3F ) );
        }

        continue;
      }

      switch ( ch )
      {
      case 0x00A9:
        fputs( "(c)", stdout );
        continue;
      case 0x00AE:
        fputs( "(r)", stdout );
        continue;

      case 0x2013:
        fputs( "--", stdout );
        continue;
      case 0x2019:
        fputs( "\'", stdout );
        continue;

      case 0x2122:
        fputs( "(tm)", stdout );
        continue;

      default:
        if ( ch < 128 )
          putchar( ch );
        else
          printf( "\\U+%04X", ch );
        continue;
      }
    }

    if ( ch != '\n' )
      putchar( '"' );
  }


/* End */
