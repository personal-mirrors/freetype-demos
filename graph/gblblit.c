/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 1996-2019 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  grblit.c: Support for alpha blending with gamma correction and caching. */
/*                                                                          */
/****************************************************************************/


#include "grobjs.h"
#include "gblblit.h"

/* blitting gray glyphs
 */

/* generic macros
 */
#define  GRGB_PACK(r,g,b)      ( ((GBlenderPixel)(r) << 16) | \
                                 ((GBlenderPixel)(g) <<  8) | \
                                  (GBlenderPixel)(b)        )

#define  GDST_STORE3(d,r,g,b)  (d)[0] = (unsigned char)(r); \
                               (d)[1] = (unsigned char)(g); \
                               (d)[2] = (unsigned char)(b)

/* */

#define  GRGB_TO_RGB565(r,g,b)   ((unsigned short)( (((r) << 8) & 0xF800) |  \
                                                    (((g) << 3) & 0x07E0) |  \
                                                    (((b) >> 3) & 0x001F) ) )

#define  GRGB565_TO_RGB24(p)   ( ( ((p) << 8) & 0xF80000 ) |             \
                                 ( ((p) << 3) & 0x0700F8 ) |             \
                                 ( ((p) << 5) & 0x00FC00 ) |             \
                                 ( ((p) >> 1) & 0x000300 ) |             \
                                 ( ((p) >> 2) & 0x000007 ) )

#define  GRGB24_TO_RGB565(p)   ( (unsigned short)( (((p) >> 8) & 0xF800 ) |   \
                                                   (((p) >> 5) & 0x07E0 ) |   \
                                                   (((p) >> 3) & 0x001F ) ) )

/* */

#define  GRGB_TO_BGR565(r,g,b)    GRGB_TO_RGB565(b,g,r)

#define  GBGR565_TO_RGB24(p)   ( ( ((p) << 19) & 0xF80000 ) |             \
                                 ( ((p) << 12) & 0x070000 ) |             \
                                 ( ((p) <<  5) & 0x00FC00 ) |             \
                                 ( ((p) >>  1) & 0x000300 ) |             \
                                 ( ((p) >>  8) & 0x0000F8 ) |             \
                                 ( ((p) >> 13) & 0x000007 ) )

#define  GRGB24_TO_BGR565(p)   ( (unsigned short)( (((p) <<  8) & 0xF800 ) |  \
                                                   (((p) >>  5) & 0x07E0 ) |  \
                                                   (((p) >> 19) & 0x001F ) ) )

/* */

#define  GRGB_TO_GRAY8(r,g,b)  ( (unsigned char)( ( 2*(r) + 7*(g) + (b) ) / 10 ) )

#define  GRGB24_TO_GRAY8(p)   ( (unsigned char)( ( 2*( ((p) >> 16) & 0xFF ) +         \
                                                   7*( ((p) >>  8) & 0xFF ) +         \
                                                     ( ((p))       & 0xFF ) ) / 10 ) )

/* */

/* Rgb32 blitting routines
 */

#define  GDST_TYPE                rgb32
#define  GDST_INCR                4
#define  GDST_READ(d,p)           (p) = *(GBlenderPixel*)(d) & 0xFFFFFF
#define  GDST_COPY(d)             *(GBlenderPixel*)(d) = color
#define  GDST_STOREP(d,cells,a)   *(GBlenderPixel*)(d) = (cells)[(a)]
#define  GDST_STOREB(d,cells,a)              \
  {                                          \
    GBlenderCell*  _g = (cells) + (a)*3;     \
                                             \
    GDST_STOREC(d,_g[0],_g[1],_g[2]);        \
  }
#define  GDST_STOREC(d,r,g,b)     *(GBlenderPixel*)(d) = GRGB_PACK(r,g,b)
#define  GDST_COPY_VAR            /* nothing */

#include "gblany.h"

/* Rgb24 blitting routines
 */

#define  GDST_TYPE                 rgb24
#define  GDST_INCR                 3
#define  GDST_READ(d,p)            (p) = GRGB_PACK((d)[0],(d)[1],(d)[2])
#define  GDST_COPY(d)              GDST_STORE3(d,r,g,b)
#define  GDST_STOREC(d,r,g,b)      GDST_STORE3(d,r,g,b)

#define  GDST_STOREB(d,cells,a)                \
    {                                          \
      GBlenderCell*  _g = (cells) + (a)*3;     \
                                               \
      (d)[0] = _g[0];                          \
      (d)[1] = _g[1];                          \
      (d)[2] = _g[2];                          \
    }

#define  GDST_STOREP(d,cells,a)                 \
    {                                           \
      GBlenderPixel  _pix = (cells)[(a)];       \
                                                \
      GDST_STORE3(d,_pix >> 16,_pix >> 8,_pix); \
    }

#define  GDST_COPY_VAR            /* nothing */

#include "gblany.h"

