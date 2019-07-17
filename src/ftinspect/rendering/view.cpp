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
#define START_X  18 * 8
#define START_Y  3 * 12

#define TRUNC(x) ((x) >> 6)


static const char*  Text =
      "The quick brown fox jumps over the lazy dog"
      " 0123456789"
      " \303\242\303\252\303\256\303\273\303\264"
      "\303\244\303\253\303\257\303\266\303\274\303\277"
      "\303\240\303\271\303\251\303\250\303\247"
      " &#~\"\'(-`_^@)=+\302\260"
      " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      " $\302\243^\302\250*\302\265\303\271%!\302\247:/;.,?<> ";


RenderAll::RenderAll(FT_Face face,
          FT_Size  size,
          FTC_Manager cacheManager,
          FTC_FaceID  face_id,
          FTC_CMapCache  cmap_cache,
          FT_Library lib,
          int render_mode,
          FTC_ScalerRec scaler,
          FTC_ImageCache imageCache,
          double x,
          double y,
          double slant_factor,
          double stroke_factor)
:face(face),
size(size),
cacheManager(cacheManager),
face_id(face_id),
cmap_cache(cmap_cache),
library(lib),
mode(render_mode),
scaler(scaler),
imageCache(imageCache),
x_factor(x),
y_factor(y),
slant_factor(slant_factor),
stroke_factor(stroke_factor)
{
}


RenderAll::~RenderAll()
{
  //FT_Stroker_Done(stroker);
  //FTC_Manager_Done(cacheManager);
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


 // Normal rendering mode
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

      x += face->glyph->advance.x/64;
      // extra space between the glyphs
      x++;
      if (x >= 350)
      { 
        y += (size->metrics.height + 4)/64;
        x = -350;
      }
    }
  }

  // Fancy rendering mode
  if (mode == 2)
  {
    // fancy render
    FT_Matrix shear;
    FT_Pos xstr, ystr;

    shear.xx = 1 << 16;
    shear.xy = (FT_Fixed)( slant_factor * ( 1 << 16 ) );
    shear.yx = 0;
    shear.yy = 1 << 16;
    
    xstr = (FT_Pos)( size->metrics.y_ppem * 64 * x_factor );
    ystr = (FT_Pos)( size->metrics.y_ppem * 64 * y_factor );

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

      x += face->glyph->advance.x/64;
      // extra space between the glyphs
      x++;
      if (x >= 350)
      { 
        y += (size->metrics.height + 4)/64;
        x = -350;
      }
    }
  }

  // Stroked rendering mode
  if (mode == 3)
  {
    FT_Fixed radius;
    FT_Stroker stroker;

    FT_Stroker_New( library, &stroker );
    radius = (FT_Fixed)( size->metrics.y_ppem * 64 * stroke_factor );

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
        error = FT_Render_Glyph(slot,
                                FT_RENDER_MODE_NORMAL);

        QImage glyphImage(slot->bitmap.buffer,
                            slot->bitmap.width,
                            slot->bitmap.rows,
                            slot->bitmap.pitch,
                            QImage::Format_Indexed8);

        

        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; ++i)
        {
          colorTable << qRgba(0, 0, 0, i);
        }
          
        glyphImage.setColorTable(colorTable);
        

        painter->drawImage(x, y,
                          glyphImage, 0, 0, -1, -1);

      x += face->glyph->advance.x/64;
      // extra space between the glyphs
      x++;
      if (x >= 350)
      { 
        y += (size->metrics.height + 4)/64;
        x = -350;
      }
      }
    }
  }
  
  // Render String mode
  if (mode == 4)
  {
    int offset = -1;
    int num_indices = 0;

    /*
     In UTF-8 encoding:

       The quick brown fox jumps over the lazy dog
       0123456789
       âêîûôäëïöüÿàùéèç
       &#~"'(-`_^@)=+°
       ABCDEFGHIJKLMNOPQRSTUVWXYZ
       $£^¨*µù%!§:/;.,?<>

     The trailing space is for `looping' in case `Text' gets displayed more
     than once.
   */
    
    const char*  p;
    const char*  pEnd;
    int          ch;

    p    = Text;
    pEnd = p + strlen( Text ); 

    int length = strlen(Text);

    for ( int i = 0; i < length; i++ )
    {
      QChar ch = Text[i];

      // get char index 
      glyph_idx = FT_Get_Char_Index( face , ch.unicode());
      /* if ( face->charmap->encoding != FT_ENCODING_ORDER )
      {
        glyph_idx = FTC_CMapCache_Lookup(cmap_cache, face_id,
                                          FT_Get_Charmap_Index(face->charmap), (FT_UInt32)ch);
      }
      else
      {
        glyph_idx = (FT_UInt32)ch;
      }*/

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

      x += face->glyph->advance.x/64;
      // extra space between the glyphs
      x++;
      if (x >= 350)
      { 
        y += (size->metrics.height + 4)/64;
        x = -350;
      }
    }
  }

  // Waterfall rendering mode
  if (mode == 5)
  {
    
    int length = strlen(Text);
    while (y <= 200)
    { 
      int m = 0;
      while ( m < length )
      {

        FT_Glyph  glyph;
        QChar ch = Text[m];
        m += 1;

          
        // get char index 
        glyph_idx = FT_Get_Char_Index( face , ch.unicode());

        error = FTC_ImageCache_LookupScaler(imageCache,
                                  &scaler,
                                  FT_LOAD_NO_BITMAP,
                                  glyph_idx,
                                  &glyph,
                                  NULL);

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Glyph( face, glyph_idx, FT_LOAD_DEFAULT );
        if ( error )
        {
          break;  /* ignore errors */
        }

        error = FT_Get_Glyph( slot, &glyph );
        if ( error )
          break;

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

        x += face->glyph->advance.x/64;
        if (x >= 350)
        { 
          break;
        }
      }
      y = y + 50;
      x = -350;
    }
  }
}

// end of RenderAll.cpp
