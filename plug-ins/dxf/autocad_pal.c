/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * autocad_pal.h: AutoCAD definitions for DXF Import
 * Copyright (C) 2002 Angus Ainslie 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"

#include <stdlib.h> /* abs() from stdlib.h */

#include "autocad_pal.h"

static const 
RGB_t acad_pal[256] =
{
     { 0x00, 0x00, 0x00 },      /* 0 */
     { 0xFF, 0x00, 0x00 },
     { 0xFF, 0xFF, 0x00 },
     { 0x00, 0xFF, 0x00 },
     { 0x00, 0xFF, 0xFF },
     { 0x00, 0x00, 0xFF },      /* 5 */
     { 0xFF, 0x00, 0xFF },
     { 0xFF, 0xFF, 0xFF },
     { 0x41, 0x41, 0x41 },
     { 0x80, 0x80, 0x80 },
     { 0xFF, 0x00, 0x00 },      /* 10 */
     { 0xFF, 0xAA, 0xAA },
     { 0xBD, 0x00, 0x00 },
     { 0xBD, 0x7E, 0x7E },
     { 0x81, 0x00, 0x00 },
     { 0x81, 0x56, 0x56 },      /* 15 */
     { 0x68, 0x00, 0x00 },
     { 0x68, 0x45, 0x45 },
     { 0x4F, 0x00, 0x00 },
     { 0x4F, 0x35, 0x35 },
     { 0xFF, 0x3F, 0x00 },      /* 20 */
     { 0xFF, 0xBF, 0xAA },
     { 0xBD, 0x2E, 0x00 },
     { 0xBD, 0x8D, 0x7E },
     { 0x81, 0x1F, 0x00 },
     { 0x81, 0x60, 0x56 },      /* 25 */
     { 0x68, 0x19, 0x00 },
     { 0x68, 0x4E, 0x45 },
     { 0x4F, 0x13, 0x00 },
     { 0x4F, 0x3B, 0x35 },
     { 0xFF, 0x7F, 0x00 },      /* 30 */
     { 0xFF, 0xD4, 0xAA },
     { 0xBD, 0x5E, 0x00 },
     { 0xBD, 0x9D, 0x7E },
     { 0x81, 0x40, 0x00 },
     { 0x81, 0x6B, 0x56 },      /* 35 */
     { 0x68, 0x34, 0x00 },
     { 0x68, 0x56, 0x45 },
     { 0x4F, 0x27, 0x00 },
     { 0x4F, 0x42, 0x35 },
     { 0xFF, 0xBF, 0x00 },      /* 40 */
     { 0xFF, 0xEA, 0xAA },
     { 0xBD, 0x8D, 0x00 },
     { 0xBD, 0xAD, 0x7E },
     { 0x81, 0x60, 0x00 },
     { 0x81, 0x76, 0x56 },      /* 45 */
     { 0x68, 0x4E, 0x00 },
     { 0x68, 0x5F, 0x45 },
     { 0x4F, 0x3B, 0x00 },
     { 0x4F, 0x49, 0x35 },
     { 0xFF, 0xFF, 0x00 },      /* 50 */
     { 0xFF, 0xFF, 0xAA },
     { 0xBD, 0xBD, 0x00 },
     { 0xBD, 0xBD, 0x7E },
     { 0x81, 0x81, 0x00 },
     { 0x81, 0x81, 0x56 },      /* 55 */
     { 0x68, 0x68, 0x00 },
     { 0x68, 0x68, 0x45 },
     { 0x4F, 0x4F, 0x00 },
     { 0x4F, 0x4F, 0x35 },
     { 0xBF, 0xFF, 0x00 },      /* 60 */
     { 0xEA, 0xFF, 0xAA },
     { 0x8D, 0xBD, 0x00 },
     { 0xAD, 0xBD, 0x7E },
     { 0x60, 0x81, 0x00 },
     { 0x76, 0x81, 0x56 },      /* 65 */
     { 0x4E, 0x68, 0x00 },
     { 0x5F, 0x68, 0x45 },
     { 0x3B, 0x4F, 0x00 },
     { 0x49, 0x4F, 0x35 },
     { 0x7F, 0xFF, 0x00 },      /* 70 */
     { 0xD4, 0xFF, 0xAA },
     { 0x5E, 0xBD, 0x00 },
     { 0x9D, 0xBD, 0x7E },
     { 0x40, 0x81, 0x00 },
     { 0x6B, 0x81, 0x56 },      /* 75 */
     { 0x34, 0x68, 0x00 },
     { 0x56, 0x68, 0x45 },
     { 0x27, 0x4F, 0x00 },
     { 0x42, 0x4F, 0x35 },
     { 0x3F, 0xFF, 0x00 },      /*80 */
     { 0xBF, 0xFF, 0xAA },
     { 0x2E, 0xBD, 0x00 },
     { 0x8D, 0xBD, 0x7E },
     { 0x1F, 0x81, 0x00 },
     { 0x60, 0x81, 0x56 },      /*85 */
     { 0x19, 0x68, 0x00 },
     { 0x4E, 0x68, 0x45 },
     { 0x13, 0x4F, 0x00 },
     { 0x3B, 0x4F, 0x35 },
     { 0x00, 0xFF, 0x00 },      /*90 */
     { 0xAA, 0xFF, 0xAA },
     { 0x00, 0xBD, 0x00 },
     { 0x7E, 0xBD, 0x7E },
     { 0x00, 0x81, 0x00 },
     { 0x56, 0x81, 0x56 },      /*95 */
     { 0x00, 0x68, 0x00 },
     { 0x45, 0x68, 0x45 },
     { 0x00, 0x4F, 0x00 },
     { 0x35, 0x4F, 0x35 },
     { 0x00, 0xFF, 0x3F },      /*100 */
     { 0xAA, 0xFF, 0xBF },
     { 0x00, 0xBD, 0x2E },
     { 0x7E, 0xBD, 0x8D },
     { 0x00, 0x81, 0x1F },
     { 0x56, 0x81, 0x60 },      /*105 */
     { 0x00, 0x68, 0x19 },
     { 0x45, 0x68, 0x4E },
     { 0x00, 0x4F, 0x13 },
     { 0x35, 0x4F, 0x3B },
     { 0x00, 0xFF, 0x7F },      /*110 */
     { 0xAA, 0xFF, 0xD4 },
     { 0x00, 0xBD, 0x5E },
     { 0x7E, 0xBD, 0x9D },
     { 0x00, 0x81, 0x40 },
     { 0x56, 0x81, 0x6B },      /*115 */
     { 0x00, 0x68, 0x34 },
     { 0x45, 0x68, 0x56 },
     { 0x00, 0x4F, 0x27 },
     { 0x35, 0x4F, 0x42 },
     { 0x00, 0xFF, 0xBF },      /*120 */
     { 0xAA, 0xFF, 0xEA },
     { 0x00, 0xBD, 0x8D },
     { 0x7E, 0xBD, 0xAD },
     { 0x00, 0x81, 0x60 },
     { 0x56, 0x81, 0x76 },      /*125 */
     { 0x00, 0x68, 0x4E },
     { 0x45, 0x68, 0x5F },
     { 0x00, 0x4F, 0x3B },
     { 0x35, 0x4F, 0x49 },
     { 0x00, 0xFF, 0xFF },      /*130 */
     { 0xAA, 0xFF, 0xFF },
     { 0x00, 0xBD, 0xBD },
     { 0x7E, 0xBD, 0xBD },
     { 0x00, 0x81, 0x81 },
     { 0x56, 0x81, 0x81 },      /*135 */
     { 0x00, 0x68, 0x68 },
     { 0x45, 0x68, 0x68 },
     { 0x00, 0x4F, 0x4F },
     { 0x35, 0x4F, 0x4F },
     { 0x00, 0xBF, 0xFF },      /*140 */
     { 0xAA, 0xEA, 0xFF },
     { 0x00, 0x8D, 0xBD },
     { 0x7E, 0xAD, 0xBD },
     { 0x00, 0x60, 0x81 },
     { 0x56, 0x76, 0x81 },      /*145 */
     { 0x00, 0x4E, 0x68 },
     { 0x45, 0x5F, 0x68 },
     { 0x00, 0x3B, 0x4F },
     { 0x35, 0x49, 0x4F },
     { 0x00, 0x7F, 0xFF },      /*150 */
     { 0xAA, 0xD4, 0xFF },
     { 0x00, 0x5E, 0xBD },
     { 0x7E, 0x9D, 0xBD },
     { 0x00, 0x40, 0x81 },
     { 0x56, 0x6B, 0x81 },      /*155 */
     { 0x00, 0x34, 0x68 },
     { 0x45, 0x56, 0x68 },
     { 0x00, 0x27, 0x4F },
     { 0x35, 0x42, 0x4F },
     { 0x00, 0x3F, 0xFF },      /*160 */
     { 0xAA, 0xBF, 0xFF },
     { 0x00, 0x2E, 0xBD },
     { 0x7E, 0x8D, 0xBD },
     { 0x00, 0x1F, 0x81 },
     { 0x56, 0x60, 0x81 },      /*165 */
     { 0x00, 0x19, 0x68 },
     { 0x45, 0x4E, 0x68 },
     { 0x00, 0x13, 0x4F },
     { 0x35, 0x3B, 0x4F },
     { 0x00, 0x00, 0xFF },      /*170 */
     { 0xAA, 0xAA, 0xFF },
     { 0x00, 0x00, 0xBD },
     { 0x7E, 0x7E, 0xBD },
     { 0x00, 0x00, 0x81 },
     { 0x56, 0x56, 0x81 },      /*175 */
     { 0x00, 0x00, 0x68 },
     { 0x45, 0x45, 0x68 },
     { 0x00, 0x00, 0x4F },
     { 0x35, 0x35, 0x4F },
     { 0x3F, 0x00, 0xFF },      /*180 */
     { 0xBF, 0xAA, 0xFF },
     { 0x2E, 0x00, 0xBD },
     { 0x8D, 0x7E, 0xBD },
     { 0x1F, 0x00, 0x81 },
     { 0x60, 0x56, 0x81 },      /*185 */
     { 0x19, 0x00, 0x68 },
     { 0x4E, 0x45, 0x68 },
     { 0x13, 0x00, 0x4F },
     { 0x3B, 0x35, 0x4F },
     { 0x7F, 0x00, 0xFF },      /*190 */
     { 0xD4, 0xAA, 0xFF },
     { 0x5E, 0x00, 0xBD },
     { 0x9D, 0x7E, 0xBD },
     { 0x40, 0x00, 0x81 },
     { 0x6B, 0x56, 0x81 },      /*195 */
     { 0x34, 0x00, 0x68 },
     { 0x56, 0x45, 0x68 },
     { 0x27, 0x00, 0x4F },
     { 0x42, 0x35, 0x4F },
     { 0xBF, 0x00, 0xFF },      /*200 */
     { 0xEA, 0xAA, 0xFF },
     { 0x8D, 0x00, 0xBD },
     { 0xAD, 0x7E, 0xBD },
     { 0x60, 0x00, 0x81 },
     { 0x76, 0x56, 0x81 },      /*205 */
     { 0x4E, 0x00, 0x68 },
     { 0x5F, 0x45, 0x68 },
     { 0x3B, 0x00, 0x4F },
     { 0x49, 0x35, 0x4F },
     { 0xFF, 0x00, 0xFF },      /*210 */
     { 0xFF, 0xAA, 0xFF },
     { 0xBD, 0x00, 0xBD },
     { 0xBD, 0x7E, 0xBD },
     { 0x81, 0x00, 0x81 },
     { 0x81, 0x56, 0x81 },      /*215 */
     { 0x68, 0x00, 0x68 },
     { 0x68, 0x45, 0x68 },
     { 0x4F, 0x00, 0x4F },
     { 0x4F, 0x35, 0x4F },
     { 0xFF, 0x00, 0xBF },      /*220 */
     { 0xFF, 0xAA, 0xEA },
     { 0xBD, 0x00, 0x8D },
     { 0xBD, 0x7E, 0xAD },
     { 0x81, 0x00, 0x60 },
     { 0x81, 0x56, 0x76 },      /*225 */
     { 0x68, 0x00, 0x4E },
     { 0x68, 0x45, 0x5F },
     { 0x4F, 0x00, 0x3B },
     { 0x4F, 0x35, 0x49 },
     { 0xFF, 0x00, 0x7F },      /*230 */
     { 0xFF, 0xAA, 0xD4 },
     { 0xBD, 0x00, 0x5E },
     { 0xBD, 0x7E, 0x9D },
     { 0x81, 0x00, 0x40 },
     { 0x81, 0x56, 0x6B },      /*235 */
     { 0x68, 0x00, 0x34 },
     { 0x68, 0x45, 0x56 },
     { 0x4F, 0x00, 0x27 },
     { 0x4F, 0x35, 0x42 },
     { 0xFF, 0x00, 0x3F },      /*240 */
     { 0xFF, 0xAA, 0xBF },
     { 0xBD, 0x00, 0x2E },
     { 0xBD, 0x7E, 0x8D },
     { 0x81, 0x00, 0x1F },
     { 0x81, 0x56, 0x60 },      /*245 */
     { 0x68, 0x00, 0x19 },
     { 0x68, 0x45, 0x4E },
     { 0x4F, 0x00, 0x13 },
     { 0x4F, 0x35, 0x3B },
     { 0x33, 0x33, 0x33 },      /*250 */
     { 0x50, 0x50, 0x50 },
     { 0x69, 0x69, 0x69 },
     { 0x82, 0x82, 0x82 },
     { 0xBE, 0xBE, 0xBE },
     { 0xFF, 0xFF, 0xFF }      /*255 */
};

static const int num_colors = sizeof(acad_pal)/sizeof(acad_pal[0]);

RGB_t 
pal_get_rgb (int index)
{
  if (index >= 0 && index < num_colors)
    return acad_pal[index];
    
  return acad_pal[0];
}

int
pal_get_index (const RGB_t rgb)
{
  int i, n = 0;
  int dist, last = 256*3;
  
  for (i = 0; i < num_colors; ++i) {
    if (acad_pal[i].r == rgb.r && acad_pal[i].g == rgb.g && acad_pal[i].b == rgb.b)
      return i;
    dist = abs((int)rgb.r - acad_pal[i].r)
         + abs((int)rgb.g - acad_pal[i].g) 
	 + abs((int)rgb.b - acad_pal[i].b);
    if (dist < last) {
      n = i;
      last = dist;
    }
  }
  return n;
}

