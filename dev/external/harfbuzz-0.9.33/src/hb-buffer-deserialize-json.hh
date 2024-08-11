
#line 1 "hb-buffer-deserialize-json.rl"
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

#ifndef HB_BUFFER_DESERIALIZE_JSON_HH
#define HB_BUFFER_DESERIALIZE_JSON_HH

#include "hb-private.hh"


#line 33 "hb-buffer-deserialize-json.c"
static const char _deserialize_json_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 2, 4, 1, 
	2, 5, 1, 2, 6, 1, 2, 7, 
	1, 2, 8, 1, 2, 9, 1
};

static const unsigned char _deserialize_json_key_offsets[] = {
	0, 0, 4, 8, 12, 14, 15, 19, 
	26, 29, 34, 39, 46, 47, 51, 58, 
	61, 66, 73, 74, 75, 79, 85, 90, 
	97, 99, 100, 104, 111, 114, 119, 126, 
	127, 131, 138, 141, 146, 153, 154, 158, 
	165, 169, 179, 184, 191, 196, 200
};

static const unsigned char _deserialize_json_trans_keys[] = {
	32u, 123u, 9u, 13u, 32u, 34u, 9u, 13u, 
	97u, 99u, 100u, 103u, 120u, 121u, 34u, 32u, 
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u, 
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u, 
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 32u, 
	44u, 125u, 9u, 13u, 48u, 57u, 34u, 32u, 
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u, 
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u, 
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u, 
	57u, 108u, 34u, 32u, 58u, 9u, 13u, 32u, 
	48u, 9u, 13u, 49u, 57u, 32u, 44u, 125u, 
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u, 
	57u, 120u, 121u, 34u, 32u, 58u, 9u, 13u, 
	32u, 45u, 48u, 9u, 13u, 49u, 57u, 48u, 
	49u, 57u, 32u, 44u, 125u, 9u, 13u, 32u, 
	44u, 125u, 9u, 13u, 48u, 57u, 34u, 32u, 
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u, 
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u, 
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u, 
	57u, 34u, 32u, 58u, 9u, 13u, 32u, 34u, 
	48u, 9u, 13u, 49u, 57u, 65u, 90u, 97u, 
	122u, 34u, 95u, 45u, 46u, 48u, 57u, 65u, 
	90u, 97u, 122u, 32u, 44u, 125u, 9u, 13u, 
	32u, 44u, 125u, 9u, 13u, 48u, 57u, 32u, 
	44u, 93u, 9u, 13u, 32u, 123u, 9u, 13u, 
	0
};

static const char _deserialize_json_single_lengths[] = {
	0, 2, 2, 4, 2, 1, 2, 3, 
	1, 3, 3, 3, 1, 2, 3, 1, 
	3, 3, 1, 1, 2, 2, 3, 3, 
	2, 1, 2, 3, 1, 3, 3, 1, 
	2, 3, 1, 3, 3, 1, 2, 3, 
	0, 2, 3, 3, 3, 2, 0
};

static const char _deserialize_json_range_lengths[] = {
	0, 1, 1, 0, 0, 0, 1, 2, 
	1, 1, 1, 2, 0, 1, 2, 1, 
	1, 2, 0, 0, 1, 2, 1, 2, 
	0, 0, 1, 2, 1, 1, 2, 0, 
	1, 2, 1, 1, 2, 0, 1, 2, 
	2, 4, 1, 2, 1, 1, 0
};

static const unsigned char _deserialize_json_index_offsets[] = {
	0, 0, 4, 8, 13, 16, 18, 22, 
	28, 31, 36, 41, 47, 49, 53, 59, 
	62, 67, 73, 75, 77, 81, 86, 91, 
	97, 100, 102, 106, 112, 115, 120, 126, 
	128, 132, 138, 141, 146, 152, 154, 158, 
	164, 167, 174, 179, 185, 190, 194
};

