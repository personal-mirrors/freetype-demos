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

extern "C" {

// vertical font coordinates are bottom-up,
// while Qt uses top-down

static int
moveTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->moveTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
lineTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->lineTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
conicTo(const FT_Vector* control,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->quadTo(qreal(control->x) / 64,
               -qreal(control->y) / 64,
               qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
cubicTo(const FT_Vector* control1,
        const FT_Vector* control2,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->cubicTo(qreal(control1->x) / 64,
                -qreal(control1->y) / 64,
                qreal(control2->x) / 64,
                -qreal(control2->y) / 64,
                qreal(to->x) / 64,
                -qreal(to->y) / 64);

  return 0;
}


static FT_Outline_Funcs outlineFuncs =
{
  moveTo,
  lineTo,
  conicTo,
  cubicTo,
  0, // no shift
  0  // no delta
};

} // extern "C"

static const char*  Sample[] =
  {
    "The quick brown fox jumps over the lazy dog",

    /* Luís argüia à Júlia que «brações, fé, chá, óxido, pôr, zângão» */
    /* eram palavras do português */
    "Lu\u00EDs arg\u00FCia \u00E0 J\u00FAlia que \u00ABbra\u00E7\u00F5es, "
    "f\u00E9, ch\u00E1, \u00F3xido, p\u00F4r, z\u00E2ng\u00E3o\u00BB eram "
    "palavras do portugu\u00EAs",

    /* Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει */
    "\u039F \u03BA\u03B1\u03BB\u03CD\u03BC\u03BD\u03B9\u03BF\u03C2 \u03C3"
    "\u03C6\u03BF\u03C5\u03B3\u03B3\u03B1\u03C1\u03AC\u03C2 \u03C8\u03B9"
    "\u03B8\u03CD\u03C1\u03B9\u03C3\u03B5 \u03C0\u03C9\u03C2 \u03B8\u03B1 "
    "\u03B2\u03BF\u03C5\u03C4\u03AE\u03BE\u03B5\u03B9 \u03C7\u03C9\u03C1"
    "\u03AF\u03C2 \u03BD\u03B1 \u03B4\u03B9\u03C3\u03C4\u03AC\u03B6\u03B5"
    "\u03B9",

    /* Съешь ещё этих мягких французских булок да выпей же чаю */
    "\u0421\u044A\u0435\u0448\u044C \u0435\u0449\u0451 \u044D\u0442\u0438"
    "\u0445 \u043C\u044F\u0433\u043A\u0438\u0445 \u0444\u0440\u0430\u043D"
    "\u0446\u0443\u0437\u0441\u043A\u0438\u0445 \u0431\u0443\u043B\u043E"
    "\u043A \u0434\u0430 \u0432\u044B\u043F\u0435\u0439 \u0436\u0435 "
    "\u0447\u0430\u044E",

    /* 天地玄黃，宇宙洪荒。日月盈昃，辰宿列張。寒來暑往，秋收冬藏。*/
    "\u5929\u5730\u7384\u9EC3\uFF0C\u5B87\u5B99\u6D2A\u8352\u3002\u65E5"
    "\u6708\u76C8\u6603\uFF0C\u8FB0\u5BBF\u5217\u5F35\u3002\u5BD2\u4F86"
    "\u6691\u5F80\uFF0C\u79CB\u6536\u51AC\u85CF\u3002",

    /* いろはにほへと ちりぬるを わかよたれそ つねならむ */
    /* うゐのおくやま けふこえて あさきゆめみし ゑひもせす */
    "\u3044\u308D\u306F\u306B\u307B\u3078\u3068 \u3061\u308A\u306C\u308B"
    "\u3092 \u308F\u304B\u3088\u305F\u308C\u305D \u3064\u306D\u306A\u3089"
    "\u3080 \u3046\u3090\u306E\u304A\u304F\u3084\u307E \u3051\u3075\u3053"
    "\u3048\u3066 \u3042\u3055\u304D\u3086\u3081\u307F\u3057 \u3091\u3072"
    "\u3082\u305B\u3059",

    /* 키스의 고유조건은 입술끼리 만나야 하고 특별한 기술은 필요치 않다 */
    "\uD0A4\uC2A4\uC758 \uACE0\uC720\uC870\uAC74\uC740 \uC785\uC220\uB07C"
    "\uB9AC \uB9CC\uB098\uC57C \uD558\uACE0 \uD2B9\uBCC4\uD55C \uAE30"
    "\uC220\uC740 \uD544\uC694\uCE58 \uC54A\uB2E4"
  };



/* static const char*  Sample[0] =
      "The quick brown fox jumps over the lazy dog"
      " 0123456789"
      " \303\242\303\252\303\256\303\273\303\264"
      "\303\244\303\253\303\257\303\266\303\274\303\277"
      "\303\240\303\271\303\251\303\250\303\247"
      " &#~\"\'(-`_^@)=+\302\260"
      " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      " $\302\243^\302\250*\302\265\303\271%!\302\247:/;.,?<> ";*/


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
          double stroke_factor,
          int kern_mode,
          int kern_degree)
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
stroke_factor(stroke_factor),
kerning_mode(kern_mode),
kerning_degree(kern_degree)
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

      
      FT_Stroker_Set( stroker, 32, FT_STROKER_LINECAP_BUTT,
                  FT_STROKER_LINEJOIN_BEVEL, 0x20000 );

    for ( int i = 0; i <2; i++ )
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

      
      FT_Glyph glyph;

      // XXX handle bitmap fonts

      // the `scaler' object is set up by the
      // `update' and `loadFont' methods
      error = FTC_ImageCache_LookupScaler(imageCache,
                                &scaler,
                                FT_LOAD_NO_BITMAP,
                                glyph_idx,
                                &glyph,
                                NULL);
      {
      }

      FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyph);

      FT_Outline* outline = &outlineGlyph->outline;

      FT_BBox cbox;

      FT_Outline_Get_CBox(outline, &cbox);

      QPainterPath path;
      FT_Outline_Decompose(outline, &outlineFuncs, &path);

      painter->drawPath(path);



