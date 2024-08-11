/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
* 
* CRC code taken and modified from zlib:
* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.2.8, April 28th, 2013

  Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu


  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
  (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
*/

#include "crc.h"

namespace Red { namespace System {

/*
  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The first table is simply the CRC of all possible eight bit values.  This is
  all the information needed to generate CRCs on data a byte at a time for all
  combinations of CRC register values and incoming bytes.  The remaining tables
  allow for word-at-a-time CRC calculation for both big-endian and little-
  endian machines, where a word is four bytes.
  */

Bool CRC32::initialized = false;
Uint32 CRC32::m_table[ CRC32::INNER_SIZE ][ CRC32::OUTER_SIZE ];

CRC32::CRC32()
{
	Initialize();
}

void CRC32::Initialize()
{
	if( initialized )
		return;

	/* Reverse the bytes in a 32-bit value */
#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
	(((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

	Uint32 c;
	Uint32 poly; /* polynomial exclusive-or pattern */
	
	/* terms of polynomial defining this crc (except x^32): */
	const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

	/* make exclusive-or pattern from polynomial (0xedb88320UL) */
	poly = 0;
	for ( Uint32 n = 0; n < (sizeof(p)/sizeof(Uint8)); ++n )
	{
		poly |= 1 << (31 - p[n]);
	}

	/* generate a crc for every 8-bit value */
	for ( Uint32 n = 0; n < OUTER_SIZE; ++n )
	{
		c = n;
		for ( Uint32 k = 0; k < INNER_SIZE; ++k )
		{
			c = c & 1 ? poly ^ (c >> 1) : c >> 1;
		}

		m_table[0][n] = c;
	}

	/* generate crc for each value followed by one, two, and three zeros,
		and then the byte reversal of those as well as the first table */
	for ( Uint32 n = 0; n < OUTER_SIZE; ++n )
	{
		c = m_table[0][n];
		m_table[4][n] = ZSWAP32(c);
		for ( Uint32 k = 1; k < 4; ++k)
		{
			c = m_table[0][c & 0xff] ^ (c >> 8);
			m_table[k][n] = c;
			m_table[k + 4][n] = ZSWAP32(c);
		}
	}

	initialized = true;
}

Uint32 CRC32::littleEndian( Uint32 c, const void* buf, Uint32 len ) const
{
#define DOLIT4( buf ) c ^= *buf++; \
	c = m_table[3][c & 0xff] ^ m_table[2][(c >> 8) & 0xff] ^ \
	m_table[1][(c >> 16) & 0xff] ^ m_table[0][c >> 24]
#define DOLIT32( buf ) DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf ); DOLIT4( buf )


	c = ~c;
	
	const Uint8* buf8 = static_cast< const Uint8* >( buf );

	// Iterate over any data that isn't aligned to a 4 byte boundary
	while ( len && ( reinterpret_cast< ptrdiff_t >( buf8 ) & 3 ) )
	{
		c = m_table[0][(c ^ *buf8++) & 0xff] ^ (c >> 8);
		--len;
	}

	const Uint32* buf32 = reinterpret_cast< const Uint32* >( buf8 );

	// Iterate over 32 byte chunks at a time
	while (len >= 32)
	{
		DOLIT32( buf32 );
		len -= 32;
	}

	// Iterate over the remaining < 32 byte chunks
	while (len >= 4)
	{
		DOLIT4( buf32 );
		len -= 4;
	}

	// Iterate over the final < 4 bytes
	buf8 = reinterpret_cast< const Uint8* >( buf32 );

	if (len)
	{
		do
		{
			c = m_table[0][(c ^ *buf8++) & 0xff] ^ (c >> 8);
		} while (--len);
	}
	
	c = ~c;

	return c;
}

} } // namespace Red { namespace System {