static const char _deserialize_json_indicies[] = {
	0, 2, 0, 1, 3, 4, 3, 1, 
	5, 6, 7, 8, 1, 9, 10, 1, 
	11, 1, 11, 12, 11, 1, 12, 13, 
	14, 12, 15, 1, 16, 17, 1, 18, 
	19, 20, 18, 1, 21, 3, 22, 21, 
	1, 18, 19, 20, 18, 17, 1, 23, 
	1, 23, 24, 23, 1, 24, 25, 26, 
	24, 27, 1, 28, 29, 1, 30, 31, 
	32, 30, 1, 30, 31, 32, 30, 29, 
	1, 33, 1, 34, 1, 34, 35, 34, 
	1, 35, 36, 35, 37, 1, 38, 39, 
	40, 38, 1, 38, 39, 40, 38, 41, 
	1, 42, 43, 1, 44, 1, 44, 45, 
	44, 1, 45, 46, 47, 45, 48, 1, 
	49, 50, 1, 51, 52, 53, 51, 1, 
	51, 52, 53, 51, 50, 1, 54, 1, 
	54, 55, 54, 1, 55, 56, 57, 55, 
	58, 1, 59, 60, 1, 61, 62, 63, 
	61, 1, 61, 62, 63, 61, 60, 1, 
	64, 1, 64, 65, 64, 1, 65, 66, 
	67, 65, 68, 1, 69, 69, 1, 70, 
	71, 71, 71, 71, 71, 1, 72, 73, 
	74, 72, 1, 72, 73, 74, 72, 75, 
	1, 76, 77, 78, 76, 1, 0, 2, 
	0, 1, 1, 0
};

static const char _deserialize_json_trans_targs[] = {
	1, 0, 2, 2, 3, 4, 18, 24, 
	37, 5, 12, 6, 7, 8, 9, 11, 
	9, 11, 10, 2, 44, 10, 44, 13, 
	14, 15, 16, 17, 16, 17, 10, 2, 
	44, 19, 20, 21, 22, 23, 10, 2, 
	44, 23, 25, 31, 26, 27, 28, 29, 
	30, 29, 30, 10, 2, 44, 32, 33, 
	34, 35, 36, 35, 36, 10, 2, 44, 
	38, 39, 40, 42, 43, 41, 10, 41, 
	10, 2, 44, 43, 44, 45, 46
};

static const char _deserialize_json_trans_actions[] = {
	0, 0, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 5, 5, 5, 
	0, 0, 17, 17, 33, 0, 3, 0, 
	0, 5, 5, 5, 0, 0, 19, 19, 
	36, 0, 0, 0, 5, 5, 11, 11, 
	24, 0, 0, 0, 0, 0, 5, 5, 
	5, 0, 0, 13, 13, 27, 0, 0, 
	5, 5, 5, 0, 0, 15, 15, 30, 
	0, 0, 0, 5, 5, 5, 7, 0, 
	9, 9, 21, 0, 0, 0, 0
};

static const int deserialize_json_start = 1;
static const int deserialize_json_first_final = 44;
static const int deserialize_json_error = 0;

static const int deserialize_json_en_main = 1;


#line 97 "hb-buffer-deserialize-json.rl"


static hb_bool_t
_hb_buffer_deserialize_glyphs_json (hb_buffer_t *buffer,
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
  if (p < pe && *p == (buffer->len ? ',' : '['))
  {
    *end_ptr = ++p;
  }

  const char *tok = NULL;
  int cs;
  hb_glyph_info_t info;
  hb_glyph_position_t pos;
  
#line 191 "hb-buffer-deserialize-json.c"
	{
	cs = deserialize_json_start;
	}

#line 194 "hb-buffer-deserialize-json.c"
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
	_keys = _deserialize_json_trans_keys + _deserialize_json_key_offsets[cs];
	_trans = _deserialize_json_index_offsets[cs];

	_klen = _deserialize_json_single_lengths[cs];
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

	_klen = _deserialize_json_range_lengths[cs];
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
	_trans = _deserialize_json_indicies[_trans];
	cs = _deserialize_json_trans_targs[_trans];

	if ( _deserialize_json_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _deserialize_json_actions + _deserialize_json_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 38 "hb-buffer-deserialize-json.rl"
	{
	memset (&info, 0, sizeof (info));
	memset (&pos , 0, sizeof (pos ));
}
	break;
	case 1:
#line 43 "hb-buffer-deserialize-json.rl"
	{
	buffer->add_info (info);
	if (buffer->in_error)
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 2:
#line 51 "hb-buffer-deserialize-json.rl"
	{
	tok = p;
}
	break;
	case 3:
#line 55 "hb-buffer-deserialize-json.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 4:
#line 62 "hb-buffer-deserialize-json.rl"
	{ if (!parse_uint (tok, p, &info.codepoint)) return false; }
	break;
	case 5:
#line 63 "hb-buffer-deserialize-json.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 6:
#line 64 "hb-buffer-deserialize-json.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 7:
#line 65 "hb-buffer-deserialize-json.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 8:
#line 66 "hb-buffer-deserialize-json.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 9:
#line 67 "hb-buffer-deserialize-json.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
#line 313 "hb-buffer-deserialize-json.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 125 "hb-buffer-deserialize-json.rl"


  *end_ptr = p;

  return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_JSON_HH */
