/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraph.h"
#include "questBehaviorCtrlBlock.h"


class CQuestBehaviorEventBlock;

class SQuestBehaviorEvent :	public CObject, 
										public IBehaviorGraphProperty, 
										public IQuestBehaviorCtrlScopedBlock
{
	DECLARE_ENGINE_CLASS( SQuestBehaviorEvent, CObject, 0 )

public:
	CName			m_npcTag;
	CName			m_behaviorEvent;

	// ------------------------------------------------------------------------
	// IBehaviorGraphProperty implementation
	// ------------------------------------------------------------------------
	virtual CBehaviorGraph* GetParentGraph();

	// ------------------------------------------------------------------------
	// IQuestBehaviorCtrlScopedBlock implementation
	// ------------------------------------------------------------------------
	virtual CQuestBehaviorCtrlBlock* GetParentBehaviorBlock();
};
BEGIN_CLASS_RTTI( SQuestBehaviorEvent )
	PARENT_CLASS( CObject )
	PROPERTY_CUSTOM_EDIT( m_npcTag, TXT( "Tag of the affected NPC" ), TXT( "QuestBehaviorTagsSelection" ) )
	PROPERTY_CUSTOM_EDIT( m_behaviorEvent, TXT( "Event we wish to send" ), TXT( "BehaviorEventSelection" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

// This block emits an event to the specified NPC's behavior
class CQuestBehaviorEventBlock : public CQuestGraphBlock, public IQuestBehaviorCtrlScopedBlock
{
	DECLARE_ENGINE_CLASS( CQuestBehaviorEventBlock, CQuestGraphBlock, 0 )

protected:
	TDynArray< SQuestBehaviorEvent* >	m_events;
	Float								m_timeout;

private:
	TInstanceVar< Float >				i_startTime;

public:
	CQuestBehaviorEventBlock();

	// Checks if the block is configured with a tag
	Bool DoesContainTags( const TagList& tags ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetClientColor() const { return Color( 207, 153, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Behavior control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;

	//! Caches the block connections, removing the actual connections and memorizing only the blocks they are connected to
	virtual void CacheConnections();

#endif

	// ------------------------------------------------------------------------
	// IQuestBehaviorCtrlScopedBlock implementation
	// ------------------------------------------------------------------------
	virtual CQuestBehaviorCtrlBlock* GetParentBehaviorBlock();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

private:
	Bool VerifyConnectedNotifications( InstanceBuffer& data, CQuestThread* parentThread, String& errorMsg ) const;
	Bool RaiseEvent( InstanceBuffer& data, String& errorMsg ) const;
	Bool VerifyEventsProcessed( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestBehaviorEventBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_events, TXT( "Events" ) )
	PROPERTY_EDIT( m_timeout, TXT("Timeout") )
END_CLASS_RTTI()
