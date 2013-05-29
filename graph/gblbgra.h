
  int                   h        = blit->height;
  const unsigned char*  src_line = blit->src_line;
  unsigned char*        dst_line = blit->dst_line;

  gblender_use_channels( blender, 0 );

  do
  {
    const unsigned char*  src = src_line + blit->src_x*4;
    unsigned char*        dst = dst_line + blit->dst_x*GDST_INCR;
    int                   w   = blit->width;

    do
    {
      int  a = GBLENDER_SHADE_INDEX(src[3]);
      int ra = src[3];
      int b = src[0];
      int g = src[1];
      int r = src[2];

      if ( a == 0 )
      {
        /* nothing */
      }
      else if ( a == GBLENDER_SHADE_COUNT-1 )
      {
        GDST_COPY_VAR
        GDST_COPY(dst);
      }
      else
      {
        b = b * 255 / ra;
        g = g * 255 / ra;
        r = r * 255 / ra;

        {
          GBLENDER_VARS(blender,color);

          GBlenderPixel  back;

          GDST_READ(dst,back);

          GBLENDER_LOOKUP( blender, back );

#ifdef GBLENDER_STORE_BYTES
          GDST_STOREB(dst,_gcells,a);
#else
          GDST_STOREP(dst,_gcells,a);
#endif

          GBLENDER_CLOSE(blender);
        }
     }

      src += 4;
      dst += GDST_INCR;
    }
    while (--w > 0);

    src_line += blit->src_pitch;
    dst_line += blit->dst_pitch;
  }
  while (--h > 0);

