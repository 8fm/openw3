
#include "build.h"
#include "controlRigPropertySet.h"

IMPLEMENT_ENGINE_CLASS( TCrPropertySet );

TCrPropertySet::TCrPropertySet()
	: m_shoulderWeight( 0.2f )
	, m_shoulderLimitUpDeg( 45.f )
	, m_shoulderLimitDownDeg( -20.f )
	, m_shoulderLimitLeftDeg( 45.f )
	, m_shoulderLimitRightDeg( 45.f )
{
	
}
