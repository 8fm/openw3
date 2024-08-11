/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraph.h"
#include "../../common/game/questScopeBlock.h"

class CBehaviorGraph;

class SBehaviorGroup : public CObject
{
	DECLARE_ENGINE_CLASS( SBehaviorGroup, CObject, 0 )

public:
	TSoftHandle< CBehaviorGraph >		m_behavior;			//! Behavior we want to set
	TagList								m_affectedNPCs;		//! Tags of the NPCs affected by the operation
	Uint32								m_expectedCount;	//! Number of entities we want to control

	SBehaviorGroup() : m_behavior( NULL ), m_expectedCount( 0 ) {}

	void Copy( const SBehaviorGroup& rhs ) 
	{
		m_behavior = rhs.m_behavior;
		m_affectedNPCs = rhs.m_affectedNPCs;
	}
};
BEGIN_CLASS_RTTI( SBehaviorGroup )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_behavior, TXT( "Behavior we want to set" ) )
	PROPERTY_CUSTOM_EDIT( m_affectedNPCs, TXT( "Tags of the NPCs affected by the operation" ), TXT( "TagListEditor" ) )
	PROPERTY_EDIT( m_expectedCount, TXT( "Number of entities we want to control( or 0 if we don't care )" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

// Quest behavior scene save mode
enum EQuestBehaviorSceneSaveMode
{
	QBDSM_SaveBlocker,			//!< Game will not be saved if this scene is played
	QBDSM_Restart,				//!< Restart scene after loading
};

BEGIN_ENUM_RTTI( EQuestBehaviorSceneSaveMode );
	ENUM_OPTION( QBDSM_SaveBlocker );
	ENUM_OPTION( QBDSM_Restart );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

// This scoped block allows to control the animation behaviors of a group of NPCs
class CQuestBehaviorCtrlBlock : public CQuestScopeBlock
{
	DECLARE_ENGINE_CLASS( CQuestBehaviorCtrlBlock, CQuestScopeBlock, 0 )

private:
	typedef TDynArray< TSoftHandle< CBehaviorGraph >, MC_Gameplay >			BehaviorsToLoadArr;

protected:
	// runtime data
	TInstanceVar< TGenericPtr >												i_activatedNPCs;
	TInstanceVar< Float >													i_activationStartTime;
	TInstanceVar< Int32 >													i_saveLock;

	// block data
	TDynArray< SBehaviorGroup* >											m_groups;
	Float																	m_activationTimeout;
	EQuestBehaviorSceneSaveMode												m_saveMode;

	// temporary data copy for the edition undo purposes
	TDynArray< SBehaviorGroup* >											m_undoGroups;

public:
	CQuestBehaviorCtrlBlock();

	//! Returns a behavior to which the specified tag is assigned
	TSoftHandle< CBehaviorGraph > GetBehaviorFor( const CName& tag ) const;

	//! Returns all currently specified behaviors
	void GetBehaviorGraphs( TDynArray< TSoftHandle< CBehaviorGraph > >& graphs ) const;

	//! Returns all currently specified tags but only from groups that 
	//! have a valid behavior assigned
	void GetAllTags( TagList& tags ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual void OnPropertyPreChange( IProperty* property );
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetClientColor() const { return Color( 163, 124, 163 ); }
	virtual Bool CanConvertToResource() { return false; }
	virtual String GetBlockCategory() const { return TXT( "Behavior control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE

#endif
	
	// Behavior scene is blocker by itself so we don't need clutter from internal blockers
	virtual Bool CanSpawnedThreadBlockSaves() const { return false; }

	// Never save internals of behavior scene
	virtual Bool ShouldSaveThread() const { return false; }

	// Restart the block only
	virtual Bool CanActivateInputsOnLoad( CQuestGraphBlock::EState activationState ) const { return m_saveMode == QBDSM_Restart; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

private:
	void LoadBehaviors( InstanceBuffer& data ) const;
	void UnloadBehaviors( InstanceBuffer& data ) const;
	Bool AreBehaviorsLoaded( InstanceBuffer& data ) const;

	Bool WaitForNPCsToAppear( InstanceBuffer& data ) const;
	Bool ActivateNPCs( InstanceBuffer& data ) const;
	void DeactivateNPCs( InstanceBuffer& data ) const;

};

BEGIN_CLASS_RTTI( CQuestBehaviorCtrlBlock )
	PARENT_CLASS( CQuestScopeBlock )
	PROPERTY_EDIT( m_saveMode, TXT("Save mode for behavior block") );
	PROPERTY_EDIT( m_activationTimeout, TXT( "Block activation time limit." ) )
	PROPERTY_INLINED( m_groups, TXT( "Controlled behavior groups" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class IQuestBehaviorCtrlScopedBlock
{
public:
	virtual ~IQuestBehaviorCtrlScopedBlock() {}

	// Returns the parent scoped behavior control block in which this block is embedded.
	virtual CQuestBehaviorCtrlBlock* GetParentBehaviorBlock() = 0;
};
