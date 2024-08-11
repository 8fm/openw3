#pragma once

#include "entity.h"

/////////////////////////////////////////////////////////
// CSpeedConfig
class CSpeedConfig
{
public :
	// User interface
	RED_INLINE const Float & GetWalkSpeedAbs()const		{ return m_walkSpeedAbs;		}
	RED_INLINE const Float & GetSlowRunSpeedAbs()const	{ return m_slowRunSpeedAbs;		}
	RED_INLINE const Float & GetFastRunSpeedAbs()const	{ return m_fastRunSpeedAbs;		}
	RED_INLINE const Float & GetSprintSpeedAbs()const		{ return m_sprintSpeedAbs;		}

	RED_INLINE const Float & GetSpeedRelSpan()const		{ return m_sprintSpeedRel;		}

	Float ConvertSpeedRelToAbs( Float speedRel ) const;
	Float ConvertSpeedAbsToRel( Float speedAbs ) const;
	Float GetMoveTypeRelativeMoveSpeed( EMoveType moveType )const;

public :
	// Engine interface

	CSpeedConfig( CName key, Float walkSpeedAbs, Float slowRunSpeedAbs, Float fastRunSpeedAbs, Float sprintSpeedAbs, Float walkSpeedRel, Float slowRunSpeedRel, Float fastRunSpeedRel, Float sprintSpeedRel );

private :
	Int32 m_refCount;
	CName m_key;

	Float m_walkSpeedAbs;
	Float m_slowRunSpeedAbs;
	Float m_fastRunSpeedAbs;
	Float m_sprintSpeedAbs;
	
	Float m_walkSpeedRel;
	Float m_slowRunSpeedRel;
	Float m_fastRunSpeedRel;
	Float m_sprintSpeedRel;
};

//////////////////////////////////////////////////////////////////
// CSpeedConfigManager
class CSpeedConfigManager
{
public :
	// User interface

	/// return a the speedConfig corresponding to the key ( see speedConfig.xml )
	const CSpeedConfig *const GetSpeedConfig( CName key )const;
public:
	//Engine interface

	CSpeedConfigManager();
	virtual ~CSpeedConfigManager();



protected :
	THashMap< CName, CSpeedConfig* >	m_speedConfigMap;
};