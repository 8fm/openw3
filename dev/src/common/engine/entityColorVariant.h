#pragma once

struct CColorShift
{		  
	DECLARE_RTTI_STRUCT( CColorShift );

public:
	Uint16	m_hue;
	Int8	m_saturation;
	Int8	m_luminance;

	CColorShift() : m_hue( 0 ), m_saturation( 0 ), m_luminance( 0 ) {}

	RED_INLINE bool operator== ( const CColorShift& colorShift ) const
	{
		return m_hue == colorShift.m_hue && m_saturation == colorShift.m_saturation 
			&& m_luminance == colorShift.m_luminance;
	}

	RED_INLINE bool IsNoShift() const
	{
		return m_hue == 0 && m_saturation == 0 && m_luminance == 0;
	}

	void CalculateColorShiftMatrix( Matrix& coloringMatrix ) const;
};

BEGIN_CLASS_RTTI( CColorShift )
	PROPERTY_EDIT_RANGE( m_hue, TXT( "Hue shift" ), 0, 360 )
	PROPERTY_EDIT_RANGE( m_saturation, TXT( "Saturation shift" ), -100, 100 )
	PROPERTY_EDIT_RANGE( m_luminance, TXT( "Luminance shift" ), -100, 100 )
END_CLASS_RTTI()
