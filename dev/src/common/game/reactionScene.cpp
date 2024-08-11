#include "build.h"

#include "reactionSceneActor.h"
#include "reactionScene.h"
#include "behTreeReactionData.h"
#include "behTreeInstance.h"

IMPLEMENT_ENGINE_CLASS( CReactionScene  );


void CReactionScene::SetSourceEvent( CBehTreeReactionEventData* sourceEvent )
{ 
	m_sourceEvent = sourceEvent; 
}

void CReactionScene::InitializeRoles( TDynArray< CName >& roles )
{
	if( m_rolesInitialized )
		return;
	m_rolesInitialized = true;

	m_allRoles.Reserve( roles.Size() );

	for( Uint32 i=0; i< roles.Size(); ++i )
	{
		m_allRoles.PushBack( roles[ i ] );
	}

	m_allActorsCollected = false;
}

void CReactionScene::InitializeBlockingBranches( SceneBranches branches )
{
	if( !m_blockingInitialized )
	{
		m_blockingInitialized		= true;
		m_blockingBranches			= branches;
		m_executedBlockingBranches	= 0;
	}
}

Bool CReactionScene::IfNeedsActorToRole( CName roleName )
{
	Bool exists = m_actrosRoles.KeyExist( roleName );
	
	return !exists && m_allRoles.Exist( roleName );
}

void CReactionScene::AssignActorToRole( CReactionSceneActorComponent* actor, CName roleName )
{
	if( !IfNeedsActorToRole( roleName) || actor->GetRole() )
	{
		return;
	}

	m_actrosRoles.Insert( roleName, actor );
	actor->SetRole( roleName );
	CahceIfAllActrorsCollected();
}

void CReactionScene::AssigntToFirstFreeRole( CReactionSceneActorComponent* actor )
{
	CName freeRole;

	for( Uint32 i=0; i<m_allRoles.Size(); ++i )
	{
		if( !m_actrosRoles.KeyExist( m_allRoles[ i ] ) )
		{
			freeRole =  m_allRoles[ i ];
			break;
		}
	}
	if( freeRole )
	{
		AssignActorToRole( actor, freeRole );
	}
}

void CReactionScene::LeaveScene( CReactionSceneActorComponent* actor )
{
	CName role = actor->GetRole();

	m_actionTargetInitialized = false;
	m_actrosRoles.Erase( role );
	actor->SetRole( CName::NONE );
	CahceIfAllActrorsCollected();
}

void CReactionScene::CahceIfAllActrorsCollected()
{
	for( Uint32 i=0; i<m_allRoles.Size(); ++i )
	{
		if( !m_actrosRoles.KeyExist( m_allRoles[ i ] ) )
		{
			m_allActorsCollected = false;
			return;
		}
	}

	m_allActorsCollected = true;	
}

void CReactionScene::MarkAsFinished( CName roleName, Bool interupted )
{
	m_sceneInterupted = interupted;
	Int32 roleIndex = static_cast< Int32 >( m_allRoles.GetIndex( roleName ) );
	if( roleIndex < 0 )
	{
		return;
	}	

	m_finishedRoles |= 1 << roleIndex;
}

Bool CReactionScene::IfAllRolesFinished()
{
	for( Uint32 i=0; i<m_allRoles.Size(); ++i )
	{
		if( !( m_finishedRoles & ( 1 << i ) ) )
		{
			return false;
		}
	}
	return true;
}

void CReactionScene::EndScene()
{
	CBehTreeReactionEventData* source = m_sourceEvent.Get();

	if( source )
	{
		source->ForceExpiration();
	}
}

void CReactionScene::Clear()
{
	for( auto it = m_actrosRoles.Begin(); it != m_actrosRoles.End(); ++it )
	{
		CReactionSceneActorComponent* actor = (*it).m_second.Get();

		if( actor )
		{
			actor->SetRole( CName::NONE );
			actor->SetReactionScene( nullptr );
		}		
	}

	m_actrosRoles.Clear();
}

Bool CReactionScene::CanBeActivated( Int32 branch )
{
	for( Int32 i=0; i<branch; ++i )
	{
		if( 
			( m_blockingBranches & ( 1 << i ) ) //if i is blocking branch
			&& ( ( m_executedBlockingBranches & ( 1 << i ) ) == 0 ) //if it was not yet executed
			)
		{
			return false;
		}
	}
	return true;
}

void CReactionScene::MarkBlockingBranchAsExecuted(  Int32 branch  )
{
	if( m_blockingBranches & ( 1 << branch ) )
	{
		m_executedBlockingBranches |= 1 << branch;
	}
}

void CReactionScene::InitializeActionTargets()
{		
	auto cIt = m_actrosRoles.Begin();
	auto nIt = m_actrosRoles.Begin();
	++nIt;

	for( ; cIt != m_actrosRoles.End(); ++cIt, ++nIt )
	{
		if( nIt == m_actrosRoles.End() )
		{
			nIt = m_actrosRoles.Begin();
		}
		CReactionSceneActorComponent* cActorCmp = (*cIt).m_second.Get();
		CReactionSceneActorComponent* nActorCmp = (*nIt).m_second.Get();

		if( !nActorCmp || !cActorCmp )
		{
			continue;
		}

		CNewNPC* cNPC = Cast< CNewNPC >( cActorCmp->GetEntity() );
		
		if( cNPC )
		{
			CBehTreeInstance* cBTI = cNPC->GetBehTreeMachine()->GetBehTreeInstance();			
			cBTI->SetActionTarget( nActorCmp->GetEntity() );
		}
	}
		

	m_actionTargetInitialized = true;
}