/*

  U8g2_for_Adafruit_GFX_extmem.cpp

  Add unicode support and U8g2 fonts to Adafruit GFX libraries.

  U8g2 for Adafruit GFX Lib (https://github.com/olikraus/U8g2_for_Adafruit_GFX)

  Library modified by RobotGB (https://github.com/RobotGB/U8G2_for_Adafruit_GFX_extmem)

  Copyright (c) 2018, olikraus@gmail.com
  All rights reserved.
  Copyright (c) 2019, RobotGB (robot_m0@outlook.com)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include "U8g2_for_Adafruit_GFX_extmem.h"

static uint8_t u8g2_font_get_byte(MEM::File font, uint32_t offset)
{
  #ifdef USE_SPIFFSMEM
    font.seek(offset, MEM::SeekSet);
  #else
    font.seek(offset);
  #endif

  return font.read();
}

static uint16_t u8g2_font_get_word(MEM::File font, uint32_t offset)
{
    uint16_t pos;
    pos += u8g2_font_get_byte(font, offset);
    pos <<= 8;
    pos += u8g2_font_get_byte(font, offset + 1);
    return pos;
}

/*========================================================================*/
/* new font format */
void u8g2_read_font_info(u8g2_font_info_t *font_info, MEM::File font)
{
  /* offset 0 */
  font_info->glyph_cnt = u8g2_font_get_byte(font, 0);
  font_info->bbx_mode = u8g2_font_get_byte(font, 1);
  font_info->bits_per_0 = u8g2_font_get_byte(font, 2);
  font_info->bits_per_1 = u8g2_font_get_byte(font, 3);

  /* offset 4 */
  font_info->bits_per_char_width = u8g2_font_get_byte(font, 4);
  font_info->bits_per_char_height = u8g2_font_get_byte(font, 5);
  font_info->bits_per_char_x = u8g2_font_get_byte(font, 6);
  font_info->bits_per_char_y = u8g2_font_get_byte(font, 7);
  font_info->bits_per_delta_x = u8g2_font_get_byte(font, 8);

  /* offset 9 */
  font_info->max_char_width = u8g2_font_get_byte(font, 9);
  font_info->max_char_height = u8g2_font_get_byte(font, 10);
  font_info->x_offset = u8g2_font_get_byte(font, 11);
  font_info->y_offset = u8g2_font_get_byte(font, 12);

  /* offset 13 */
  font_info->ascent_A = u8g2_font_get_byte(font, 13);
  font_info->descent_g = u8g2_font_get_byte(font, 14);
  font_info->ascent_para = u8g2_font_get_byte(font, 15);
  font_info->descent_para = u8g2_font_get_byte(font, 16);

  /* offset 17 */
  font_info->start_pos_upper_A = u8g2_font_get_word(font, 17);
  font_info->start_pos_lower_a = u8g2_font_get_word(font, 19);

  /* offset 21 */
  font_info->start_pos_unicode = u8g2_font_get_word(font, 21);
}

uint8_t u8g2_GetFontBBXWidth(u8g2_font_t *u8g2)
{
  return u8g2->font_info.max_char_width;    /* new font info structure */
}

uint8_t u8g2_GetFontBBXHeight(u8g2_font_t *u8g2)
{
  return u8g2->font_info.max_char_height;   /* new font info structure */
}

int8_t u8g2_GetFontBBXOffX(u8g2_font_t *u8g2)
{
  return u8g2->font_info.x_offset;    /* new font info structure */
}

int8_t u8g2_GetFontBBXOffY(u8g2_font_t *u8g2)
{
  return u8g2->font_info.y_offset;    /* new font info structure */
}

uint8_t u8g2_GetFontCapitalAHeight(u8g2_font_t *u8g2)
{
  return u8g2->font_info.ascent_A;    /* new font info structure */
}

static uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t *f, uint8_t cnt)
{
  uint8_t val;
  uint8_t bit_pos = f->decode_bit_pos;
  uint8_t bit_pos_plus_cnt;

  val = u8g2_font_get_byte(f->font, f->decode_ptr);

  val >>= bit_pos;
  bit_pos_plus_cnt = bit_pos;
  bit_pos_plus_cnt += cnt;
  if ( bit_pos_plus_cnt >= 8 )
  {
    uint8_t s = 8;
    s -= bit_pos;
    f->decode_ptr++;
    val |= (u8g2_font_get_byte(f->font, f->decode_ptr) << (s));
    bit_pos_plus_cnt -= 8;
  }
  val &= (1U<<cnt)-1;

  f->decode_bit_pos = bit_pos_plus_cnt;
  return val;
}


