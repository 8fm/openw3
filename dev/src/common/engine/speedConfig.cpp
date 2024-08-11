#include "build.h"
#include "speedConfig.h"

//////////////////////////////////////////////////////////
// CSpeedConfig
CSpeedConfig::CSpeedConfig( CName key, Float walkSpeedAbs, Float slowRunSpeedAbs, Float fastRunSpeedAbs, Float sprintSpeedAbs, Float walkSpeedRel, Float slowRunSpeedRel, Float fastRunSpeedRel, Float sprintSpeedRel )
	: m_key( key )
	, m_walkSpeedAbs( walkSpeedAbs )
	, m_slowRunSpeedAbs( slowRunSpeedAbs )
	, m_fastRunSpeedAbs( fastRunSpeedAbs )
	, m_sprintSpeedAbs( sprintSpeedAbs )

	, m_walkSpeedRel( walkSpeedRel )
	, m_slowRunSpeedRel( slowRunSpeedRel )
	, m_fastRunSpeedRel( fastRunSpeedRel )
	, m_sprintSpeedRel( sprintSpeedRel )
{
}

namespace
{
	Float CalcProp( Float arg, Float x1, Float x2, Float y1, Float y2 )
	{
		Bool sign = arg >= 0 ? true : false; 
		Float x = MAbs( arg );

		Float p = ( x - x1 ) / ( x2 - x1 );
		Float res = ( y2 - y1 ) * p + y1;

		return sign ? res : -res;
	}
}

Float CSpeedConfig::ConvertSpeedAbsToRel( Float speedAbs ) const
{
	Float aSpeedAbs = MAbs( speedAbs );

	// Need to be <= because if m_walkSpeed == m_slowRunSpeed then we will end up in m_slowRunSpeed ( if speedAbs == m_walkSpeed )
	if ( aSpeedAbs <= m_walkSpeedAbs )
	{
		return CalcProp( speedAbs, 0.0f, m_walkSpeedAbs, 0.0f, m_walkSpeedRel );
	}
	else if ( aSpeedAbs <= m_slowRunSpeedAbs  )
	{
		return CalcProp( speedAbs, m_walkSpeedAbs, m_slowRunSpeedAbs, m_walkSpeedRel, m_slowRunSpeedRel );
	}
	else if ( aSpeedAbs <= m_fastRunSpeedAbs  )
	{
		return CalcProp( speedAbs, m_slowRunSpeedAbs, m_fastRunSpeedAbs, m_slowRunSpeedRel, m_fastRunSpeedRel );
	}
	else if ( aSpeedAbs <= m_sprintSpeedAbs  )
	{
		return CalcProp( speedAbs, m_fastRunSpeedAbs, m_sprintSpeedAbs, m_fastRunSpeedRel, m_sprintSpeedRel );
	}
	else
	{
		return m_sprintSpeedRel;
	}
}


Float CSpeedConfig::ConvertSpeedRelToAbs( Float speedRel ) const
{
	Float aSpeedRel = MAbs( speedRel );
	// Need to be <= because if m_walkSpeedRel == m_slowRunSpeedRel then we will end up in m_slowRunSpeed ( if speedRel == m_walkSpeedRel )
	if ( aSpeedRel <= m_walkSpeedRel )
	{
		return CalcProp( speedRel, 0.0f, m_walkSpeedRel, 0.0f, m_walkSpeedAbs );
	}
	else if ( aSpeedRel <= m_slowRunSpeedRel  )
	{
		return CalcProp( speedRel, m_walkSpeedRel, m_slowRunSpeedRel, m_walkSpeedAbs, m_slowRunSpeedAbs );
	}
	else if ( aSpeedRel <= m_fastRunSpeedRel  )
	{
		return CalcProp( speedRel, m_slowRunSpeedRel, m_fastRunSpeedRel, m_slowRunSpeedAbs, m_fastRunSpeedAbs );
	}
	else if ( aSpeedRel <= m_sprintSpeedRel  )
	{
		return CalcProp( speedRel, m_fastRunSpeedRel, m_sprintSpeedRel, m_fastRunSpeedAbs, m_sprintSpeedAbs );
	}
	else
	{
		return m_sprintSpeedAbs;
	}
}

Float CSpeedConfig::GetMoveTypeRelativeMoveSpeed( EMoveType moveType )const
{
	switch( moveType )
	{
	case MT_Walk:
		return m_walkSpeedRel;
	case MT_Run:
		return m_slowRunSpeedRel;
	case MT_FastRun:
		return m_fastRunSpeedRel;
	case MT_Sprint:
		return m_sprintSpeedRel;
	}
	return 0.0;
}



//////////////////////////////////////////////////////////////////
// CSpeedConfigManager
const CSpeedConfig *const CSpeedConfigManager::GetSpeedConfig( CName key )const 
{ 
	CSpeedConfig * speedConfig;
	if ( m_speedConfigMap.Find( key, speedConfig ) )
	{
		return speedConfig;
	}
	RED_LOG_ERROR( RED_LOG_CHANNEL( SpeedConfig ), TXT(" speed config key %s does not exist ( see speedConfig.xml ) "), key.AsChar() );
	return nullptr;
}



CSpeedConfigManager::CSpeedConfigManager()
{
}

CSpeedConfigManager::~CSpeedConfigManager()
{
	for ( auto it = m_speedConfigMap.Begin(), end = m_speedConfigMap.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_speedConfigMap.Clear();
}
