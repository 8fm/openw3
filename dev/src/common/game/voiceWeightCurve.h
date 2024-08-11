
#pragma once

struct SVoiceWeightCurve
{
	DECLARE_RTTI_STRUCT( SVoiceWeightCurve )

	Bool		m_useCurve;
	SCurveData	m_curve;
	Float		m_timeOffset;
	Float		m_valueOffset;
	Float		m_valueMulPre;
	Float		m_valueMulPost;

	SVoiceWeightCurve();

	Float GetValue( Float time ) const;
};

BEGIN_CLASS_RTTI( SVoiceWeightCurve );
	PROPERTY_EDIT( m_useCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT( "" ), TXT("VoiceCurveDataEditor") );
	PROPERTY_EDIT( m_timeOffset, TXT( "" ) );
	PROPERTY_EDIT( m_valueMulPre, TXT( "" ) );
	PROPERTY_EDIT( m_valueOffset, TXT( "" ) );
	PROPERTY_EDIT( m_valueMulPost, TXT( "" ) );
END_CLASS_RTTI();
