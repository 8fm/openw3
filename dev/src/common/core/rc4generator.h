/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CRC4Generator
{
	// Code from Wikipedia :)

public:
	CRC4Generator( const AnsiChar* key, Uint32 keyLength )
	{
		for( m_i = 0; m_i < 256; ++m_i )
		{
			m_state[ m_i ] = ( AnsiChar ) m_i;
		}

		for( m_i = m_j = 0; m_i < 256; ++m_i )
		{
			m_j = ( m_j + key[ m_i % keyLength ] + m_state[ m_i ] ) & 255;
			Swap( m_state[ m_i ], m_state[ m_j ] );
		}

		m_i = m_j = 0;
	}

	AnsiChar NextByte()
	{
		m_i = ( m_i + 1 ) & 255;
		m_j = ( m_j + m_state[ m_i ] ) & 255;

		Swap( m_state[ m_i ], m_state[ m_j ] );

		return m_state[ ( m_state[ m_i ] + m_state[ m_j ] ) & 255 ];
	}

private:
	AnsiChar	m_state[ 256 ];
	Uint32		m_i;
	Uint32		m_j;

	template< typename T >
	void Swap( T& a, T& b )
	{
		T temp = a;
		a = b;
		b = temp;
	}
};
