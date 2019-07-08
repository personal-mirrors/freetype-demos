#include "view.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QFile>
#include <QImage>
#include <iostream>
#include <QtDebug>

#define ft_render_mode_normal  FT_RENDER_MODE_NORMAL
  /* special encoding to display glyphs in order */
#define FT_ENCODING_ORDER  0xFFFF
#define ft_encoding_unicode         FT_ENCODING_UNICODE

#define TRUNC(x) ((x) >> 6)


RenderAll::RenderAll(FT_Face face,
          FT_Size  size,
          FTC_Manager cacheManager,
          FTC_FaceID  face_id,
          FTC_CMapCache  cmap_cache,
          FT_Library lib,
          int render_mode)
:face(face),
size(size),
cacheManager(cacheManager),
face_id(face_id),
cmap_cache(cmap_cache),
library(lib),
mode(render_mode)
{
}


RenderAll::~RenderAll()
{
  //FT_Done_Face(face);
}

QRectF
RenderAll::boundingRect() const
{
  return QRectF(-350, -200,
                700, 400);
}


void
RenderAll::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  // Basic def
  FT_GlyphSlot  slot;
  FT_Int cmap_index;

  
  // Basic def or just pass size 
  slot = face->glyph;
  FT_UInt  glyph_idx;
  int x = -350;
  int y = -180;
  
  if (mode == 1)
  {
    // Normal rendering
    for ( int i = 0; i < face->num_glyphs; i++ )
    {
      // get char index
      //glyph_idx = FT_Get_Char_Index( face , (FT_ULong)i );
      if ( face->charmap->encoding != FT_ENCODING_ORDER )
      {
        glyph_idx = FTC_CMapCache_Lookup(cmap_cache, face_id,
                                          FT_Get_Charmap_Index(face->charmap), (FT_UInt32)i);
      }
      else
      {
        glyph_idx = (FT_UInt32)i;
      }
      //glyph_idx = (FT_UInt)i;
      /* load glyph image into the slot (erase previous one) */
      error = FT_Load_Glyph( face, glyph_idx, FT_LOAD_DEFAULT );
      if ( error )
      {
        break;  /* ignore errors */
      } 

      error = FT_Render_Glyph(face->glyph,
                                FT_RENDER_MODE_NORMAL);

      QImage glyphImage(face->glyph->bitmap.buffer,
                          face->glyph->bitmap.width,
                          face->glyph->bitmap.rows,
                          face->glyph->bitmap.pitch,
                          QImage::Format_Indexed8);

      

      QVector<QRgb> colorTable;
      for (int i = 0; i < 256; ++i)
      {
        colorTable << qRgba(0, 0, 0, i);
      }
        
      glyphImage.setColorTable(colorTable);
      

      painter->drawImage(x, y,
                        glyphImage, 0, 0, -1, -1);
      x = x + 20;

      if (x >= 350)
      { 
        y = y + 30;
        x = -350;
      }
    }
  }

  if (mode == 2)
  {
    // fancy render
    FT_Matrix shear;
    FT_Pos xstr, ystr;

    shear.xx = 1 << 16;
    shear.xy = (FT_Fixed)( ( 1 << 16 ) );
    shear.yx = 0;
    shear.yy = 1 << 16;

    xstr = (FT_Pos)( size->metrics.y_ppem * 64 * 0 );
    ystr = (FT_Pos)( size->metrics.y_ppem * 64 * 0 ); 

    for ( int i = 0; i < face->num_glyphs; i++ )
    {
      // get char index 
      //glyph_idx = FT_Get_Char_Index( face , (FT_ULong)i );
      if ( face->charmap->encoding != FT_ENCODING_ORDER )
      {
        glyph_idx = FTC_CMapCache_Lookup(cmap_cache, face_id,
                                          FT_Get_Charmap_Index(face->charmap), (FT_UInt32)i);
      }
      else
      {
        glyph_idx = (FT_UInt32)i;
      }

      /* load glyph image into the slot (erase previous one) */
      error = FT_Load_Glyph( face, glyph_idx, FT_LOAD_DEFAULT );
      if ( error )
      {
        break;  /* ignore errors */
      }

      /* this is essentially the code of function */
      /* `FT_GlyphSlot_Embolden'                  */
      if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        FT_Outline_Transform( &slot->outline, &shear );

        error = FT_Outline_EmboldenXY( &slot->outline, xstr, ystr );
        /* ignore error */
      }
      else if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
      {
        /* round to full pixels */
        xstr &= ~63;
        ystr &= ~63;

        error = FT_GlyphSlot_Own_Bitmap( slot );
        if ( error )
          break;

        error = FT_Bitmap_Embolden( slot->library, &slot->bitmap,
                                    xstr, ystr );
        if ( error )
          break;
      }
      else
      {
        break;
      }

      if ( slot->advance.x )
          slot->advance.x += xstr;

      if ( slot->advance.y )
        slot->advance.y += ystr;

      slot->metrics.width        += xstr;
      slot->metrics.height       += ystr;
      slot->metrics.horiAdvance  += xstr;
      slot->metrics.vertAdvance  += ystr;

      if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
        slot->bitmap_top += ystr >> 6;

      error = FT_Render_Glyph(face->glyph,
                                FT_RENDER_MODE_NORMAL);

      QImage glyphImage(face->glyph->bitmap.buffer,
                          face->glyph->bitmap.width,
                          face->glyph->bitmap.rows,
                          face->glyph->bitmap.pitch,
                          QImage::Format_Indexed8);

      

      QVector<QRgb> colorTable;
      for (int i = 0; i < 256; ++i)
      {
        colorTable << qRgba(0, 0, 0, i);
      }
        
      glyphImage.setColorTable(colorTable);
      

      painter->drawImage(x, y,
                        glyphImage, 0, 0, -1, -1);
      x = x + 20;

      if (x >= 350)
      { 
        y = y + 30;
        x = -350;
      }
    }
  }

  // Stroked mode
  if (mode == 3)
  {
    FT_Fixed radius;
    FT_Stroker stroker;

    FT_Stroker_New( library, &stroker );
    radius = (FT_Fixed)( size->metrics.y_ppem * 64 * 0.2 );

    FT_Stroker_Set( stroker, radius,
                    FT_STROKER_LINECAP_ROUND,
                    FT_STROKER_LINEJOIN_ROUND,
                    0 );

    for ( int i = 0; i < face->num_glyphs; i++ )
    {
      // get char index 
      //glyph_idx = FT_Get_Char_Index( face , (FT_ULong)i );
      if ( face->charmap->encoding != FT_ENCODING_ORDER )
      {
        glyph_idx = FTC_CMapCache_Lookup(cmap_cache, face_id,
                                          FT_Get_Charmap_Index(face->charmap), (FT_UInt32)i);
      }
      else
      {
        glyph_idx = (FT_UInt32)i;
      }

      /* load glyph image into the slot (erase previous one) */
      error = FT_Load_Glyph( face, glyph_idx, FT_LOAD_DEFAULT );
      if ( error )
      {
        break;  /* ignore errors */
      }

      if ( !error && slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        FT_Glyph  glyph;

        error = FT_Get_Glyph( slot, &glyph );
        if ( error )
          break;
        error = FT_Glyph_Stroke( &glyph, stroker, 1 );
        if ( error )
        {
          //FT_Done_Glyph( glyph );
          break;
        }
        error = FT_Render_Glyph(face->glyph,
                                FT_RENDER_MODE_NORMAL);

        QImage glyphImage(face->glyph->bitmap.buffer,
                            face->glyph->bitmap.width,
                            face->glyph->bitmap.rows,
                            face->glyph->bitmap.pitch,
                            QImage::Format_Indexed8);

        

        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; ++i)
        {
          colorTable << qRgba(0, 0, 0, i);
        }
          
        glyphImage.setColorTable(colorTable);
        

        painter->drawImage(x, y,
                          glyphImage, 0, 0, -1, -1);
        x = x + 20;

        if (x >= 350)
        { 
          y = y + 30;
          x = -350;
        }
      }
    }
    FT_Stroker_Done( stroker );
  }
}

// end of RenderAll.cpp
