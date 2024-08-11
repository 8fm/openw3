#include "build.h"
#include "teleportHelper.h"
#include "../engine/tagManager.h"

namespace
{
	void FixRotationForTeleport( EulerAngles& rotation )
	{
		rotation.Roll = 0.f;
		rotation.Pitch = 0.f;
	}
}

CTeleportHelper::CTeleportHelper( const STeleportInfo* info )
	: m_info( info )
{}

Bool CTeleportHelper::GetPlayerStartingPoint( Vector& position, EulerAngles& rotation )
{
	switch ( m_info->m_targetType )
	{
	case STeleportInfo::TargetType_Custom:
		{
			if ( m_info->m_targetTag.Empty() )
			{
				return false;
			}
			const Bool ret = GCommonGame->GetCustomPlayerStartingPoint( m_info->m_targetTag.GetTag( 0 ), position, rotation );
			FixRotationForTeleport( rotation );
			return ret;
		}

	case STeleportInfo::TargetType_Node:
		if ( CNode* node = GetTeleportLocation() )
		{
			position = node->GetWorldPosition();
			rotation = node->GetWorldRotation();
			FixRotationForTeleport( rotation );
			return true;
		}
		break;

	case STeleportInfo::TargetType_PositionAndRotation:
		{
			position = m_info->m_targetPosition;
			rotation = m_info->m_targetRotation;
			FixRotationForTeleport( rotation );
			return true;
		}
	}
	return false;
}

Bool CTeleportHelper::TeleportAllActors()
{
	if ( m_info->m_targetTag.Empty() || !GGame->GetActiveWorld() )
	{
		return true;
	}

	CNode* teleportDest = GetTeleportLocation();
	if ( !teleportDest )
	{
		return false;
	}

	// Get actors to teleport

	TDynArray< CActor* > actors;
	ExtractActorsForTeleport( actors );
	if ( actors.Empty() )
	{
		return false;
	}

	// Teleport first actor to target point
	
	TeleportToDestination( teleportDest, m_info->m_offset, actors[ 0 ] );
	actors.Remove( actors[ 0 ] );

	// Teleport remaining actors around target

	if ( !actors.Empty() )
	{
		TeleportAroundDestination( teleportDest, actors );
	}
	return true;
}

CNode* CTeleportHelper::GetTeleportLocation() const
{
	TDynArray< CNode* > nodes;
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( m_info->m_targetTag, nodes, BCTO_MatchAll );
	return nodes.Empty() ? nullptr : nodes[ 0 ];
}

void CTeleportHelper::TeleportToDestination( CNode* teleportDest, const Vector& offset, CActor* actor ) const
{
	ASSERT( teleportDest );

	if ( actor )
	{
		actor->SignalGameplayEvent( CNAME( AI_ForceInterruption ) );
		actor->Teleport( teleportDest, true, offset );
	}
}

void CTeleportHelper::TeleportAroundDestination( CNode* teleportDest, const TDynArray< CActor* >& actors ) const
{
	ASSERT( teleportDest );

	// teleport the actors behind the position
	Uint32 count = actors.Size();
	if ( count == 0 )
	{
		return;
	}

	Vector pos = teleportDest->GetWorldPosition();
	EulerAngles rotation( 0.f, 0.f, teleportDest->GetWorldYaw() );

	Float angleDiff = 180.0f / (Float)count;
	Float initialAngle = 90.0f / (Float)count - 90;

	Vector newPos;
	EulerAngles newRot(0, 0, 0);
	newRot.Yaw = rotation.Yaw + initialAngle - angleDiff;

	for ( Uint32 i = 0; i < count; ++i )
	{
		CActor* actor = actors[ i ];

		newRot += angleDiff;
		newPos = pos - newRot.TransformVector( Vector( 0, -m_info->m_distanceFromTarget, 0 ) );

		FixRotationForTeleport( rotation );

		actor->Teleport( newPos, rotation );
	}
}

void CTeleportHelper::ExtractActorsForTeleport( TDynArray< CActor* >& outActors ) const
{
	TDynArray< CNode* > nodes;
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( m_info->m_actorsTags, nodes );

	Uint32 count = nodes.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CActor* actor = Cast< CActor >( nodes[i] );
		if ( actor )
		{
			outActors.PushBack( actor );
		}
	}
}