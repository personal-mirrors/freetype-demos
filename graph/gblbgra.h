
  int                   h        = blit->height;
  const unsigned char*  src_line = blit->src_line;
  unsigned char*        dst_line = blit->dst_line;

  do
  {
    const unsigned char*  src = src_line + blit->src_x * 4;
    unsigned char*        dst = dst_line + blit->dst_x * GDST_INCR;
    int                   w   = blit->width;

    do
    {
      unsigned int  pix_b = src[0];
      unsigned int  pix_g = src[1];
      unsigned int  pix_r = src[2];
      unsigned int  a = src[3];


      if ( a == 0 )
      {
        /* nothing */
      }
      else if ( a == 255 )
      {
        GDST_STOREC(dst,pix_r,pix_g,pix_b);
      }
      else
      {
        GBlenderPixel  back;

        GDST_READ(dst,back);

        {
          unsigned int  ba = 255 - a;
          unsigned int  back_r = (back >> 16) & 255;
          unsigned int  back_g = (back >> 8) & 255;
          unsigned int  back_b = (back) & 255;


          pix_r = (back_r * ba / 255 + pix_r);
          pix_g = (back_g * ba / 255 + pix_g);
          pix_b = (back_b * ba / 255 + pix_b);
        }

        GDST_STOREC(dst,pix_r,pix_g,pix_b);
      }

      src += 4;
      dst += GDST_INCR;

    } while ( --w > 0 );

    src_line += blit->src_pitch;
    dst_line += blit->dst_pitch;

  } while ( --h > 0 );
