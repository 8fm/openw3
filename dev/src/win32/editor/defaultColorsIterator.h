
#pragma once

class DefaultColorsIterator
{
	Uint32	m_index;
	Bool	m_inf;

public:
	DefaultColorsIterator( Bool inf = false )
		: m_index( 0 )
		, m_inf( inf )
	{
		
	}

	RED_INLINE operator Bool () const
	{
		return m_inf ? true : m_index < 7;
	}

	RED_INLINE void operator++ ()
	{
		m_index++;
	}

	RED_INLINE Color operator* () const
	{
		return GetColor( m_index );
	}

	static Color GetColor( Uint32 i )
	{
		Uint32 colorMask = (i + 1) % 7;	
		if ( !colorMask ) colorMask = 1;

		return Color( colorMask & 1 ? 255 : 0, colorMask & 2 ? 255 : 0, colorMask & 4 ? 255 : 0 );
	}
};
