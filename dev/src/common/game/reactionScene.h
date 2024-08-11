/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "reactionSceneActor.h"

class CReactionSceneActorComponent;
class CBehTreeReactionEventData;

enum EReactionSceneExecutionPhase
{
	ERSEP_Assigning,
	ERSEP_Playing
};

class CReactionScene : public CObject
{
	DECLARE_ENGINE_CLASS( CReactionScene, CObject, 0 );
public:
	typedef Char SceneBranches;

private:
	THandle< CBehTreeReactionEventData >			m_sourceEvent;
	CReactionSceneActorComponent*					m_invoker;
	THashMap< CName, THandle< CReactionSceneActorComponent > >	m_actrosRoles;	
	TDynArray< CName >								m_allRoles;	

	Bool											m_allActorsCollected;
	Bool											m_rolesInitialized;
	Bool											m_blockingInitialized;
	Bool											m_actionTargetInitialized;
	Bool											m_sceneInterupted;
	EReactionSceneExecutionPhase					m_sceneExecutionPhase;

	Char											m_finishedRoles;			// flags
	SceneBranches									m_blockingBranches;			// flags
	SceneBranches									m_executedBlockingBranches;	// flags

private:
	void CahceIfAllActrorsCollected();

public:

	CReactionScene() 
		: m_allActorsCollected		( false )
		, m_rolesInitialized		( false )
		, m_blockingInitialized		( false )
		, m_actionTargetInitialized	( false )
		, m_sceneInterupted			( false )
		, m_sceneExecutionPhase		( ERSEP_Assigning )
		, m_finishedRoles			( 0		)
		, m_blockingBranches		( 0		)
		, m_executedBlockingBranches( 0		)
	{}

	RED_INLINE void SetInvoker( CReactionSceneActorComponent* invoker ){ m_invoker = invoker; }
	RED_INLINE CReactionSceneActorComponent* GetInvoker(){ return m_invoker; }
	RED_INLINE Bool IfAllActrorsCollected(){ return m_allActorsCollected; }
	RED_INLINE void ResetFinishedRoles(){ m_finishedRoles = 0; }
	RED_INLINE Bool IfBlockingBrancesInitialized(){ return m_blockingInitialized; }
	RED_INLINE EReactionSceneExecutionPhase GetScenePhase(){ return m_sceneExecutionPhase; }
	RED_INLINE void SetScenePhase( EReactionSceneExecutionPhase phase ){ m_sceneExecutionPhase = phase; }
	RED_INLINE THashMap< CName, THandle< CReactionSceneActorComponent > >* GetActors(){ return &m_actrosRoles; };
	RED_INLINE Bool IsSceneInterupted(){ return m_sceneInterupted; }

	void SetSourceEvent( CBehTreeReactionEventData* sourceEvent );

	void InitializeRoles( TDynArray< CName >& roles );
	void InitializeBlockingBranches( SceneBranches branches );
	void InitializeActionTargets();
	
	Bool IfNeedsActorToRole( CName roleName );
	void AssignActorToRole( CReactionSceneActorComponent* actor, CName roleName );
	void LeaveScene( CReactionSceneActorComponent* actor );	
	void MarkAsFinished( CName roleName, Bool interupted );
	Bool IfAllRolesFinished();	
	void EndScene();
	void Clear();
	Bool CanBeActivated( Int32 branch );
	void MarkBlockingBranchAsExecuted(  Int32 branch  );
	void AssigntToFirstFreeRole( CReactionSceneActorComponent* actor );
};

BEGIN_CLASS_RTTI( CReactionScene );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();