/* Rgb565 blitting routines
 */

#define  GDST_TYPE               rgb565
#define  GDST_INCR               2

#define  GDST_READ(d,p)          p = (GBlenderPixel)*(unsigned short*)(d);  \
                                 p = GRGB565_TO_RGB24(p)

#define  GDST_COPY_VAR           unsigned short  pix = GRGB_TO_RGB565(r,g,b);
#define  GDST_COPY(d)            *(unsigned short*)(d) = pix

#define  GDST_STOREB(d,cells,a)                                   \
    {                                                             \
      GBlenderCell*  _g = (cells) + (a)*3;                        \
                                                                  \
      *(unsigned short*)(d) = GRGB_TO_RGB565(_g[0],_g[1],_g[2]);  \
    }

#define  GDST_STOREP(d,cells,a)                         \
    {                                                   \
      GBlenderPixel  _pix = (cells)[(a)];               \
                                                        \
      *(unsigned short*)(d) = GRGB24_TO_RGB565(_pix);   \
    }

#define  GDST_STOREC(d,r,g,b)   *(unsigned short*)(d) = GRGB_TO_RGB565(r,g,b)

#include "gblany.h"

/* Bgr565 blitting routines
 */
#define  GDST_TYPE               bgr565
#define  GDST_INCR               2

#define  GDST_READ(d,p)          p = (GBlenderPixel)*(unsigned short*)(d);  \
                                 p = GBGR565_TO_RGB24(p)

#define  GDST_COPY_VAR           unsigned short  pix = GRGB_TO_BGR565(r,g,b);
#define  GDST_COPY(d)            *(d) = (unsigned char)pix

#define  GDST_STOREB(d,cells,a)                                   \
    {                                                             \
      GBlenderCell*  _g = (cells) + (a)*3;                        \
                                                                  \
      *(unsigned short*)(d) = GRGB_TO_BGR565(_g[0],_g[1],_g[2]);  \
    }

#define  GDST_STOREP(d,cells,a)                         \
    {                                                   \
      GBlenderPixel  _pix = (cells)[(a)];               \
                                                        \
      *(unsigned short*)(d) = GRGB24_TO_BGR565(_pix);   \
    }

#define  GDST_STOREC(d,r,g,b)   *(unsigned short*)(d) = GRGB_TO_BGR565(r,g,b)

#include "gblany.h"

/* Gray8 blitting routines
 */
#define  GDST_TYPE               gray8
#define  GDST_INCR               1
#define  GDST_READ(d,p)          (p) = GRGB_PACK((d)[0],(d)[0],(d)[0])

#define  GDST_COPY_VAR           /* nothing */
#define  GDST_COPY(d)            *(d) = GRGB_TO_GRAY8(r,g,b)

#define  GDST_STOREB(d,cells,a)                 \
    {                                           \
      GBlenderCell*  _g = (cells) + (a)*3;      \
                                                \
      *(d) = GRGB_TO_GRAY8(_g[0],_g[1],_g[2]);  \
    }

#define  GDST_STOREP(d,cells,a)           \
    {                                     \
      GBlenderPixel  _pix = (cells)[(a)]; \
                                          \
      *(d) = GRGB24_TO_GRAY8(_pix);       \
    }

#define  GDST_STOREC(d,r,g,b)   *(d) = GRGB_TO_GRAY8(r,g,b)

#include "gblany.h"

/* */

static const GBlenderBlitFunc*
blit_funcs[GBLENDER_TARGET_MAX] =
{
  blit_funcs_gray8,
  blit_funcs_rgb32,
  blit_funcs_rgb24,
  blit_funcs_rgb565,
  blit_funcs_bgr565
};


static void
_gblender_blit_dummy( GBlenderBlit   blit,
                      GBlenderPixel  color )
{
  (void)blit;
  (void)color;
}


