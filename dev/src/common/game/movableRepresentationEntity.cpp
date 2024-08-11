#include "build.h"
#include "movableRepresentationEntity.h"
#include "movableRepresentationPathAgent.h"

///////////////////////////////////////////////////////////////////////////////

CMREntity::CMREntity( CMovingAgentComponent& host )
: m_host( host )
{}

CName CMREntity::GetName() const 
{ 
	return CNAME( CMREntity ); 
}

void CMREntity::OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation )
{
	PC_SCOPE( CMREntity_OnMove );

	if ( Vector::Near3( deltaPosition, Vector::ZERO_3D_POINT, 0.0001f ) && deltaOrientation.AlmostEquals( EulerAngles::ZEROS ) )
	{
		return;
	}

	CEntity* entity = m_host.GetEntity();

	if ( !entity->GetTransformParent() )
	{
		Vector pos = m_host.GetWorldPosition() + deltaPosition;
		EulerAngles rot = m_host.GetWorldRotation() + deltaOrientation;

		//if ( m_host.IsMotionEnabled() && m_host.IsSnapToSurfaceRequested() )
		//{
		//	CPathAgent* pathAgent = m_host.GetPathAgent();
		//	if ( pathAgent )
		//	{
		//		pathAgent->ComputeHeight( pos.AsVector3(), pos.Z );
		//	}
		//}

		entity->SetRawPlacement( &pos, &rot, NULL );
	}
	else
	{
		Vector pos = entity->GetPosition() + deltaPosition;
		EulerAngles rot = entity->GetRotation() + deltaOrientation;

		//if ( m_host.IsMotionEnabled() && m_host.IsSnapToSurfaceRequested() )
		//{
		//	CPathAgent* pathAgent = m_host.GetPathAgent();
		//	if ( pathAgent )
		//	{
		//		pathAgent->ComputeHeight( pos.AsVector3(), pos.Z );
		//	}
		//}

		entity->SetRawPlacement( &pos, &rot, NULL );
	}
}

void CMREntity::OnSeparate( const Vector& deltaPosition )
{
	// No separation for entity representations - Empty by design
}

void CMREntity::OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ )
{
	PC_SCOPE( CMREntity_OnSetPlacement );

	RED_UNUSED( correctZ );
	Vector meshPos = newPosition;
	CEntity* entity = m_host.GetEntity();

	if ( Vector::Near3( entity->GetWorldPositionRef(), newPosition, 0.001f ) && newOrientation.AlmostEquals( entity->GetRotation() ) )
	{
		return;
	}

	entity->SetRawPlacement( &meshPos, &newOrientation, NULL );
}

Vector CMREntity::GetRepresentationPosition( Bool smooth /*= false */) const
{
	return m_host.GetWorldPosition();
}

EulerAngles CMREntity::GetRepresentationOrientation() const
{
	return m_host.GetWorldRotation();
}

///////////////////////////////////////////////////////////////////////////////
