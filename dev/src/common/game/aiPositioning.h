#pragma once

#include "../engine/areaConvex.h"
#include "../engine/pathlibWalkableSpotQueryRequest.h"

// General and powerful walkable spot query request object.
class CPositioningFilterRequest : public PathLib::CWalkableSpotQueryRequest
{
	typedef PathLib::CWalkableSpotQueryRequest Super;
protected:
	Float							m_minDistSq;
	Float							m_maxDistSq;
	Float							m_minAngle;
	Float							m_maxAngle;
	Bool							m_ringTest;
	Bool							m_angleTest;
	Bool							m_cameraTest;
	Matrix							m_cameraTransform;
	Float							m_cameraFov;
	Uint32							m_cameraViewportWidth;
	Uint32							m_cameraViewportHeight;
	CAreaShapePtr					m_areaShape;
	Matrix							m_areaShapeTransform;
public:
	typedef TRefCountPointer< CPositioningFilterRequest > Ptr;

	CPositioningFilterRequest()																	{}
	~CPositioningFilterRequest();
	
	// find in ring
	Bool		Setup( const SPositioningFilter& filter, CWorld* world, const Vector3& basePos, Float personalSpace, Float baseDirection = -1024.f, const Vector3* originSpot = nullptr, CAreaComponent* areaTest = nullptr );
	// find closest to spot
	Bool		Setup( const SClosestSpotFilter& filter, CWorld* world, const Vector3& basePos, CActor* actor, const Vector3* originSpot, CAreaComponent* areaTest = nullptr );

	Bool		AcceptPosition( const Vector3& nodePos ) override;

	void		CompletionCallback() override;
};

// Data for customizing find-closest-to-spot queries
struct SClosestSpotFilter
{
	DECLARE_RTTI_STRUCT( SClosestSpotFilter );

public:
	Float							m_maxDistance;
	Float							m_zDiff;
	Bool							m_awayFromCamera;
	Bool							m_onlyReachable;
	Bool							m_noRoughTerrain;
	Bool							m_noInteriors;
	Bool							m_limitToBaseArea;
	Bool							m_limitedPrecision;

	SClosestSpotFilter()
		: m_maxDistance( 25.f )
		, m_zDiff( 5.f )
		, m_awayFromCamera( false )
		, m_onlyReachable( true )
		, m_noRoughTerrain( true )
		, m_noInteriors( false )
		, m_limitToBaseArea( false )
		, m_limitedPrecision( false )													{}
};

BEGIN_CLASS_RTTI( SClosestSpotFilter )
	PROPERTY_EDIT( m_maxDistance, TXT("Maximum distance from base position") )
	PROPERTY_EDIT( m_zDiff, TXT("Maximum z-distance") )
	PROPERTY_EDIT( m_awayFromCamera, TXT("Pick position only away from camera") )
	PROPERTY_EDIT( m_onlyReachable, TXT("Pick position reachable from base spot") )
	PROPERTY_EDIT( m_noRoughTerrain, TXT("Pick away from rought terrain") )
	PROPERTY_EDIT( m_noInteriors, TXT("Pick away from interiors") )
	PROPERTY_EDIT( m_limitToBaseArea, TXT("") )
	PROPERTY_EDIT( m_limitedPrecision, TXT("Less costly algorithm version, that sacrifice preciseness in terms of finding the closest node") )
END_CLASS_RTTI()


// Data to customize find-in-ring walkable spot queries.
// NOTICY: Legacy class naming. This class should be someting like RandomSpotInRingFilter or whatever. It was created way ago,
// and many objects already contain data, so we shall stick with it.
struct SPositioningFilter
{
	DECLARE_RTTI_STRUCT( SPositioningFilter );
public:
	Float							m_minDistance;
	Float							m_maxDistance;
	Float							m_zDiff;
	Float							m_angleLimit;
	Float							m_personalSpace;
	Bool							m_awayFromCamera;
	Bool							m_onlyReachable;
	Bool							m_noRoughTerrain;
	Bool							m_noInteriors;
	Bool							m_limitToBaseArea;
public:
	SPositioningFilter()
		: m_minDistance( 10.f )
		, m_maxDistance( 30.f )
		, m_zDiff( 5.f )
		, m_angleLimit( 180.f )
		, m_personalSpace( 1.f )
		, m_awayFromCamera( false )
		, m_onlyReachable( true )
		, m_noRoughTerrain( true )
		, m_noInteriors( false )
		, m_limitToBaseArea( false )																		{}
};

BEGIN_CLASS_RTTI( SPositioningFilter )
	PROPERTY_EDIT( m_minDistance, TXT("Minimum distance from base position") )
	PROPERTY_EDIT( m_maxDistance, TXT("Maximum distance from base position") )
	PROPERTY_EDIT( m_zDiff, TXT("Maximum z-distance") )
	PROPERTY_EDIT_RANGE( m_angleLimit, TXT("Maximum angle distance from base angle (if applicable)"), 0.f, 180.f )
	PROPERTY_EDIT( m_personalSpace, TXT("Agent radius") )
	//PROPERTY_EDIT( m_checkCreatureCollision, TXT("Check position agains other creatures") )
	PROPERTY_EDIT( m_awayFromCamera, TXT("Pick position only away from camera") )
	PROPERTY_EDIT( m_onlyReachable, TXT("Pick position reachable from base spot") )
	PROPERTY_EDIT( m_noRoughTerrain, TXT("Pick away from rought terrain") )
	PROPERTY_EDIT( m_noInteriors, TXT("Pick away from interiors") )
	PROPERTY_EDIT( m_limitToBaseArea, TXT("") )
END_CLASS_RTTI()

// general callback that just teleports entity
class CPositionFilterTeleportEntityCallback : public PathLib::IWalkableSpotQueryCallback
{
protected:
	THandle< CEntity >				m_entity;

	void		Callback( PathLib::CWalkableSpotQueryRequest* request ) override;
public:
	CPositionFilterTeleportEntityCallback( CEntity* entity );
};
