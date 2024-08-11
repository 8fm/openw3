
#line 1 "hb-ot-shape-complex-sea-machine.rl"
/*
 * Copyright © 2011,2012,2013  Google, Inc.
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

#ifndef HB_OT_SHAPE_COMPLEX_SEA_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_SEA_MACHINE_HH

#include "hb-private.hh"


#line 33 "hb-ot-shape-complex-sea-machine.c"
static const char _sea_syllable_machine_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8
};

static const char _sea_syllable_machine_key_offsets[] = {
	0, 1, 2, 12, 19, 26
};

static const unsigned char _sea_syllable_machine_trans_keys[] = {
	1u, 1u, 3u, 4u, 10u, 12u, 17u, 22u, 
	1u, 2u, 26u, 29u, 3u, 4u, 10u, 17u, 
	22u, 26u, 29u, 3u, 4u, 10u, 17u, 22u, 
	26u, 29u, 1u, 0
};

static const char _sea_syllable_machine_single_lengths[] = {
	1, 1, 6, 5, 5, 1
};

static const char _sea_syllable_machine_range_lengths[] = {
	0, 0, 2, 1, 1, 0
};

static const char _sea_syllable_machine_index_offsets[] = {
	0, 2, 4, 13, 20, 27
};

static const char _sea_syllable_machine_indicies[] = {
	1, 0, 3, 2, 3, 5, 3, 1, 
	3, 3, 1, 3, 4, 1, 7, 1, 
	1, 1, 1, 6, 3, 9, 3, 3, 
	3, 3, 8, 3, 10, 0
};

static const char _sea_syllable_machine_trans_targs[] = {
	2, 3, 2, 4, 2, 5, 2, 0, 
	2, 1, 2
};

static const char _sea_syllable_machine_trans_actions[] = {
	15, 5, 17, 5, 7, 0, 9, 0, 
	11, 0, 13
};

static const char _sea_syllable_machine_to_state_actions[] = {
	0, 0, 1, 0, 0, 0
};

static const char _sea_syllable_machine_from_state_actions[] = {
	0, 0, 3, 0, 0, 0
};

static const char _sea_syllable_machine_eof_trans[] = {
	1, 3, 0, 7, 9, 11
};

static const int sea_syllable_machine_start = 2;
static const int sea_syllable_machine_first_final = 2;
static const int sea_syllable_machine_error = -1;

static const int sea_syllable_machine_en_main = 2;


#line 36 "hb-ot-shape-complex-sea-machine.rl"



#line 67 "hb-ot-shape-complex-sea-machine.rl"


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
  
#line 117 "hb-ot-shape-complex-sea-machine.c"
	{
	cs = sea_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 88 "hb-ot-shape-complex-sea-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int last = 0;
  unsigned int syllable_serial = 1;
  
#line 130 "hb-ot-shape-complex-sea-machine.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _sea_syllable_machine_actions + _sea_syllable_machine_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
#line 1 "NONE"
	{ts = p;}
	break;
#line 147 "hb-ot-shape-complex-sea-machine.c"
		}
	}

	_keys = _sea_syllable_machine_trans_keys + _sea_syllable_machine_key_offsets[cs];
	_trans = _sea_syllable_machine_index_offsets[cs];

	_klen = _sea_syllable_machine_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( ( info[p].sea_category()) < *_mid )
				_upper = _mid - 1;
			else if ( ( info[p].sea_category()) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _sea_syllable_machine_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( ( info[p].sea_category()) < _mid[0] )
				_upper = _mid - 2;
			else if ( ( info[p].sea_category()) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _sea_syllable_machine_indicies[_trans];
_eof_trans:
	cs = _sea_syllable_machine_trans_targs[_trans];

	if ( _sea_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _sea_syllable_machine_actions + _sea_syllable_machine_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 3:
#line 63 "hb-ot-shape-complex-sea-machine.rl"
	{te = p+1;{ found_syllable (non_sea_cluster); }}
	break;
	case 4:
#line 61 "hb-ot-shape-complex-sea-machine.rl"
	{te = p;p--;{ found_syllable (consonant_syllable); }}
	break;
	case 5:
#line 62 "hb-ot-shape-complex-sea-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 6:
#line 63 "hb-ot-shape-complex-sea-machine.rl"
	{te = p;p--;{ found_syllable (non_sea_cluster); }}
	break;
	case 7:
#line 61 "hb-ot-shape-complex-sea-machine.rl"
	{{p = ((te))-1;}{ found_syllable (consonant_syllable); }}
	break;
	case 8:
#line 62 "hb-ot-shape-complex-sea-machine.rl"
	{{p = ((te))-1;}{ found_syllable (broken_cluster); }}
	break;
#line 233 "hb-ot-shape-complex-sea-machine.c"
		}
	}

_again:
	_acts = _sea_syllable_machine_actions + _sea_syllable_machine_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 244 "hb-ot-shape-complex-sea-machine.c"
		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _sea_syllable_machine_eof_trans[cs] > 0 ) {
		_trans = _sea_syllable_machine_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 97 "hb-ot-shape-complex-sea-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_SEA_MACHINE_HH */
