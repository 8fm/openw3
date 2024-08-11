/**
* Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "reactionsManager.h"

#include "interestPointComponent.h"
#include "potentialField.h"
#include "binaryStorage.h"
#include "actorsManager.h"
#include "gameWorld.h"

#define INTEREST_POINTS_QUERY_COOLDOWN		2.0

IMPLEMENT_ENGINE_CLASS( CReactionsManager );

CReactionsManager::CReactionsManager()
	: m_interestPointsQueryCooldownTimer( 0 )
	, m_impactBBoxSize( 100, 100, 3.0f )
{
}

CReactionsManager::~CReactionsManager()
{
}

void CReactionsManager::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	file << m_instances << m_mappedInstances;
}

void CReactionsManager::Tick( Float timeElapsed )
{
	PC_SCOPE( AI_Reactions_ManagerTick );

	// gather static interest points
	m_interestPointsQueryCooldownTimer -= timeElapsed;
	if ( m_interestPointsQueryCooldownTimer < 0 )
	{
		m_interestPointsQueryCooldownTimer = INTEREST_POINTS_QUERY_COOLDOWN;
		QueryStaticInterestPoints();
	}

	UpdateInterestPoints( timeElapsed );
	Broadcast( timeElapsed );
}

void CReactionsManager::UpdateInterestPoints( Float timeElapsed )
{
	for ( Int32 i = Int32( m_instances.Size() ) - 1; i >= 0; --i )
	{
		CInterestPointInstance* ip = m_instances[ i ];
		if ( ip->Update( timeElapsed ) || ip->GetParentPoint() == NULL )
		{
			// the interest point has expired - remove it
			m_instances[ i ]->Discard();
			m_instances.EraseFast( m_instances.Begin() + i );
		}
	}

	for( Int32 i = Int32( m_mappedInstances.Size() ) - 1; i >= 0; --i )
	{
		CInterestPointInstance* ip = m_mappedInstances[i].m_instance;
		ASSERT( ip );
		if( ip->Update( timeElapsed ) || ip->GetParentPoint() == NULL )
		{
			ip->Discard();
			m_mappedInstances.EraseFast( m_mappedInstances.Begin() + i );
		}
	}
}

void CReactionsManager::QueryStaticInterestPoints()
{
	if ( !GCommonGame || !GCommonGame->GetActiveWorld() )
	{
		return;
	}

	if ( !GCommonGame->GetPlayer() )
	{
		return;
	}

	// gather the interest points from around the level
	Vector playerPos = GCommonGame->GetPlayer()->GetWorldPosition();
	Box bb( playerPos - m_impactBBoxSize, playerPos + m_impactBBoxSize );

	TDynArray< CInterestPointComponent* > comps;
	GCommonGame->GetActiveWorld()->FindInterestPoint( bb, comps );

	for ( TDynArray< CInterestPointComponent* >::iterator it = comps.Begin(); it != comps.End(); ++it )
	{
		CInterestPointComponent* comp = *it;
		if ( comp && comp->IsActive() )
		{
			BroadcastInterestPoint( comp->GetInterestPoint(), comp->GetWorldPosition(), 0.0f );
		}
	}
}

void CReactionsManager::BroadcastInterestPoint( CInterestPoint* interestPoint, const Vector& pos, Float duration )
{
	ASSERT( interestPoint );
	PC_SCOPE( AI_Reactions_Broadcast );

	for( Uint32 i = 0; i < m_instances.Size(); i++ )
	{
		CInterestPointInstance* instance = m_instances[i];
		if( instance && instance->GetParentPoint() == interestPoint && instance->GetPosition() == pos )
		{
			instance->Rebind( pos, duration );
			return;
		}
	}

	// not found
	CInterestPointInstance* instance = interestPoint->CreateInstance( this, pos, duration );
	BroadcastInterestPoint( instance );
}

void CReactionsManager::BroadcastInterestPoint( CInterestPoint* interestPoint, const THandle< CNode >& node, Float duration )
{
	ASSERT( interestPoint );
	PC_SCOPE( AI_Reactions_Broadcast );

	for( Uint32 i = 0; i < m_instances.Size(); i++ )
	{
		CInterestPointInstance* instance = m_instances[i];
		if( instance && instance->GetParentPoint() == interestPoint && instance->GetNode() == node )
		{
			instance->Rebind( node, duration );
			return;
		}
	}

	// not found
	CInterestPointInstance* instance = interestPoint->CreateInstance( this, node, duration );
	BroadcastInterestPoint( instance );
}

void CReactionsManager::BroadcastInterestPoint( CInterestPointInstance* interestPoint )
{
	m_instances.PushBack(interestPoint);
}

void CReactionsManager::SendInterestPoint( CNewNPC* npc, CInterestPoint* interestPoint, const Vector& pos, Float duration )
{
	ASSERT( npc );
	ASSERT( interestPoint );

	PC_SCOPE( AI_Reactions_Broadcast );

	// If no reaction do not create instance
	if( !npc->FindReaction( interestPoint->GetFieldName() ) )
	{
		return;
	}

	for( Uint32 i = 0; i < m_mappedInstances.Size(); i++ )
	{
		if( m_mappedInstances[i].m_npc == npc )
		{
			CInterestPointInstance* instance = m_mappedInstances[i].m_instance;
			if( instance && instance->GetParentPoint() == interestPoint && instance->GetPosition() == pos )
			{
				instance->Rebind( pos, duration );
				return;
			}
		}
	}

	// not found
	CInterestPointInstance* instance = interestPoint->CreateInstance( this, pos, duration );
	instance->SetParent( this );
	SendInterestPoint( npc, instance );
}

void CReactionsManager::SendInterestPoint( CNewNPC* npc, CInterestPoint* interestPoint, const THandle< CNode >& node, Float duration )
{
	ASSERT( npc );
	ASSERT( interestPoint );

	PC_SCOPE( AI_Reactions_Broadcast );

	// If no reaction do not create instance
	if( !npc->FindReaction( interestPoint->GetFieldName() ) )
	{
		return;
	}

	for( Uint32 i = 0; i < m_mappedInstances.Size(); i++ )
	{
		if( m_mappedInstances[i].m_npc == npc )
		{
			CInterestPointInstance* instance = m_mappedInstances[i].m_instance;
			if( instance && instance->GetParentPoint() == interestPoint && instance->GetNode() == node )
			{
				instance->Rebind( node, duration );
				return;
			}
		}
	}

	// not found
	CInterestPointInstance* instance = interestPoint->CreateInstance( this, node, duration );
	instance->SetParent( this );
	SendInterestPoint( npc, instance );
}

void CReactionsManager::SendInterestPoint( CNewNPC* npc, CInterestPointInstance* interestPoint )
{
	Uint32 oldSize = static_cast< Uint32 >( m_mappedInstances.Grow(1) );
	m_mappedInstances[oldSize].m_npc = npc;
	m_mappedInstances[oldSize].m_instance = interestPoint;
}

void CReactionsManager::Broadcast( Float timeElapsed )
{
	CPlayer* player = GCommonGame->GetPlayer();
	if ( player )
	{
		CActorsManager* actorsStorage = GCommonGame->GetActorsManager();
		Vector playerPos = player->GetWorldPosition();

		m_instancesCopy = m_instances;

		struct Functor : public Red::System::NonCopyable
		{
			enum { SORT_OUTPUT = false };

			Functor( const TDynArray< CInterestPointInstance* >& instances )
				: m_instances( instances ) {}
			Bool operator()( const CActorsManagerMemberData& memberData ) const
			{
				CNewNPC* npc = Cast< CNewNPC >( memberData.Get() );
				if ( npc )
				{
					Vector npcPos = npc->GetWorldPosition();
					for ( TDynArray< CInterestPointInstance* >::const_iterator ipIt = m_instances.Begin();
						ipIt != m_instances.End(); ++ipIt )
					{
						CInterestPointInstance* ipInstance = *ipIt;
						if ( ipInstance )
						{
							if( ipInstance->RangeTest( npcPos ) )
							{
								npc->OnInterestPoint( ipInstance );
							}
						}
					}
				}
				return true;
			};

			const TDynArray< CInterestPointInstance* >& m_instances;
		} functor( m_instancesCopy );
		actorsStorage->TQuery( playerPos, functor, Box( -m_impactBBoxSize * 0.5f, m_impactBBoxSize * 0.5f ), true, NULL, 0 );
		m_instancesCopy.ClearFast();
	}

	m_mappedInstancesCopy = m_mappedInstances;
	for( TDynArray< MappedInstance >::iterator iter = m_mappedInstancesCopy.Begin(); iter != m_mappedInstancesCopy.End(); ++iter )
	{
		iter->m_npc->OnInterestPoint( iter->m_instance );	
	}

	m_mappedInstancesCopy.ClearFast();
}

void CReactionsManager::OnNPCDetached( CNewNPC* npc )
{
	for( Int32 i = Int32( m_mappedInstances.Size() ) - 1; i >= 0; --i )
	{
		CInterestPointInstance* ip = m_mappedInstances[i].m_instance;
		if( m_mappedInstances[i].m_npc == npc )
		{
			ip->Discard();
			m_mappedInstances.EraseFast( m_mappedInstances.Begin() + i );
		}
	}
}

// -------------------------------------------------------------------------
// Scripting support
// -------------------------------------------------------------------------
void CReactionsManager::funcBroadcastStaticInterestPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CInterestPoint >, interestPoint, NULL );
	GET_PARAMETER( Vector, position, Vector() );
	GET_PARAMETER_OPT( Float, timeout, 0.0f );
	FINISH_PARAMETERS;

	CInterestPoint *pInterestPoint = interestPoint.Get();
	if ( pInterestPoint == NULL )
	{
		return;
	}

	BroadcastInterestPoint( pInterestPoint, position, timeout );
};

void CReactionsManager::funcBroadcastDynamicInterestPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CInterestPoint >, interestPoint, NULL );
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Float, timeout, 0.0f );
	FINISH_PARAMETERS;

	CInterestPoint *pInterestPoint = interestPoint.Get();
	CNode *pNode = node.Get();

	if ( pInterestPoint == NULL || pNode == NULL )
	{
		return;
	}

	BroadcastInterestPoint( pInterestPoint, node, timeout );
};

void CReactionsManager::funcSendStaticInterestPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNewNPC >, npcHanlde, NULL );
	GET_PARAMETER( THandle< CInterestPoint >, interestPointHandle, NULL );
	GET_PARAMETER( Vector, position, Vector() );
	GET_PARAMETER_OPT( Float, timeout, 0.0f );
	FINISH_PARAMETERS;
	
	CNewNPC* npc = npcHanlde.Get();
	CInterestPoint* interestPoint = interestPointHandle.Get();

	if ( npc == NULL || interestPoint == NULL )
	{
		return;
	}

	SendInterestPoint( npc, interestPoint, position, timeout );
}

void CReactionsManager::funcSendDynamicInterestPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNewNPC >, npcHanlde, NULL );
	GET_PARAMETER( THandle< CInterestPoint >, interestPointHandle, NULL );
	GET_PARAMETER( THandle< CNode >, nodeHandle, NULL );
	GET_PARAMETER_OPT( Float, timeout, 0.0f );
	FINISH_PARAMETERS;

	CNewNPC* npc = npcHanlde.Get();
	CInterestPoint* interestPoint = interestPointHandle.Get();
	CNode* node = nodeHandle.Get();

	if ( npc == NULL || interestPoint == NULL || node == NULL )
	{
		return;
	}

	SendInterestPoint( npc, interestPoint, nodeHandle, timeout );
}
