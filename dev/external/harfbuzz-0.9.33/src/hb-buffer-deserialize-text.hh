
#line 1 "hb-buffer-deserialize-text.rl"
/*
 * Copyright © 2013  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_BUFFER_DESERIALIZE_TEXT_HH
#define HB_BUFFER_DESERIALIZE_TEXT_HH

#include "hb-private.hh"


#line 33 "hb-buffer-deserialize-text.c"
static const char _deserialize_text_actions[] = {
	0, 1, 2, 1, 3, 1, 4, 1, 
	5, 1, 6, 1, 7, 2, 0, 2, 
	2, 3, 1, 2, 4, 1, 2, 6, 
	1, 2, 7, 1, 2, 8, 1
};

static const unsigned char _deserialize_text_key_offsets[] = {
	0, 0, 10, 14, 17, 21, 24, 27, 
	31, 34, 35, 39, 42, 45, 53, 58, 
	58, 68, 78, 84, 89, 96, 104, 111, 
	117, 125, 134
};

static const unsigned char _deserialize_text_trans_keys[] = {
	32u, 48u, 9u, 13u, 49u, 57u, 65u, 90u, 
	97u, 122u, 45u, 48u, 49u, 57u, 48u, 49u, 
	57u, 45u, 48u, 49u, 57u, 48u, 49u, 57u, 
	48u, 49u, 57u, 45u, 48u, 49u, 57u, 48u, 
	49u, 57u, 44u, 45u, 48u, 49u, 57u, 48u, 
	49u, 57u, 44u, 48u, 57u, 32u, 43u, 61u, 
	64u, 93u, 124u, 9u, 13u, 32u, 93u, 124u, 
	9u, 13u, 32u, 48u, 9u, 13u, 49u, 57u, 
	65u, 90u, 97u, 122u, 32u, 43u, 61u, 64u, 
	93u, 124u, 9u, 13u, 48u, 57u, 32u, 44u, 
	93u, 124u, 9u, 13u, 32u, 93u, 124u, 9u, 
	13u, 32u, 93u, 124u, 9u, 13u, 48u, 57u, 
	32u, 44u, 93u, 124u, 9u, 13u, 48u, 57u, 
	32u, 43u, 64u, 93u, 124u, 9u, 13u, 32u, 
	43u, 93u, 124u, 9u, 13u, 32u, 43u, 93u, 
	124u, 9u, 13u, 48u, 57u, 32u, 43u, 64u, 
	93u, 124u, 9u, 13u, 48u, 57u, 32u, 43u, 
	61u, 64u, 93u, 95u, 124u, 9u, 13u, 45u, 
	46u, 48u, 57u, 65u, 90u, 97u, 122u, 0
};

static const char _deserialize_text_single_lengths[] = {
	0, 2, 2, 1, 2, 1, 1, 2, 
	1, 1, 2, 1, 1, 6, 3, 0, 
	2, 6, 4, 3, 3, 4, 5, 4, 
	4, 5, 7
};

static const char _deserialize_text_range_lengths[] = {
	0, 4, 1, 1, 1, 1, 1, 1, 
	1, 0, 1, 1, 1, 1, 1, 0, 
	4, 2, 1, 1, 2, 2, 1, 1, 
	2, 2, 5
};

static const unsigned char _deserialize_text_index_offsets[] = {
	0, 0, 7, 11, 14, 18, 21, 24, 
	28, 31, 33, 37, 40, 43, 51, 56, 
	57, 64, 73, 79, 84, 90, 97, 104, 
	110, 117, 125
};

static const char _deserialize_text_indicies[] = {
	0, 2, 0, 3, 4, 4, 1, 5, 
	6, 7, 1, 8, 9, 1, 10, 11, 
	12, 1, 13, 14, 1, 15, 16, 1, 
	17, 18, 19, 1, 20, 21, 1, 22, 
	1, 23, 24, 25, 1, 26, 27, 1, 
	22, 21, 1, 28, 29, 30, 31, 32, 
	33, 28, 1, 34, 35, 36, 34, 1, 
	1, 0, 2, 0, 3, 4, 4, 1, 
	28, 29, 30, 31, 32, 33, 28, 37, 
	1, 38, 39, 40, 41, 38, 1, 42, 
	43, 44, 42, 1, 42, 43, 44, 42, 
	14, 1, 38, 39, 40, 41, 38, 9, 
	1, 45, 46, 47, 48, 49, 45, 1, 
	50, 51, 52, 53, 50, 1, 50, 51, 
	52, 53, 50, 27, 1, 45, 46, 47, 
	48, 49, 45, 54, 1, 28, 29, 30, 
	31, 32, 55, 33, 28, 55, 55, 55, 
	55, 1, 0
};

static const char _deserialize_text_trans_targs[] = {
	1, 0, 13, 17, 26, 3, 18, 21, 
	18, 21, 5, 19, 20, 19, 20, 22, 
	25, 8, 9, 12, 9, 12, 10, 11, 
	23, 24, 23, 24, 14, 2, 6, 7, 
	15, 16, 14, 15, 16, 17, 14, 4, 
	15, 16, 14, 15, 16, 14, 2, 7, 
	15, 16, 14, 2, 15, 16, 25, 26
};

static const char _deserialize_text_trans_actions[] = {
	0, 0, 13, 13, 13, 1, 1, 1, 
	0, 0, 1, 1, 1, 0, 0, 1, 
	1, 1, 1, 1, 0, 0, 7, 1, 
	1, 1, 0, 0, 16, 3, 3, 3, 
	16, 16, 0, 0, 0, 0, 25, 11, 
	25, 25, 28, 28, 28, 19, 5, 5, 
	19, 19, 22, 9, 22, 22, 0, 0
};

static const char _deserialize_text_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 16, 0, 0, 
	0, 16, 25, 28, 28, 25, 19, 22, 
	22, 19, 16
};

static const int deserialize_text_start = 1;
static const int deserialize_text_first_final = 13;
static const int deserialize_text_error = 0;

static const int deserialize_text_en_main = 1;


#line 91 "hb-buffer-deserialize-text.rl"


static hb_bool_t
_hb_buffer_deserialize_glyphs_text (hb_buffer_t *buffer,
				    const char *buf,
				    unsigned int buf_len,
				    const char **end_ptr,
				    hb_font_t *font)
{
  const char *p = buf, *pe = buf + buf_len;

  /* Ensure we have positions. */
  (void) hb_buffer_get_glyph_positions (buffer, NULL);

  while (p < pe && ISSPACE (*p))
    p++;
  if (p < pe && *p == (buffer->len ? '|' : '['))
  {
    *end_ptr = ++p;
  }

  const char *eof = pe, *tok = NULL;
  int cs;
  hb_glyph_info_t info;
  hb_glyph_position_t pos;
  
