
#include "build.h"
#include "lookAtParam.h"

IMPLEMENT_ENGINE_CLASS( CLookAtStaticParam );

CLookAtStaticParam::CLookAtStaticParam()
	: m_maxLookAtLevel( LL_Body )
	, m_maxVerAngle( 90.f )
	, m_maxHorAngle( 90.f )
	, m_firstWeight( 1.f )
	, m_secWeight( 1.f )
	, m_responsiveness( 1.f )
{

}

ELookAtLevel CLookAtStaticParam::GetLevel() const
{
	return LL_Body;
}

Float CLookAtStaticParam::GetMaxAngleVer() const
{
	return 90.f;
}

Float CLookAtStaticParam::GetMaxAngleHor() const
{
	return 90.f;
}

Float CLookAtStaticParam::GetResponsiveness() const
{
	return m_responsiveness;
}

Float CLookAtStaticParam::GetWeight( Uint32 segmentNum ) const
{
	if ( segmentNum == 0 )
	{
		return m_firstWeight;
	}
	else if ( segmentNum == 1 )
	{
		return m_secWeight;
	}
	else
	{
		return 1.f;
	}
}
