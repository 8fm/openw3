#pragma once

struct SEntityMapPinInfo
{
	DECLARE_RTTI_STRUCT( SEntityMapPinInfo );

	CName		m_entityName;
	Uint32		m_entityCustomNameId;
	TagList		m_entityTags;
	Vector		m_entityPosition;
	float		m_entityRadius;
	CName		m_entityType;
	Int32		m_alternateVersion;
	CName		m_fastTravelSpotName;
	CName		m_fastTravelGroupName;
	CName		m_fastTravelTeleportWayPointTag;
	Vector		m_fastTravelTeleportWayPointPosition;
	EulerAngles	m_fastTravelTeleportWayPointRotation;

	SEntityMapPinInfo()
		: m_entityCustomNameId( 0 )
		, m_entityPosition( Vector::ZEROS )
		, m_entityRadius( 0 )
		, m_alternateVersion( 0 )
		, m_fastTravelTeleportWayPointPosition( Vector::ZEROS )
		, m_fastTravelTeleportWayPointRotation( EulerAngles::ZEROS )
	{}
};

BEGIN_CLASS_RTTI( SEntityMapPinInfo );
	PROPERTY_EDIT( m_entityName, TXT( "Name" ) );
	PROPERTY_EDIT( m_entityCustomNameId, TXT( "Custom Name Id" ) );
	PROPERTY_EDIT( m_entityTags, TXT( "Tags" ) );
	PROPERTY_EDIT( m_entityPosition, TXT( "Position" ) );
	PROPERTY_EDIT( m_entityRadius, TXT( "Radius" ) );
	PROPERTY_EDIT( m_entityType, TXT( "Type" ) );
	PROPERTY_EDIT( m_alternateVersion, TXT( "Alternate version" ) );
	PROPERTY_EDIT( m_fastTravelSpotName, TXT( "Fast travel spot name" ) );
	PROPERTY_EDIT( m_fastTravelGroupName, TXT( "Fast travel group name" ) );
	PROPERTY_EDIT( m_fastTravelTeleportWayPointTag, TXT( "Fast travel teleport waypoint tag" ) );
	PROPERTY_EDIT( m_fastTravelTeleportWayPointPosition, TXT( "Fast travel teleport waypoint position" ) );
	PROPERTY_EDIT( m_fastTravelTeleportWayPointRotation, TXT( "Fast travel teleport waypoint rotation" ) );
END_CLASS_RTTI();

class CEntityMapPinsResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CEntityMapPinsResource, CResource, "w2em", "Entity Mappins" );

public:
	CEntityMapPinsResource();
	~CEntityMapPinsResource();

#ifndef NO_EDITOR
	Bool AddEntry( const SEntityMapPinInfo& info );
#endif //NO_EDITOR

	RED_INLINE const TDynArray< SEntityMapPinInfo >& GetMapPinsInfo() const	{ return m_mappinsInfo; } 
	RED_INLINE TDynArray< SEntityMapPinInfo >& GetMapPinsInfo()				{ return m_mappinsInfo; } 
	RED_INLINE void ClearMapPinsInfo()										{ m_mappinsInfo.Clear(); } 

private:
    TDynArray< SEntityMapPinInfo > m_mappinsInfo;
};

BEGIN_CLASS_RTTI( CEntityMapPinsResource )
	PARENT_CLASS( CResource )
	PROPERTY_RO( m_mappinsInfo, TXT("Mappins info") )
END_CLASS_RTTI()