#line 169 "hb-buffer-deserialize-text.c"
	{
	cs = deserialize_text_start;
	}

#line 172 "hb-buffer-deserialize-text.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _deserialize_text_trans_keys + _deserialize_text_key_offsets[cs];
	_trans = _deserialize_text_index_offsets[cs];

	_klen = _deserialize_text_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _deserialize_text_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _deserialize_text_indicies[_trans];
	cs = _deserialize_text_trans_targs[_trans];

	if ( _deserialize_text_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _deserialize_text_actions + _deserialize_text_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	memset (&info, 0, sizeof (info));
	memset (&pos , 0, sizeof (pos ));
}
	break;
	case 1:
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (buffer->in_error)
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 2:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
	break;
	case 3:
#line 55 "hb-buffer-deserialize-text.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 4:
#line 62 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 5:
#line 63 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 6:
#line 64 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 7:
#line 65 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 8:
#line 66 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
#line 288 "hb-buffer-deserialize-text.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _deserialize_text_actions + _deserialize_text_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (buffer->in_error)
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 3:
#line 55 "hb-buffer-deserialize-text.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 4:
#line 62 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 6:
#line 64 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 7:
#line 65 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 8:
#line 66 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
#line 332 "hb-buffer-deserialize-text.c"
		}
	}
	}

	_out: {}
	}

#line 119 "hb-buffer-deserialize-text.rl"


  *end_ptr = p;

  return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_HH */
