
  GBLENDER_VARS;

  unsigned char*  dst_origin = surface->origin - y * surface->bitmap.pitch;

  gblender_use_channels( blender, 0 );

  GBLENDER_VARS_SET(blender,color);

  /* make compiler happy */
  (void)(r);
  (void)(g);
  (void)(b);

  for ( ; count--; spans++ )
  {
    unsigned char*  dst = dst_origin + spans->x * GDST_INCR;
    unsigned short  w   = spans->len;
    int             a   = GBLENDER_SHADE_INDEX( spans->coverage );

    if ( a == GBLENDER_SHADE_COUNT-1 )
      for ( ; w-- ; dst += GDST_INCR )
      {
        GDST_COPY(dst);
      }
    else if ( a )
      for ( ; w-- ; dst += GDST_INCR )
      {
        GBlenderPixel  back;

        GDST_READ(dst,back);

        GBLENDER_LOOKUP( blender, back );

#ifdef GBLENDER_STORE_BYTES
        GDST_STOREB(dst,_gcells,a);
#else
        GDST_STOREP(dst,_gcells,a);
#endif
      }
  }

  GBLENDER_CLOSE(blender);
