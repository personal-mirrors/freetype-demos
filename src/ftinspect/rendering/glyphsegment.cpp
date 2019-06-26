#include "glyphsegment.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>


/* these variables, structures, and declarations are for  */
/* communication with the debugger in the autofit module; */
/* normal programs don't need this */
struct  AF_GlyphHintsRec_;
typedef struct AF_GlyphHintsRec_*  AF_GlyphHints;
extern AF_GlyphHints  _af_debug_hints;

#ifdef __cplusplus
  extern "C" {
#endif
  extern void
  af_glyph_hints_dump_segments( AF_GlyphHints  hints,
                                FT_Bool        to_stdout );
  extern void
  af_glyph_hints_dump_points( AF_GlyphHints  hints,
                              FT_Bool        to_stdout );
  extern void
  af_glyph_hints_dump_edges( AF_GlyphHints  hints,
                             FT_Bool        to_stdout );
  extern FT_Error
  af_glyph_hints_get_num_segments( AF_GlyphHints  hints,
                                   FT_Int         dimension,
                                   FT_Int*        num_segments );
  extern FT_Error
  af_glyph_hints_get_segment_offset( AF_GlyphHints  hints,
                                     FT_Int         dimension,
                                     FT_Int         idx,
                                     FT_Pos        *offset,
                                     FT_Bool       *is_blue,
                                     FT_Pos        *blue_offset );
#ifdef __cplusplus
  }
#endif


GlyphSegment::GlyphSegment(const QPen& segmentP,
                           const QPen& bluezoneP,
                           FT_Size fts)
: segmentPen(segmentP),
  bluezonePen(bluezoneP),
  ftsize(fts)
{
}


QRectF
GlyphSegment::boundingRect() const
{
  return QRectF(-100, -100,
                200, 200);
}


void
GlyphSegment::paint(QPainter* painter,
            const QStyleOptionGraphicsItem* option,
            QWidget*)
{

  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  if (lod >= 5)
  {
    FT_Fixed  x_scale = ftsize->metrics.x_scale;
    FT_Fixed  y_scale = ftsize->metrics.y_scale;

    FT_Int  dimension;
    int     x_org = 0;
    int     y_org = 0;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      FT_Int  num_seg;
      FT_Int  count;

      af_glyph_hints_get_num_segments( _af_debug_hints, dimension, &num_seg );

      for ( count = 0; count < num_seg; count++ )
      {
        int      pos;
        FT_Pos   offset;
        FT_Bool  is_blue;
        FT_Pos   blue_offset;

        painter->setPen(segmentPen);

        af_glyph_hints_get_segment_offset( _af_debug_hints, dimension,
                                              count, &offset,
                                              &is_blue, &blue_offset);
        
        if ( dimension == 0 )
        {
          offset = FT_MulFix( offset, x_scale );
          pos    = x_org + ( ( offset) >> 6 );
          painter->drawLine(pos, -100, pos, 100);
        }
        else
        {
          offset = FT_MulFix( offset, y_scale );
          pos    = y_org - ( ( offset) >> 6 );

          if ( is_blue )
          {
            int  blue_pos;


            blue_offset = FT_MulFix( blue_offset, y_scale );
            blue_pos    = y_org - ( ( blue_offset) >> 6 );
            if ( blue_pos == pos )
            {
              painter->setPen(bluezonePen);
              painter->drawLine(-100, blue_pos, 100, blue_pos);
            }
            else
            {
              painter->setPen(bluezonePen);
              painter->drawLine(-100, blue_pos, 100, blue_pos);
              painter->setPen(segmentPen);
              painter->drawLine(-100, pos, 100, pos);
            }
          }
          else
          {
            painter->drawLine(-100, pos, 100, pos);
          }
        }
      }
    }
  }
}


// end of grid.cpp