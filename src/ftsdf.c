
#include <freetype/ftmodapi.h>

#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <time.h>


  typedef FT_Vector  Vec2;
  typedef FT_BBox    Box;


#define FT_CALL( X )                                                \
          do                                                        \
          {                                                         \
            err = X;                                                \
            if ( err != FT_Err_Ok )                                 \
            {                                                       \
              printf( "FreeType error: %s [LINE: %d, FILE: %s]\n",  \
                      FT_Error_String( err ), __LINE__, __FILE__ ); \
              goto Exit;                                            \
            }                                                       \
          } while ( 0 )


  typedef struct  Status_
  {
    FT_Face  face;
    FT_Int   ptsize;
    FT_Int   glyph_index;
    FT_Int   scale;
    FT_Int   spread;
    FT_Int   x_offset;
    FT_Int   y_offset;
    FT_Bool  nearest_filtering;
    float    generation_time;
    FT_Bool  reconstruct;
    FT_Bool  use_bitmap;
    FT_Bool  overlaps;

    /* params for reconstruction */
    float  width;
    float  edge;

  } Status;


  static FTDemo_Handle*   handle  = NULL;
  static FTDemo_Display*  display = NULL;

  static Status  status =
  {
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

/* END */
