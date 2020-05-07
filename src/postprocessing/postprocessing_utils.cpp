/* -*- C++ -*-
 * Copyright 2019-2020 LibRaw LLC (info@libraw.org)
 *
 LibRaw is free software; you can redistribute it and/or modify
 it under the terms of the one of two licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
   (See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
   (See file LICENSE.CDDL provided in LibRaw distribution archive for details).

 */

#include "../../internal/libraw_cxx_defs.h"

#define TBLN 65535

void LibRaw::exp_bef(float shift, float smooth)
{
  // params limits
  if (shift > 8)
    shift = 8;
  if (shift < 0.25)
    shift = 0.25;
  if (smooth < 0.0)
    smooth = 0.0;
  if (smooth > 1.0)
    smooth = 1.0;

  unsigned short *lut = (ushort *)malloc((TBLN + 1) * sizeof(unsigned short));

  if (shift <= 1.0)
  {
    for (int i = 0; i <= TBLN; i++)
      lut[i] = (unsigned short)((float)i * shift);
  }
  else
  {
    float x1, x2, y1, y2;

    float cstops = log(shift) / log(2.0f);
    float room = cstops * 2;
    float roomlin = powf(2.0f, room);
    x2 = (float)TBLN;
    x1 = (x2 + 1) / roomlin - 1;
    y1 = x1 * shift;
    y2 = x2 * (1 + (1 - smooth) * (shift - 1));
    float sq3x = powf(x1 * x1 * x2, 1.0f / 3.0f);
    float B = (y2 - y1 + shift * (3 * x1 - 3.0f * sq3x)) /
              (x2 + 2.0f * x1 - 3.0f * sq3x);
    float A = (shift - B) * 3.0f * powf(x1 * x1, 1.0f / 3.0f);
    float CC = y2 - A * powf(x2, 1.0f / 3.0f) - B * x2;
    for (int i = 0; i <= TBLN; i++)
    {
      float X = (float)i;
      float Y = A * powf(X, 1.0f / 3.0f) + B * X + CC;
      if (i < x1)
        lut[i] = (unsigned short)((float)i * shift);
      else
        lut[i] = Y < 0 ? 0 : (Y > TBLN ? TBLN : (unsigned short)(Y));
    }
  }
  for (int i = 0; i < S.height * S.width; i++)
  {
    imgdata.image[i][0] = lut[imgdata.image[i][0]];
    imgdata.image[i][1] = lut[imgdata.image[i][1]];
    imgdata.image[i][2] = lut[imgdata.image[i][2]];
    imgdata.image[i][3] = lut[imgdata.image[i][3]];
  }

  if (C.data_maximum <= TBLN)
    C.data_maximum = lut[C.data_maximum];
  if (C.maximum <= TBLN)
    C.maximum = lut[C.maximum];
  free(lut);
}

void LibRaw::convert_to_rgb_loop(float out_cam[3][4])
{
  int row, col, c;
  float out[3];
  ushort *img;
  memset(libraw_internal_data.output_data.histogram, 0,
         sizeof(int) * LIBRAW_HISTOGRAM_SIZE * 4);
  for (img = imgdata.image[0], row = 0; row < S.height; row++)
    for (col = 0; col < S.width; col++, img += 4)
    {
      if (!libraw_internal_data.internal_output_params.raw_color)
      {
        out[0] = out[1] = out[2] = 0;
        for (c = 0; c < imgdata.idata.colors; c++)
        {
          out[0] += out_cam[0][c] * img[c];
          out[1] += out_cam[1][c] * img[c];
          out[2] += out_cam[2][c] * img[c];
        }
        for (c = 0; c < 3; c++)
          img[c] = CLIP((int)out[c]);
      }
      for (c = 0; c < imgdata.idata.colors; c++)
        libraw_internal_data.output_data.histogram[c][img[c] >> 3]++;
    }
}

void LibRaw::scale_colors_loop(float scale_mul[4])
{
  unsigned size = S.iheight * S.iwidth;

  if (C.cblack[4] && C.cblack[5])
  {
    int val;
    for (unsigned i = 0; i < size * 4; i++)
    {
      if (!(val = imgdata.image[0][i]))
        continue;
      val -= C.cblack[6 + i / 4 / S.iwidth % C.cblack[4] * C.cblack[5] +
                      i / 4 % S.iwidth % C.cblack[5]];
      val -= C.cblack[i & 3];
      val *= scale_mul[i & 3];
      imgdata.image[0][i] = CLIP(val);
    }
  }
  else if (C.cblack[0] || C.cblack[1] || C.cblack[2] || C.cblack[3])
  {
    for (unsigned i = 0; i < size * 4; i++)
    {
      int val = imgdata.image[0][i];
      if (!val)
        continue;
      val -= C.cblack[i & 3];
      val *= scale_mul[i & 3];
      imgdata.image[0][i] = CLIP(val);
    }
  }
  else // BL is zero
  {
    for (unsigned i = 0; i < size * 4; i++)
    {
      int val = imgdata.image[0][i];
      val *= scale_mul[i & 3];
      imgdata.image[0][i] = CLIP(val);
    }
  }
}
