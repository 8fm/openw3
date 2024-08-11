
#include "build.h"
#include "voiceWeightCurve.h"

IMPLEMENT_ENGINE_CLASS( SVoiceWeightCurve );

SVoiceWeightCurve::SVoiceWeightCurve()
	: m_useCurve( false )
	, m_timeOffset( 0.f )
	, m_valueOffset( 0.f )
	, m_valueMulPre( 1.f )
	, m_valueMulPost( 1.f )
{

}

Float SVoiceWeightCurve::GetValue( Float time ) const
{
	return 0.f;
}