/*
    2 bit --> cnt = 2
      -2,-1,0. 1

    3 bit --> cnt = 3
      -2,-1,0. 1
      -4,-3,-2,-1,0,1,2,3

      if ( x < 0 )
  r = bits(x-1)+1;
    else
  r = bits(x)+1;

*/
/* optimized */
static int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t *f, uint8_t cnt)
{
  int8_t v, d;
  v = (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt);
  d = 1;
  cnt--;
  d <<= cnt;
  v -= d;
  return v;
}


static int16_t u8g2_add_vector_y(int16_t dy, int8_t x, int8_t y, uint8_t dir)
{
  switch(dir)
  {
    case 0:
      dy += y;
      break;
    case 1:
      dy += x;
      break;
    case 2:
      dy -= y;
      break;
    default:
      dy -= x;
      break;
  }
  return dy;
}

static int16_t u8g2_add_vector_x(int16_t dx, int8_t x, int8_t y, uint8_t dir)
{
  switch(dir)
  {
    case 0:
      dx += x;
      break;
    case 1:
      dx -= y;
      break;
    case 2:
      dx -= x;
      break;
    default:
      dx += y;
      break;
  }
  return dx;
}

void u8g2_draw_hv_line(u8g2_font_t *u8g2, int16_t x, int16_t y, int16_t len, uint8_t dir, uint16_t color)
{
  switch(dir)
  {
    case 0:
      u8g2->gfx->drawFastHLine(x,y,len,color);
      break;
    case 1:
      u8g2->gfx->drawFastVLine(x,y,len,color);
      break;
    case 2:
      u8g2->gfx->drawFastHLine(x-len+1,y,len,color);
      break;
    case 3:
      u8g2->gfx->drawFastVLine(x,y-len+1,len,color);
      break;
  }

}



/*
  Description:
    Draw a run-length area of the glyph. "len" can have any size and the line
    length has to be wrapped at the glyph border.
  Args:
    len:          Length of the line
    is_foreground     foreground/background?
    u8g2->font_decode.target_x    X position
    u8g2->font_decode.target_y    Y position
    u8g2->font_decode.is_transparent  Transparent mode
  Return:
    -
  Calls:
    u8g2_Draw90Line()
  Called by:
    u8g2_font_decode_glyph()
*/
/* optimized */
static void u8g2_font_decode_len(u8g2_font_t *u8g2, uint8_t len, uint8_t is_foreground)
{
  u8g2->font = u8g2->font_decode.font;
  uint8_t cnt;  /* total number of remaining pixels, which have to be drawn */
  uint8_t rem;  /* remaining pixel to the right edge of the glyph */
  uint8_t current;  /* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */

  /* local coordinates of the glyph */
  uint8_t lx,ly;

  /* target position on the screen */
  int16_t x, y;

  u8g2_font_decode_t *decode = &(u8g2->font_decode);

  cnt = len;

  /* get the local position */
  lx = decode->x;
  ly = decode->y;

  for(;;)
  {

    /* calculate the number of pixel to the right edge of the glyph */
    rem = decode->glyph_width;
    rem -= lx;

    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if ( cnt < rem )
      current = cnt;


    /* now draw the line, but apply the rotation around the glyph target position */

    /* get target position */
    x = decode->target_x;
    y = decode->target_y;

    /* apply rotation */
    x = u8g2_add_vector_x(x, lx, ly, decode->dir);
    y = u8g2_add_vector_y(y, lx, ly, decode->dir);

    /* draw foreground and background (if required) */
    if ( current > 0 )		/* avoid drawing zero length lines, issue #4 */
    {
      if ( is_foreground )
      {
	u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, decode->fg_color);
      }
      else if ( decode->is_transparent == 0 )
      {
	u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, decode->bg_color);
      }
    }

    /* check, whether the end of the run length code has been reached */
    if ( cnt < rem )
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;

  decode->x = lx;
  decode->y = ly;

}

static void u8g2_font_setup_decode(u8g2_font_t *u8g2, uint32_t glyph_data)
{
  u8g2->font = u8g2->font_decode.font;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  decode->decode_ptr = glyph_data;
  decode->decode_bit_pos = 0;

  /* 8 Nov 2015, this is already done in the glyph data search procedure */
  /*
  decode->decode_ptr += 1;
  decode->decode_ptr += 1;
  */

  decode->glyph_width = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
  decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_height);

}