/* 
      if ( !error && slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        
        


        FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1);

        FT_Outline* outline = engine->loadOutline(glyph_idx); */

       /*  error = FT_Glyph_Stroke( &glyph, stroker, 1 );
        if ( error )
        {
          //FT_Done_Glyph( glyph );
          break;
        } */

        /* error = FT_Get_Glyph( slot, &glyph );
        if ( error )
          break;

        FT_Glyph_Stroke( &glyph, stroker, 1 );

        error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

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
      //}*/
    }
  }
  
  // Render String mode
  if (mode == 4)
  {
    /*
     In UTF-8 encoding:

       The quick brown fox jumps over the lazy dog
       0123456789
       âêîûôäëïöüÿàùéèç
       &#~"'(-`_^@)=+°
       ABCDEFGHIJKLMNOPQRSTUVWXYZ
       $£^¨*µù%!§:/;.,?<>

     The trailing space is for `looping' in case `Sample[0]' gets displayed more
     than once.
   */
    
    const char*  p;
    const char*  pEnd;
    int          ch;

    p    = Sample[0];
    pEnd = p + strlen( Sample[0] ); 

    int length = strlen(Sample[0]);

    for ( int i = 0; i < length; i++ )
    {
      QChar ch = Sample[0][i];

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
    
    int length = strlen(Sample[0]);
    while (y <= 200)
    { 
      int m = 0;
      while ( m < length )
      {

        FT_Glyph  glyph;
        QChar ch = Sample[0][m];
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

  // Kerning comparison
  if (mode == 6)
  {
    /* 1. Print text without kerning
    2. Print text with kerning mode 1
    3. Print text with kerning mode 2 */
    FT_Pos     lsb_delta = 0; /* delta caused by hinting */
    FT_Pos     rsb_delta = 0; /* delta caused by hinting */
    const char*  p;
    const char*  pEnd;
    int          ch;
    FT_Pos   track_kern   = 0;
    FT_Bool use_kerning;
    FT_UInt previous;

    use_kerning = FT_HAS_KERNING( face );
    previous = 0;

    p    = Sample[0];
    pEnd = p + strlen( Sample[0] ); 

    int length = strlen(Sample[0]);

    // if kerning degree > 0
    if ( kerning_degree )
    {
      /* this function needs and returns points, not pixels */
      if ( !FT_Get_Track_Kerning( face,
                                  (FT_Fixed)scaler.width << 10,
                                  -kerning_degree,
                                  &track_kern ) )
      track_kern = (FT_Pos)(
                    ( track_kern / 1024.0 * scaler.x_res ) /
                    72.0 );
    }

    for ( int i = 0; i < length; i++ )
    {
      QChar ch = Sample[0][i];

      // get char index 
      glyph_idx = FT_Get_Char_Index( face , ch.unicode());

      x += track_kern;

      if (previous && glyph_idx )
      {
        FT_Vector delta;

        FT_Get_Kerning( face, previous, glyph_idx,
                        FT_KERNING_UNFITTED, &delta );

        x += delta.x;
        
        if ( kerning_mode > 1 )
        {   
            if ( rsb_delta && rsb_delta - face->glyph->lsb_delta > 32 )
              x -= 1;
            else if ( rsb_delta && rsb_delta - face->glyph->lsb_delta < -31 )
              x += 1;
        }
      }

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

      if (previous)
      {
        lsb_delta = face->glyph->lsb_delta;
        rsb_delta = face->glyph->rsb_delta;
      }
      // space between the glyphs
      x += face->glyph->advance.x/64;

      previous = glyph_idx;
    }
  }
}

// end of RenderAll.cpp
