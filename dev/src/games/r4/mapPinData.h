/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/binaryStorage.h"

struct SMapPinType
{
	DECLARE_RTTI_STRUCT( SMapPinType )

	CName	m_typeName;
	String	m_icon;
	Float	m_radius;
	Color	m_color;
};

BEGIN_CLASS_RTTI( SMapPinType )
	PROPERTY_EDIT( m_typeName, TXT( "Name of map pin type, used for referencing it" ) );
PROPERTY_EDIT( m_icon, TXT( "Map pin icon to display") );
PROPERTY_EDIT( m_radius, TXT( "Radius of map pin area" ) );
PROPERTY_EDIT( m_color, TXT( "Color of radius area" ) );
END_CLASS_RTTI();

struct SMapPinInstance
{
	Vector	m_position;
	Float	m_radius;
	String	m_name;
	Uint32	m_id;
    CName   m_typeName;
	CName	m_tag;
	Bool	m_isNearby;

	SMapPinInstance()	{}
};

struct SMapPinState
{
    CGUID	m_questObjectiveGUID;
	CName   m_mapPinID;
	Bool	m_enabled;
};

struct SMapPinConfig
{
	DECLARE_RTTI_STRUCT( SMapPinConfig );

	TDynArray< SMapPinType >	m_pinTypes;
	TDynArray< CName >			m_alwaysTrackedPins;
	Float						m_distantUpdateTime;
	Float						m_nearbyRadius;
};

BEGIN_CLASS_RTTI( SMapPinConfig )
	PROPERTY_EDIT( m_pinTypes, TXT( "Types of map pins used in game" ) )
	PROPERTY_EDIT( m_alwaysTrackedPins, TXT( "Tags of pins that should be always tracked" ) );
	PROPERTY_EDIT( m_distantUpdateTime, TXT( "Time interval for updating distant map pins" ) );
	PROPERTY_EDIT( m_nearbyRadius, TXT( "Radius of area where map pins should be updated per frame" ) );
END_CLASS_RTTI();