/*
  Description:
    Decode and draw a glyph.
  Args:
    glyph_data:           Pointer to the compressed glyph data of the font
    u8g2->font_decode.target_x    X position
    u8g2->font_decode.target_y    Y position
    u8g2->font_decode.is_transparent  Transparent mode
  Return:
    Width (delta x advance) of the glyph.
  Calls:
    u8g2_font_decode_len()
*/
/* optimized */
static int8_t u8g2_font_decode_glyph(u8g2_font_t *u8g2, uint32_t glyph_data)
{
  u8g2->font = u8g2->font_decode.font;
  uint8_t a, b;
  int8_t x, y;
  int8_t d;
  int8_t h;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);

  u8g2_font_setup_decode(u8g2, glyph_data);
  h = u8g2->font_decode.glyph_height;

  x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
  y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
  d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);


  if ( decode->glyph_width > 0 )
  {
    decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h+y), decode->dir);
    decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h+y), decode->dir);


    /* reset local x/y position */
    decode->x = 0;
    decode->y = 0;

    /* decode glyph */
    for(;;)
    {

      a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
      b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
      do
      {
        u8g2_font_decode_len(u8g2, a, 0);
        u8g2_font_decode_len(u8g2, b, 1);
      } while( u8g2_font_decode_get_unsigned_bits(decode, 1) != 0 );

      if ( decode->y >= h )
        break;
    }

  }
  return d;
}

/*
  Description:
    Find the starting point of the glyph data.
  Args:
    encoding: Encoding (ASCII or Unicode) of the glyph
  Return:
    Address of the glyph data or NULL, if the encoding is not avialable in the font.
*/
uint32_t u8g2_font_get_glyph_data(u8g2_font_t *u8g2, uint16_t encoding)
{
  u8g2->font = u8g2->font_decode.font;
  uint32_t fontLocation;
  fontLocation = 23;


  if ( encoding <= 255 )
  {
    if ( encoding >= 'a' )
    {
      fontLocation += u8g2->font_info.start_pos_lower_a;
    }
    else if ( encoding >= 'A' )
    {
      fontLocation += u8g2->font_info.start_pos_upper_A;
    }

    for(;;)
    {

      if (u8g2_font_get_byte(u8g2->font, fontLocation + 1) == 0)
        break;
      if (u8g2_font_get_byte(u8g2->font, fontLocation) == encoding)
      {
        return fontLocation + 2;
      }
      fontLocation += u8g2_font_get_byte(u8g2->font, fontLocation + 1);
    }
  }
  else
  {
    uint16_t e;
    uint32_t unicode_lookup_table;
    /* support for the new unicode lookup table */

    fontLocation += u8g2->font_info.start_pos_unicode;
    unicode_lookup_table = fontLocation;

    /* u8g2 issue 596: search for the glyph start in the unicode lookup table */
    do
    {
      fontLocation += u8g2_font_get_word(u8g2->font, unicode_lookup_table);
      e = u8g2_font_get_word(u8g2->font, unicode_lookup_table + 2);
      unicode_lookup_table+=4;
    } while( e < encoding );

    /* variable "font" is now updated according to the lookup table */

    for(;;)
    {

      e = u8g2_font_get_byte(u8g2->font, fontLocation);
      e <<= 8;
      e |= u8g2_font_get_byte(u8g2->font, fontLocation + 1);
      if ( e == 0 )
         break;
      if ( e == encoding )
      {
        return fontLocation+3; //skip encoding and glyph size
      }
      fontLocation += u8g2_font_get_byte(u8g2->font, fontLocation + 2);
    }
  }
  return 0;
}

static int16_t u8g2_font_draw_glyph(u8g2_font_t *u8g2, int16_t x, int16_t y, uint16_t encoding)
{
  u8g2->font = u8g2->font_decode.font;
  int16_t dx = 0;
  u8g2->font_decode.target_x = x;
  u8g2->font_decode.target_y = y;
  uint32_t glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
  if ( glyph_data != 0 )
  {
    dx = u8g2_font_decode_glyph(u8g2, glyph_data);
  }
  return dx;
}


//========================================================

uint8_t u8g2_IsGlyph(u8g2_font_t *u8g2, uint16_t requested_encoding)
{
  u8g2->font = u8g2->font_decode.font;
  /* updated to new code */
  if ( u8g2_font_get_glyph_data(u8g2, requested_encoding) != NULL )
    return 1;
  return 0;
}

/* side effect: updates u8g2->font_decode and u8g2->glyph_x_offset */
/* actually u8g2_GetGlyphWidth returns the glyph delta x and glyph width itself is set as side effect */
int8_t u8g2_GetGlyphWidth(u8g2_font_t *u8g2, uint16_t requested_encoding)
{
  // const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
  uint32_t glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
  if ( glyph_data == NULL )
    return 0;

  u8g2_font_setup_decode(u8g2, glyph_data);
  u8g2->glyph_x_offset = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
  u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);

  /* glyph width is here: u8g2->font_decode.glyph_width */

  return u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}


