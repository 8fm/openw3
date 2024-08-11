#include "build.h"

#include "../engine/areaComponent.h"
#include "../engine/pathlibWorld.h"

#include "aiPositioning.h"
#include "movableRepresentationPathAgent.h"

IMPLEMENT_ENGINE_CLASS( SPositioningFilter )
IMPLEMENT_ENGINE_CLASS( SClosestSpotFilter )

///////////////////////////////////////////////////////////////////////////////
// CPositioningFilterRequest
///////////////////////////////////////////////////////////////////////////////

CPositioningFilterRequest::~CPositioningFilterRequest()
{
}

Bool CPositioningFilterRequest::Setup( const SPositioningFilter& filter, CWorld* world, const Vector3& basePos, Float personalSpace,  Float baseDirection, const Vector3* originSpot, CAreaComponent* areaTest )
{
	CPathLibWorld* pathlib			= world->GetPathLibWorld();
	if ( !pathlib )
	{
		return false;
	}

	// test boundings
	Box testBox( basePos, filter.m_maxDistance );
	testBox.Min.Z = Max( testBox.Min.Z, basePos.Z - filter.m_zDiff );
	testBox.Max.Z = Min( testBox.Max.Z, basePos.Z + filter.m_zDiff );
	Bool testDirection				= ( baseDirection >= - 360.f ) && filter.m_angleLimit < 180.0;

	// compute desired spot
	Float desiredAngle				= testDirection
		? (GEngine->GetRandomNumberGenerator().Get< Float >() * 2.f - 1.f) * filter.m_angleLimit + baseDirection
		: GEngine->GetRandomNumberGenerator().Get< Float >() * 360.f;

	Float desiredDist				= filter.m_minDistance + (filter.m_maxDistance - filter.m_minDistance) * GEngine->GetRandomNumberGenerator().Get< Float >();

	Vector3 desiredPos				= basePos;
	desiredPos.AsVector2()			+= EulerAngles::YawToVector2( desiredAngle ) * desiredDist;

	m_areaShape.Clear();

	if ( areaTest )
	{
		testBox.Crop( areaTest->GetBoundingBox() );
		m_areaShape = areaTest->GetCompiledShapePtr();
	}

	Uint32 flags							= 0;
	PathLib::NodeFlags forbiddenNodeFlags	= PathLib::NFG_FORBIDDEN_BY_DEFAULT;

	if ( filter.m_onlyReachable )
	{
		flags						|= CPositioningFilterRequest::FLAG_REQUIRE_REACHABLE_FROM_SOURCE;
	}
	if ( filter.m_limitToBaseArea )
	{
		flags						|= CPositioningFilterRequest::FLAG_ONLY_BASEAREA;
	}
	if ( filter.m_noRoughTerrain )
	{
		forbiddenNodeFlags			= PathLib::NF_ROUGH_TERRAIN;
	}
	if ( filter.m_noInteriors )
	{
		forbiddenNodeFlags			= PathLib::NF_INTERIOR;
	}

	Super::Setup(* pathlib, testBox, desiredPos, originSpot ? *originSpot : basePos, personalSpace, filter.m_maxDistance*2.f, flags, PathLib::CT_DEFAULT, forbiddenNodeFlags, 1023 );
	
	m_minDistSq						= filter.m_minDistance * filter.m_minDistance;
	m_maxDistSq						= filter.m_maxDistance * filter.m_maxDistance;
	m_ringTest						= true;
	m_angleTest						= false;
	m_cameraTest					= false;

	if ( ( baseDirection >= - 360.f ) && filter.m_angleLimit < 180.0 )
	{
		m_angleTest					= true;
		m_minAngle					= EulerAngles::NormalizeAngle( baseDirection - filter.m_angleLimit );
		m_maxAngle					= EulerAngles::NormalizeAngle( baseDirection + filter.m_angleLimit );
	}
	if ( filter.m_awayFromCamera )
	{
		m_cameraTest				= true;
		CCameraDirector* cameraDir	= world->GetCameraDirector();
		m_cameraTransform			= cameraDir->GetCameraTransform();
		m_cameraFov					= cameraDir->GetFov();
		m_cameraViewportWidth		= cameraDir->GetViewportWidth();
		m_cameraViewportHeight		= cameraDir->GetViewportHeight();
	}

	return true;
}

