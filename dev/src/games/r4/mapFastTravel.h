/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "r4MapPinEntity.h"

class CR4FastTravelEntity: public CR4MapPinEntity
{
	DECLARE_ENGINE_CLASS( CR4FastTravelEntity, CR4MapPinEntity, 0 );

public:
	CR4FastTravelEntity()
		: m_canBeReachedByBoat( false )
		, m_isHubExit( false )
	{}

protected:
	CName	m_spotName;
	CName	m_groupName;
	CName	m_teleportWayPointTag;
	Bool	m_canBeReachedByBoat;
	Bool	m_isHubExit;

public:
	CName	GetSpotName()				{ return m_spotName; }
	CName	GetGroupName()				{ return m_groupName; }
	CName	GetTeleportWayPointTag()	{ return m_teleportWayPointTag; }
	Bool	CanBeReachedByBoat()		{ return m_canBeReachedByBoat; }
	Bool	IsHubExit()					{ return m_isHubExit; }
};

BEGIN_CLASS_RTTI( CR4FastTravelEntity )
	PARENT_CLASS( CR4MapPinEntity )	
	PROPERTY( m_spotName )
	PROPERTY( m_groupName )
	PROPERTY_EDIT( m_teleportWayPointTag,	TXT("Teleport waypoint tag") )
	PROPERTY_EDIT( m_canBeReachedByBoat,	TXT("Can be reached by boat") )
	PROPERTY_EDIT( m_isHubExit,				TXT("Is hub exit") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

struct SUsedFastTravelEvent
{
	DECLARE_RTTI_STRUCT( SUsedFastTravelEvent );

	CName	m_eventName;
	CName	m_tag;
	Bool	m_onStart;

	SUsedFastTravelEvent() {};
	SUsedFastTravelEvent( const CName& eventName, const CName& tag, Bool onStart )
		: m_eventName( eventName )
		, m_tag( tag )
		, m_onStart( onStart )
	{};

	RED_INLINE Bool operator==( const SUsedFastTravelEvent& rhs ) const
	{
		return m_eventName == rhs.m_eventName;
	}
};

BEGIN_CLASS_RTTI( SUsedFastTravelEvent );
	PROPERTY( m_eventName );
	PROPERTY( m_tag );
	PROPERTY( m_onStart );
END_CLASS_RTTI();