void u8g2_SetFontMode(u8g2_font_t *u8g2, uint8_t is_transparent)
{
  u8g2->font_decode.is_transparent = is_transparent;    // new font procedures
}

void u8g2_SetFontDirection(u8g2_font_t *u8g2, uint8_t dir)
{
  u8g2->font_decode.dir = dir;
}



int16_t u8g2_DrawGlyph(u8g2_font_t *u8g2, int16_t x, int16_t y, uint16_t encoding)
{
  return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}

int16_t u8g2_DrawStr(u8g2_font_t *u8g2, int16_t x, int16_t y, const char *s)
{
  int16_t sum, delta;

  while( *s != '\0' )
  {
    delta = u8g2_DrawGlyph(u8g2, x, y, *s);
    switch(u8g2->font_decode.dir)
    {
      case 0:
        x += delta;
        break;
      case 1:
        y += delta;
        break;
      case 2:
        x -= delta;
        break;
      case 3:
        y -= delta;
        break;
    }
    sum += delta;
    s++;
  }
  return sum;
}



void u8g2_SetFont(u8g2_font_t *u8g2, MEM::File font)
{
  if ( u8g2->font != font )
  {
    u8g2->font = font;
    u8g2->font_decode.is_transparent = 0;

    u8g2_read_font_info(&(u8g2->font_info), font);
  }
  u8g2->font_decode.font = font;
}

void u8g2_SetForegroundColor(u8g2_font_t *u8g2, uint16_t fg)
{
  u8g2->font_decode.fg_color = fg;
}

void u8g2_SetBackgroundColor(u8g2_font_t *u8g2, uint16_t bg)
{
  u8g2->font_decode.bg_color = bg;
}

//========================================================


/*
  pass a byte from an utf8 encoded string to the utf8 decoder state machine
  returns
    0x0fffe: no glyph, just continue
    0x0ffff: end of string
    anything else: The decoded encoding
*/
uint16_t U8G2_FOR_ADAFRUIT_GFX_EXTMEM::utf8_next(uint8_t b)
{
  if ( b == 0 )  /* '\n' terminates the string to support the string list procedures */
    return 0x0ffff; /* end of string detected, pending UTF8 is discarded */
  if ( utf8_state == 0 )
  {
    if ( b >= 0xfc )  /* 6 byte sequence */
    {
      utf8_state = 5;
      b &= 1;
    }
    else if ( b >= 0xf8 )
    {
      utf8_state = 4;
      b &= 3;
    }
    else if ( b >= 0xf0 )
    {
      utf8_state = 3;
      b &= 7;
    }
    else if ( b >= 0xe0 )
    {
      utf8_state = 2;
      b &= 15;
    }
    else if ( b >= 0xc0 )
    {
      utf8_state = 1;
      b &= 0x01f;
    }
    else
    {
      /* do nothing, just use the value as encoding */
      return b;
    }
    encoding = b;
    return 0x0fffe;
  }
  else
  {
    utf8_state--;
    /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
    encoding<<=6;
    b &= 0x03f;
    encoding |= b;
    if ( utf8_state != 0 )
      return 0x0fffe; /* nothing to do yet */
  }
  return encoding;
}

int16_t U8G2_FOR_ADAFRUIT_GFX_EXTMEM::drawUTF8(int16_t x, int16_t y, const char *str)
{
  uint16_t e;
  int16_t delta, sum;

  utf8_state = 0;
  sum = 0;
  for(;;)
  {

    e = utf8_next((uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      delta = drawGlyph(x, y, e);

      switch(u8g2.font_decode.dir)
      {
        case 0:
          x += delta;
          break;
        case 1:
          y += delta;
          break;
        case 2:
          x -= delta;
          break;
        case 3:
          y -= delta;
          break;
      }

      sum += delta;
    }
  }
  return sum;
}

int16_t U8G2_FOR_ADAFRUIT_GFX_EXTMEM::getUTF8Width(const char *str)
{
  uint16_t e;
  int16_t dx, w;

  u8g2.font_decode.glyph_width = 0;
  utf8_state = 0;
  w = 0;
  dx = 0;
  for(;;)
  {

    e = utf8_next((uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      dx = u8g2_GetGlyphWidth(&u8g2, e);
      w += dx;
    }
  }
  /* adjust the last glyph, check for issue #16: do not adjust if width is 0 */
  if ( u8g2.font_decode.glyph_width != 0 )
  {
    w -= dx;
    w += u8g2.font_decode.glyph_width;  /* the real pixel width of the glyph, sideeffect of GetGlyphWidth */
    /* issue #46: we have to add the x offset also */
    w += u8g2.glyph_x_offset;	/* this value is set as a side effect of u8g2_GetGlyphWidth() */
  }

  return w;
}