static int
gblender_blit_init( GBlenderBlit           blit,
                    int                    dst_x,
                    int                    dst_y,
                    grSurface*             surface,
                    grBitmap*              glyph )
{
  int               src_x = 0;
  int               src_y = 0;
  int               delta;

  grBitmap*  target = (grBitmap*)surface;

  GBlenderSourceFormat   src_format;
  const unsigned char*   src_buffer = glyph->buffer;
  int                    src_pitch  = glyph->pitch;
  int                    src_width  = glyph->width;
  int                    src_height = glyph->rows;
  GBlenderTargetFormat   dst_format;
  unsigned char*         dst_buffer = target->buffer;
  int                    dst_pitch  = target->pitch;
  int                    dst_width  = target->width;
  int                    dst_height = target->rows;


  if ( glyph->grays != 256 )
    return -2;

  switch ( glyph->mode )
  {
  case gr_pixel_mode_gray:  src_format = GBLENDER_SOURCE_GRAY8; break;
  case gr_pixel_mode_lcd:   src_format = GBLENDER_SOURCE_HRGB;  break;
  case gr_pixel_mode_lcdv:  src_format = GBLENDER_SOURCE_VRGB;  break;
  case gr_pixel_mode_lcd2:  src_format = GBLENDER_SOURCE_HBGR;  break;
  case gr_pixel_mode_lcdv2: src_format = GBLENDER_SOURCE_VBGR;  break;
  case gr_pixel_mode_bgra:  src_format = GBLENDER_SOURCE_BGRA;  break;
  default:
    return -2;
  }

  switch ( target->mode )
  {
  case gr_pixel_mode_gray:   dst_format = GBLENDER_TARGET_GRAY8; break;
  case gr_pixel_mode_rgb32:  dst_format = GBLENDER_TARGET_RGB32; break;
  case gr_pixel_mode_rgb24:  dst_format = GBLENDER_TARGET_RGB24; break;
  case gr_pixel_mode_rgb565: dst_format = GBLENDER_TARGET_RGB565; break;
  default:
    return -2;
  }

  blit->blender   = surface->gblender;
  blit->blit_func = blit_funcs[dst_format][src_format];

  if ( blit->blit_func == 0 )
  {
   /* unsupported blit mode
    */
    blit->blit_func = _gblender_blit_dummy;
    return -2;
  }

  if ( glyph->mode == gr_pixel_mode_lcd  ||
       glyph->mode == gr_pixel_mode_lcd2 )
    src_width /= 3;

  if ( glyph->mode == gr_pixel_mode_lcdv  ||
       glyph->mode == gr_pixel_mode_lcdv2 )
    src_height /= 3;

  if ( dst_x < 0 )
  {
    src_width += dst_x;
    src_x     -= dst_x;
    dst_x      = 0;
  }

  delta = dst_x + src_width - dst_width;
  if ( delta > 0 )
    src_width -= delta;

  if ( dst_y < 0 )
  {
    src_height += dst_y;
    src_y      -= dst_y;
    dst_y       = 0;
  }

  delta = dst_y + src_height - dst_height;
  if ( delta > 0 )
    src_height -= delta;

 /* nothing to blit
  */
  if ( src_width <= 0 || src_height <= 0 )
  {
    blit->blit_func = _gblender_blit_dummy;
    return -1;
  }

  blit->width      = src_width;
  blit->height     = src_height;
  blit->src_format = src_format;
  blit->dst_format = dst_format;

  blit->src_x     = src_x;
  blit->src_y     = src_y;
  blit->src_line  = src_buffer + src_pitch*src_y;
  blit->src_pitch = src_pitch;
  if ( src_pitch < 0 )
    blit->src_line -= (src_height-1)*src_pitch;

  blit->dst_x     = dst_x;
  blit->dst_y     = dst_y;
  blit->dst_line  = dst_buffer + dst_pitch*dst_y;
  blit->dst_pitch = dst_pitch;
  if ( dst_pitch < 0 )
    blit->dst_line -= (dst_height-1)*dst_pitch;

  return 0;
}


GBLENDER_APIDEF( int )
grBlitGlyphToSurface( grSurface*  surface,
                      grBitmap*   glyph,
                      grPos       x,
                      grPos       y,
                      grColor     color )
{
  GBlenderBlitRec       gblit[1];
  GBlenderPixel         gcolor;


  /* check arguments */
  if ( !surface || !glyph )
  {
    grError = gr_err_bad_argument;
    return -1;
  }

  if ( !glyph->rows || !glyph->width )
  {
    /* nothing to do */
    return 0;
  }

  switch ( gblender_blit_init( gblit, x, y, surface, glyph ) )
  {
  case -1: /* nothing to do */
    return 0;
  case -2:
    return -1;
  }

  gcolor = ((GBlenderPixel)color.chroma[0] << 16) |
           ((GBlenderPixel)color.chroma[1] << 8 ) |
           ((GBlenderPixel)color.chroma[2]      ) ;

  gblender_blit_run( gblit, gcolor );
  return 1;
}


GBLENDER_APIDEF( void )
grSetTargetGamma( grBitmap*  target,
                  double     gamma )
{
  grSurface*  surface = (grSurface*)target;


  gblender_init( surface->gblender, gamma );

  /* not related to gamma but needs to be set */
  switch ( target->mode )
  {
  case gr_pixel_mode_gray:
    surface->gray_spans = _gblender_spans_gray8;
    break;
  case gr_pixel_mode_rgb32:
    surface->gray_spans = _gblender_spans_rgb32;
    break;
  case gr_pixel_mode_rgb24:
    surface->gray_spans = _gblender_spans_rgb24;
    break;
  case gr_pixel_mode_rgb565:
    surface->gray_spans = _gblender_spans_rgb565;
    break;
  default:
    (void)_gblender_spans_bgr565;  /* unused */
    surface->gray_spans = (grSpanFunc)0;
  }
}