Bool CPositioningFilterRequest::Setup( const SClosestSpotFilter& filter, CWorld* world, const Vector3& basePos, CActor* actor, const Vector3* originSpot, CAreaComponent* areaTest )
{
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	CPathLibWorld* pathlib			= world->GetPathLibWorld();
	if ( !pathlib )
	{
		return false;
	}

	CPathAgent* pathAgent = mac->GetPathAgent();

	// test boundings
	Box testBox( basePos, filter.m_maxDistance );
	testBox.Min.Z = Max( testBox.Min.Z, basePos.Z - filter.m_zDiff );
	testBox.Max.Z = Min( testBox.Max.Z, basePos.Z + filter.m_zDiff );

	m_areaShape.Clear();

	if ( areaTest )
	{
		testBox.Crop( areaTest->GetBoundingBox() );
		m_areaShape = areaTest->GetCompiledShapePtr();
		m_areaShapeTransform = areaTest->GetWorldToLocal();
	}

	Uint32 flags							= 0;
	PathLib::NodeFlags forbiddenNodeFlags	= PathLib::NFG_FORBIDDEN_BY_DEFAULT;

	if ( filter.m_onlyReachable )
	{
		flags						|= CPositioningFilterRequest::FLAG_REQUIRE_REACHABLE_FROM_SOURCE;
	}
	if ( filter.m_limitToBaseArea )
	{
		flags						|= CPositioningFilterRequest::FLAG_ONLY_BASEAREA;
	}
	if ( filter.m_noRoughTerrain )
	{
		forbiddenNodeFlags			|= PathLib::NF_ROUGH_TERRAIN;
	}
	if ( filter.m_noInteriors )
	{
		forbiddenNodeFlags			|= PathLib::NF_INTERIOR;
	}
	if ( filter.m_limitedPrecision )
	{
		flags						|= CPositioningFilterRequest::FLAG_BAIL_OUT_ON_SUCCESS;
	}

	m_ringTest = false;

	Super::Setup(* pathlib, testBox, basePos, originSpot ? *originSpot : basePos, pathAgent->GetPersonalSpace(), filter.m_maxDistance*2.f, flags, pathAgent->GetCollisionFlags(), pathAgent->GetForbiddenPathfindFlags(), 1023 );

	m_ringTest						= true;
	m_angleTest						= false;
	m_cameraTest					= false;

	if ( filter.m_awayFromCamera )
	{
		m_cameraTest				= true;
		CCameraDirector* cameraDir	= world->GetCameraDirector();
		m_cameraTransform			= cameraDir->GetCameraTransform();
		m_cameraFov					= cameraDir->GetFov();
		m_cameraViewportWidth		= cameraDir->GetViewportWidth();
		m_cameraViewportHeight		= cameraDir->GetViewportHeight();
	}
	return true;
}

Bool CPositioningFilterRequest::AcceptPosition( const Vector3& nodePos )
{
	if ( m_ringTest )
	{
		Vector3 diff = nodePos - m_sourcePos;
		Float distSq = diff.SquareMag();
		if ( distSq < m_minDistSq || distSq > m_maxDistSq )
		{
			return false;
		}
	}
	if ( m_angleTest )
	{
		Vector2 diff = nodePos.AsVector2() - m_sourcePos.AsVector2();
		Float yaw = EulerAngles::YawFromXY( diff.X, diff.Y );
		if ( m_minAngle <= m_maxAngle )
		{
			if ( yaw < m_minAngle || yaw > m_maxAngle )
			{
				return false;
			}
		}
		else
		{
			if ( yaw < m_minAngle && yaw > m_maxAngle )
			{
				return false;
			}
		}
	}

	if ( m_cameraTest )
	{
		if ( CCameraDirector::IsPointInView( nodePos, m_cameraTransform, m_cameraFov, m_cameraViewportWidth, m_cameraViewportHeight ) )
		{
			return false;
		}
	}

	if ( m_areaShape )
	{
		if ( !m_areaShape->PointOverlap( m_areaShapeTransform.TransformPoint( nodePos ) ) )
		{
			return false;
		}
	}

	return true;
}

void CPositioningFilterRequest::CompletionCallback()
{
	m_areaShape.Clear();
	Super::CompletionCallback();
}

///////////////////////////////////////////////////////////////////////////////
// CPositionFilterTeleportEntityCallback
///////////////////////////////////////////////////////////////////////////////
CPositionFilterTeleportEntityCallback::CPositionFilterTeleportEntityCallback( CEntity* entity )
	: m_entity( entity )
{

}

void CPositionFilterTeleportEntityCallback::Callback( PathLib::CWalkableSpotQueryRequest* request )
{
	if ( request->IsQuerySuccess() )
	{
		CEntity* entity = m_entity.Get();
		if ( entity )
		{
			Vector pos = request->GetComputedPosition();
			entity->Teleport( pos, entity->GetWorldRotation() );;
		}
	}
}