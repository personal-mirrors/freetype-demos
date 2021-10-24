/****************************************************************************
 *
 * rsvg_port.h
 *
 *   Librsvg based hook functions for OT-SVG rendering in FreeType.
 *   (implementation)
 *
 * Copyright (C) 1996-2021 by
 * David Turner, Robert Wilhelm, Werner Lemberg and Moazin Khatti.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */

#include <cairo.h>
#include <librsvg/rsvg.h>
#include <rsvg_port.h>
#include <ft2build.h>
#include <stdlib.h>
#include <math.h>
#include <freetype/freetype.h>
#include <freetype/ftbbox.h>
#include <freetype/otsvg.h>


  /**
   * The init hook is called when the first OT-SVG glyph is rendered.
   * All we do is allocate an internal state structure and set the pointer
   * in `library->svg_renderer_state`. This state structure becomes very
   * useful to cache some of the results obtained by one hook function that
   * the other one might use.
   */
  FT_Error
  rsvg_port_init( FT_Pointer  *state )
  {
    /* allocate the memory upon initialization */
    *state = malloc( sizeof( Rsvg_Port_StateRec ) );
    return FT_Err_Ok;
  }


  /**
   * Freeing up the state structure.
   */
  void
  rsvg_port_free( FT_Pointer  state)
  {
    /* free the memory of the state structure */
    free( state );
  }

  /**
   * The render hook that's called to render. The job of this hook is to
   * simply render the glyph in the buffer that has been allocated on
   * the FreeType side. Here we simply use the recording surface by playing
   * it back against the surface.
   */

  FT_Error
  rsvg_port_render( FT_GlyphSlot slot, FT_Pointer  _state )
  {
    FT_Error         error = FT_Err_Ok;

    Rsvg_Port_State    state;
    cairo_status_t     status;
    cairo_t            *cr;
    cairo_surface_t    *surface;

    state = (Rsvg_Port_State)_state;

    /* create an image surface to store the rendered image, however,
     * don't allocate memory, instead use the space already provided
     * in `slot->bitmap.buffer'.
     */
    surface = cairo_image_surface_create_for_data( slot->bitmap.buffer,
                                                   CAIRO_FORMAT_ARGB32,
                                                   slot->bitmap.width,
                                                   slot->bitmap.rows,
                                                   slot->bitmap.pitch );
    status = cairo_surface_status( surface );

    if ( status != CAIRO_STATUS_SUCCESS )
    {
      if ( status == CAIRO_STATUS_NO_MEMORY )
        return FT_Err_Out_Of_Memory;
      else
        return FT_Err_Invalid_Outline;
    }

    cr     = cairo_create( surface );
    status = cairo_status( cr );

    if ( status != CAIRO_STATUS_SUCCESS )
    {
      if ( status == CAIRO_STATUS_NO_MEMORY )
        return FT_Err_Out_Of_Memory;
      else
        return FT_Err_Invalid_Outline;
    }

    /* set a translate transform that translates the points in such
     * a way that we get a tight rendering with least redundant white
     * space */
    cairo_translate( cr, -1 * (state->x), -1 * (state->y) );
    /* replay from the recorded surface. Saves us from parsing the document
     * again and redoing this already done in the preset hook. */
    cairo_set_source_surface( cr, state->rec_surface, 0.0, 0.0 );
    cairo_paint( cr );

    cairo_surface_flush( surface );

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    slot->bitmap.num_grays  = 256;
    slot->format = FT_GLYPH_FORMAT_BITMAP;

    /* clean everything */
    cairo_surface_destroy( surface );
    cairo_destroy( cr );
    cairo_surface_destroy( state->rec_surface );
    return error;
  }

  /**
   * This hook is called at two different locations. Firstly, it is called
   * when presetting the glyphslot when FT_Load_Glyph is called. Secondly, it is
   * called right before the render hook is called. When `cache` is false, it's
   * the former while when `cache` is true, it's the latter.
   *
   * The job of this function is to preset the slot setting the width, height,
   * pitch, bitmap.left and bitmap.top. These are all necessary for appropriate
   * memory allocation as well as ultimately compositing the glyph later on by
   * client applications.
   */
  FT_Error
  rsvg_port_preset_slot( FT_GlyphSlot  slot, FT_Bool  cache, FT_Pointer _state )
  {
    /* FreeType variables */
    FT_Error         error          = FT_Err_Ok;
    FT_SVG_Document  document       = (FT_SVG_Document)slot->other;
    FT_Size_Metrics  metrics        = document->metrics;
    FT_UShort        units_per_EM   = document->units_per_EM;
    FT_UShort        end_glyph_id   = document->end_glyph_id;
    FT_UShort        start_glyph_id = document->start_glyph_id;

    /* Librsvg variables */
    GError             *gerror = NULL;
    gboolean           ret;
    gboolean           out_has_width;
    gboolean           out_has_height;
    gboolean           out_has_viewbox;
    RsvgHandle         *handle;
    RsvgLength         out_width;
    RsvgLength         out_height;
    RsvgRectangle      out_viewbox;
    RsvgDimensionData  dimension_svg;
    cairo_t            *rec_cr;
    cairo_matrix_t     transform_matrix;

    /* Rendering port's state */
    Rsvg_Port_State     state;
    Rsvg_Port_StateRec  state_dummy;

    /* general variables */
    double  x;
    double  y;
    double  xx;
    double  xy;
    double  yx;
    double  yy;
    double  x0;
    double  y0;
    double  width;
    double  height;
    double  x_svg_to_out;
    double  y_svg_to_out;

    /* if cache is `TRUE` we store calculations in the actual port
     * state variable, otherwise we just create a dummy variable and
     * store there. This saves from too many if statements.
     */
    if ( cache )
      state = (Rsvg_Port_State)_state;
    else
      state = &state_dummy;

    /* form an RsvgHandle by loading the SVG document */
    handle = rsvg_handle_new_from_data( document->svg_document,
                                        document->svg_document_length,
                                        &gerror );
    if ( handle == NULL )
    {
      error = FT_Err_Invalid_SVG_Document;
      goto CleanLibrsvg;
    }

    /* get attributes like `viewBox' and `width/height'. */
    rsvg_handle_get_intrinsic_dimensions( handle,
                                          &out_has_width,
                                          &out_width,
                                          &out_has_height,
                                          &out_height,
                                          &out_has_viewbox,
                                          &out_viewbox );

    /**
     * Figure out the units in the EM square in the SVG document. This is
     * specified by the ViewBox or the width/height attributes, if present,
     * otherwise it should be assumed that the units in the EM square are
     * the same as in the TTF/CFF outlines.
     *
     * TODO: I'm not sure what the standard says about the situation if
     * the ViewBox as well as width/height are present, however, I've never
     * seen that situation in real fonts.
     */
    if ( out_has_viewbox == TRUE )
    {
      dimension_svg.width  = out_viewbox.width;
      dimension_svg.height = out_viewbox.height;
    }
    else if ( ( out_has_width == TRUE ) && ( out_has_height == TRUE ) )
    {
      dimension_svg.width  = out_width.length;
      dimension_svg.height = out_height.length;
    }
    else
    {
      /* if no `viewBox' and `width/height' are present, the `units_per_EM'
       * in SVG coordinates must be the same as `units_per_EM' of the TTF/CFF
       * outlines.
       */
      dimension_svg.width  = units_per_EM;
      dimension_svg.height = units_per_EM;
    }

    /* scale factors from SVG coordinates to the output size needed */
    x_svg_to_out = (float)metrics.x_ppem/(float)dimension_svg.width;
    y_svg_to_out = (float)metrics.y_ppem/(float)dimension_svg.height;

    /* create a cairo recording surface. This is done for two reasons. Firstly,
     * it is required to get the bounding box of the final drawing so we can
     * use appropriate translate transform to get a tight rendering. Secondly,
     * if `cache' is true, we can save this surface and later replay it
     * against an image surface for the final rendering. This saves us from
     * loading and parsing the document again.
     */
    state->rec_surface = cairo_recording_surface_create( CAIRO_CONTENT_COLOR_ALPHA,
                                                         NULL );

    rec_cr = cairo_create( state->rec_surface );


    /* we need to take into account any transformations applied. The end
     * user who applied the transformation doesn't know the internal details
     * of the SVG document. Thus, we expect that the end user should just
     * write the transformation as if the glyph is a traditional one. We then,
     * do some maths on this to get the equivalent transformation in SVG
     * coordinates.
     */
    xx =  1 * (((double)document->transform.xx)/((double)(1 << 16 )));
    xy = -1 * (((double)document->transform.xy)/((double)(1 << 16 )));
    yx = -1 * (((double)document->transform.yx)/((double)(1 << 16 )));
    yy =  1 * (((double)document->transform.yy)/((double)(1 << 16 )));
    x0 =  1 * ((double)document->delta.x/(double)(1 << 6)) *
              ((double)dimension_svg.width/(double)metrics.x_ppem);
    y0 = -1 * ((double)document->delta.y/(double)(1 << 6)) *
              ((double)dimension_svg.height/(double)metrics.y_ppem);

    /* cairo stores transformation and translate both in one matrix */
    transform_matrix.xx = xx;
    transform_matrix.yx = yx;
    transform_matrix.xy = xy;
    transform_matrix.yy = yy;
    transform_matrix.x0 = x0;
    transform_matrix.y0 = y0;

    /* set up a scale transfromation to scale up the document to the
     * required output size */
    cairo_scale( rec_cr, x_svg_to_out, y_svg_to_out );
    /* setup a transformation matrix */
    cairo_transform( rec_cr, &transform_matrix );

    /* if the document contains only one glyph, `start_glyph_id' and
     * `end_glyph_id' will have the same value, else, `end_glyph_id'
     * will be higher. */
    if ( start_glyph_id == end_glyph_id )
    {
      /* render the whole document to the recording surface */
      ret = rsvg_handle_render_cairo ( handle, rec_cr );
      if ( ret == FALSE )
      {
        error = FT_Err_Invalid_SVG_Document;
        goto CleanCairo;
      }
    }
    else if ( start_glyph_id < end_glyph_id )
    {
      int    length;
      char*  str;

      length = snprintf( NULL, 0, "%u", slot->glyph_index );
      str    = (char*)malloc( 6 + length + 1 );
      strcpy( str, "#glyph");
      snprintf( str + 6, length + 1, "%u", slot->glyph_index );
      /* render only the element with `id' equal to `glyph<ID>' */
      ret = rsvg_handle_render_cairo_sub( handle, rec_cr, str );
      free(str);
      if ( ret == FALSE )
      {
        error = FT_Err_Invalid_SVG_Document;
        goto CleanCairo;
      }
    }

    /* get the bounding box of the drawing */
    cairo_recording_surface_ink_extents( state->rec_surface, &x, &y,
                                         &width, &height);

    /* we store the bounding box's `x' and `y' so that the render hook
     * can apply a translation to get a tight rendering.
     */
    state->x            = x;
    state->y            = y;

    /* preset the values */
    slot->bitmap_left   = state->x ;
    slot->bitmap_top    = (state->y) * -1;
    slot->bitmap.rows   = ceil( height );
    slot->bitmap.width  = ceil( width );
    slot->bitmap.pitch  = slot->bitmap.width * 4;

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    /* if a render call is to follow, just destroy the context for the
     * recording surface, since no more drawing will be done on it, but,
     * keep the surface itself for use by the render hook.
     */
    if ( cache == TRUE )
    {
      cairo_destroy( rec_cr );
      goto CleanLibrsvg;
    }

    /* destroy the recording surface as well as the context */
  CleanCairo:
    cairo_surface_destroy( state->rec_surface );
    cairo_destroy( rec_cr );
  CleanLibrsvg:
    /* destroy the handle */
    g_object_unref( handle );
    return error;
  }
