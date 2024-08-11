
#line 1 "hb-ot-shape-complex-myanmar-machine.rl"
/*
 * Copyright © 2011,2012  Google, Inc.
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

#ifndef HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH

#include "hb-private.hh"


#line 33 "hb-ot-shape-complex-myanmar-machine.c"
static const char _myanmar_syllable_machine_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9
};

static const short _myanmar_syllable_machine_key_offsets[] = {
	0, 23, 40, 46, 49, 54, 61, 66, 
	70, 80, 87, 96, 104, 107, 122, 133, 
	143, 152, 160, 173, 185, 199, 215, 221, 
	224, 229, 236, 241, 245, 255, 262, 271, 
	279, 296, 312, 333, 348, 359, 369, 378, 
	386, 399, 411, 425, 440
};

static const unsigned char _myanmar_syllable_machine_trans_keys[] = {
	3u, 4u, 8u, 10u, 11u, 16u, 18u, 19u, 
	21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u, 
	29u, 30u, 31u, 1u, 2u, 5u, 6u, 3u, 
	4u, 8u, 10u, 18u, 21u, 22u, 23u, 24u, 
	25u, 26u, 27u, 28u, 29u, 30u, 5u, 6u, 
	8u, 18u, 25u, 29u, 5u, 6u, 8u, 5u, 
	6u, 8u, 25u, 29u, 5u, 6u, 3u, 8u, 
	10u, 18u, 25u, 5u, 6u, 8u, 18u, 25u, 
	5u, 6u, 8u, 25u, 5u, 6u, 3u, 8u, 
	10u, 18u, 21u, 25u, 26u, 29u, 5u, 6u, 
	3u, 8u, 10u, 25u, 29u, 5u, 6u, 3u, 
	8u, 10u, 18u, 25u, 26u, 29u, 5u, 6u, 
	3u, 8u, 10u, 25u, 26u, 29u, 5u, 6u, 
	16u, 1u, 2u, 3u, 8u, 10u, 18u, 21u, 
	22u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 
	5u, 6u, 3u, 8u, 10u, 18u, 25u, 26u, 
	27u, 28u, 29u, 5u, 6u, 3u, 8u, 10u, 
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 3u, 
	8u, 10u, 25u, 26u, 27u, 29u, 5u, 6u, 
	3u, 8u, 10u, 25u, 27u, 29u, 5u, 6u, 
	3u, 8u, 10u, 18u, 21u, 23u, 25u, 26u, 
	27u, 28u, 29u, 5u, 6u, 3u, 8u, 10u, 
	18u, 21u, 25u, 26u, 27u, 28u, 29u, 5u, 
	6u, 3u, 8u, 10u, 18u, 21u, 22u, 23u, 
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 3u, 
	4u, 8u, 10u, 18u, 21u, 22u, 23u, 24u, 
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 8u, 
	18u, 25u, 29u, 5u, 6u, 8u, 5u, 6u, 
	8u, 25u, 29u, 5u, 6u, 3u, 8u, 10u, 
	18u, 25u, 5u, 6u, 8u, 18u, 25u, 5u, 
	6u, 8u, 25u, 5u, 6u, 3u, 8u, 10u, 
	18u, 21u, 25u, 26u, 29u, 5u, 6u, 3u, 
	8u, 10u, 25u, 29u, 5u, 6u, 3u, 8u, 
	10u, 18u, 25u, 26u, 29u, 5u, 6u, 3u, 
	8u, 10u, 25u, 26u, 29u, 5u, 6u, 3u, 
	4u, 8u, 10u, 18u, 21u, 22u, 23u, 24u, 
	25u, 26u, 27u, 28u, 29u, 30u, 5u, 6u, 
	3u, 4u, 8u, 10u, 18u, 21u, 22u, 23u, 
	24u, 25u, 26u, 27u, 28u, 29u, 5u, 6u, 
	3u, 8u, 10u, 11u, 16u, 18u, 19u, 21u, 
	22u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 
	30u, 1u, 2u, 4u, 6u, 3u, 8u, 10u, 
	18u, 21u, 22u, 23u, 24u, 25u, 26u, 27u, 
	28u, 29u, 5u, 6u, 3u, 8u, 10u, 18u, 
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 3u, 
	8u, 10u, 25u, 26u, 27u, 28u, 29u, 5u, 
	6u, 3u, 8u, 10u, 25u, 26u, 27u, 29u, 
	5u, 6u, 3u, 8u, 10u, 25u, 27u, 29u, 
	5u, 6u, 3u, 8u, 10u, 18u, 21u, 23u, 
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 3u, 
	8u, 10u, 18u, 21u, 25u, 26u, 27u, 28u, 
	29u, 5u, 6u, 3u, 8u, 10u, 18u, 21u, 
	22u, 23u, 25u, 26u, 27u, 28u, 29u, 5u, 
	6u, 3u, 8u, 10u, 18u, 21u, 22u, 23u, 
	24u, 25u, 26u, 27u, 28u, 29u, 4u, 6u, 
	8u, 0
};

static const char _myanmar_syllable_machine_single_lengths[] = {
	19, 15, 4, 1, 3, 5, 3, 2, 
	8, 5, 7, 6, 1, 13, 9, 8, 
	7, 6, 11, 10, 12, 14, 4, 1, 
	3, 5, 3, 2, 8, 5, 7, 6, 
	15, 14, 17, 13, 9, 8, 7, 6, 
	11, 10, 12, 13, 1
};

static const char _myanmar_syllable_machine_range_lengths[] = {
	2, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 2, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 0
};

static const short _myanmar_syllable_machine_index_offsets[] = {
	0, 22, 39, 45, 48, 53, 60, 65, 
	69, 79, 86, 95, 103, 106, 121, 132, 
	142, 151, 159, 172, 184, 198, 214, 220, 
	223, 228, 235, 240, 244, 254, 261, 270, 
	278, 295, 311, 331, 346, 357, 367, 376, 
	384, 397, 409, 423, 438
};

static const char _myanmar_syllable_machine_indicies[] = {
	2, 3, 5, 6, 1, 7, 8, 1, 
	9, 10, 11, 12, 13, 14, 15, 16, 
	17, 18, 19, 1, 4, 0, 21, 22, 
	24, 25, 26, 27, 28, 29, 30, 31, 
	32, 33, 34, 35, 36, 23, 20, 24, 
	37, 31, 35, 23, 20, 24, 23, 20, 
	24, 31, 35, 23, 20, 38, 24, 31, 
	39, 31, 23, 20, 24, 39, 31, 23, 
	20, 24, 31, 23, 20, 21, 24, 25, 
	40, 40, 31, 41, 35, 23, 20, 21, 
	24, 25, 31, 35, 23, 20, 21, 24, 
	25, 40, 31, 41, 35, 23, 20, 21, 
	24, 25, 31, 41, 35, 23, 20, 1, 
	1, 20, 21, 24, 25, 26, 27, 28, 
	29, 30, 31, 32, 33, 34, 35, 23, 
	20, 21, 24, 25, 34, 31, 32, 33, 
	34, 35, 23, 20, 21, 24, 25, 31, 
	32, 33, 34, 35, 23, 20, 21, 24, 
	25, 31, 32, 33, 35, 23, 20, 21, 
	24, 25, 31, 33, 35, 23, 20, 21, 
	24, 25, 34, 27, 29, 31, 32, 33, 
	34, 35, 23, 20, 21, 24, 25, 34, 
	27, 31, 32, 33, 34, 35, 23, 20, 
	21, 24, 25, 34, 27, 28, 29, 31, 
	32, 33, 34, 35, 23, 20, 21, 22, 
	24, 25, 26, 27, 28, 29, 30, 31, 
	32, 33, 34, 35, 23, 20, 5, 43, 
	13, 17, 3, 42, 5, 3, 42, 5, 
	13, 17, 3, 42, 44, 5, 13, 45, 
	13, 3, 42, 5, 45, 13, 3, 42, 
	5, 13, 3, 42, 2, 5, 6, 46, 
	46, 13, 47, 17, 3, 42, 2, 5, 
	6, 13, 17, 3, 42, 2, 5, 6, 
	46, 13, 47, 17, 3, 42, 2, 5, 
	6, 13, 47, 17, 3, 42, 21, 22, 
	24, 25, 48, 27, 28, 29, 30, 31, 
	32, 33, 34, 35, 36, 23, 20, 21, 
	49, 24, 25, 26, 27, 28, 29, 30, 
	31, 32, 33, 34, 35, 23, 20, 2, 
	5, 6, 1, 1, 8, 1, 9, 10, 
	11, 12, 13, 14, 15, 16, 17, 18, 
	1, 3, 42, 2, 5, 6, 8, 9, 
	10, 11, 12, 13, 14, 15, 16, 17, 
	3, 42, 2, 5, 6, 16, 13, 14, 
	15, 16, 17, 3, 42, 2, 5, 6, 
	13, 14, 15, 16, 17, 3, 42, 2, 
	5, 6, 13, 14, 15, 17, 3, 42, 
	2, 5, 6, 13, 15, 17, 3, 42, 
	2, 5, 6, 16, 9, 11, 13, 14, 
	15, 16, 17, 3, 42, 2, 5, 6, 
	16, 9, 13, 14, 15, 16, 17, 3, 
	42, 2, 5, 6, 16, 9, 10, 11, 
	13, 14, 15, 16, 17, 3, 42, 2, 
	5, 6, 8, 9, 10, 11, 12, 13, 
	14, 15, 16, 17, 3, 42, 51, 50, 
	0
};

static const char _myanmar_syllable_machine_trans_targs[] = {
	0, 1, 22, 0, 0, 23, 29, 32, 
	35, 36, 40, 41, 42, 25, 38, 39, 
	37, 28, 43, 44, 0, 2, 12, 0, 
	3, 9, 13, 14, 18, 19, 20, 5, 
	16, 17, 15, 8, 21, 4, 6, 7, 
	10, 11, 0, 24, 26, 27, 30, 31, 
	33, 34, 0, 0
};

static const char _myanmar_syllable_machine_trans_actions[] = {
	13, 0, 0, 11, 7, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 15, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 17, 0, 0, 0, 0, 0, 
	0, 0, 19, 9
};

static const char _myanmar_syllable_machine_to_state_actions[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0
};

static const char _myanmar_syllable_machine_from_state_actions[] = {
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0
};

static const short _myanmar_syllable_machine_eof_trans[] = {
	0, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 43, 43, 
	43, 43, 43, 43, 43, 43, 43, 43, 
	21, 21, 43, 43, 43, 43, 43, 43, 
	43, 43, 43, 43, 51
};

static const int myanmar_syllable_machine_start = 0;
static const int myanmar_syllable_machine_first_final = 0;
static const int myanmar_syllable_machine_error = -1;

static const int myanmar_syllable_machine_en_main = 0;


#line 36 "hb-ot-shape-complex-myanmar-machine.rl"



#line 93 "hb-ot-shape-complex-myanmar-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", last, p+1, #syllable_type); \
    for (unsigned int i = last; i < p+1; i++) \
      info[i].syllable() = (syllable_serial << 4) | syllable_type; \
    last = p+1; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END

static void
find_syllables (hb_buffer_t *buffer)
{
  unsigned int p, pe, eof, ts HB_UNUSED, te HB_UNUSED, act HB_UNUSED;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 266 "hb-ot-shape-complex-myanmar-machine.c"
	{
	cs = myanmar_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 114 "hb-ot-shape-complex-myanmar-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int last = 0;
  unsigned int syllable_serial = 1;
  
#line 279 "hb-ot-shape-complex-myanmar-machine.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _myanmar_syllable_machine_actions + _myanmar_syllable_machine_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
#line 1 "NONE"
	{ts = p;}
	break;
#line 296 "hb-ot-shape-complex-myanmar-machine.c"
		}
	}

	_keys = _myanmar_syllable_machine_trans_keys + _myanmar_syllable_machine_key_offsets[cs];
	_trans = _myanmar_syllable_machine_index_offsets[cs];

	_klen = _myanmar_syllable_machine_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( ( info[p].myanmar_category()) < *_mid )
				_upper = _mid - 1;
			else if ( ( info[p].myanmar_category()) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _myanmar_syllable_machine_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( ( info[p].myanmar_category()) < _mid[0] )
				_upper = _mid - 2;
			else if ( ( info[p].myanmar_category()) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _myanmar_syllable_machine_indicies[_trans];
_eof_trans:
	cs = _myanmar_syllable_machine_trans_targs[_trans];

	if ( _myanmar_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _myanmar_syllable_machine_actions + _myanmar_syllable_machine_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
#line 85 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (consonant_syllable); }}
	break;
	case 3:
#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (non_myanmar_cluster); }}
	break;
	case 4:
#line 87 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (punctuation_cluster); }}
	break;
	case 5:
#line 88 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (broken_cluster); }}
	break;
	case 6:
#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (non_myanmar_cluster); }}
	break;
	case 7:
#line 85 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (consonant_syllable); }}
	break;
	case 8:
#line 88 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 9:
#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (non_myanmar_cluster); }}
	break;
#line 385 "hb-ot-shape-complex-myanmar-machine.c"
		}
	}

_again:
	_acts = _myanmar_syllable_machine_actions + _myanmar_syllable_machine_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 396 "hb-ot-shape-complex-myanmar-machine.c"
		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _myanmar_syllable_machine_eof_trans[cs] > 0 ) {
		_trans = _myanmar_syllable_machine_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 123 "hb-ot-shape-complex-myanmar-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH */
