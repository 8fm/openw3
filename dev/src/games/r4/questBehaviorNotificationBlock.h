/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questBehaviorCtrlBlock.h"

class CBehaviorGraph;
class CQuestBehaviorEventBlock;
class CQuestControlledNPCsManager;

class SQuestBehaviorNotification :	public CObject, 
												public IBehaviorGraphProperty, 
												public IQuestBehaviorCtrlScopedBlock
{
	DECLARE_ENGINE_CLASS( SQuestBehaviorNotification, CObject, 0 )

public:
	CName							m_npcTag;
	CName							m_notification;
	Bool							m_all;

	SQuestBehaviorNotification() : m_all( false ) {}

	// runtime data
	TInstanceVar< Uint32 >		i_expectedNotificationsCount;
	TInstanceVar< Uint32 >		i_receivedNotificationsCount;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	void OnInitInstance( InstanceBuffer& instanceData ) const;

	void SetExpectedNotificationsCount( InstanceBuffer& data, CQuestControlledNPCsManager& mgr ) const;
	void UpdateReceivedNotificationsCount( InstanceBuffer& data, CQuestControlledNPCsManager& mgr ) const;
	Bool IsFulfilled( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// IBehaviorGraphProperty implementation
	// ------------------------------------------------------------------------
	virtual CBehaviorGraph* GetParentGraph();

	// ------------------------------------------------------------------------
	// IQuestBehaviorCtrlScopedBlock implementation
	// ------------------------------------------------------------------------
	virtual CQuestBehaviorCtrlBlock* GetParentBehaviorBlock();
};
BEGIN_CLASS_RTTI( SQuestBehaviorNotification )
	PARENT_CLASS( CObject )
	PROPERTY_CUSTOM_EDIT( m_npcTag, TXT( "Tag of the affected NPC" ), TXT( "QuestBehaviorTagsSelection" ) )
	PROPERTY_CUSTOM_EDIT( m_notification, TXT( "Notification we're looking forward to" ), TXT( "BehaviorNotificationSelection" ) )
	PROPERTY_EDIT( m_all, TXT( "Should we wait until all controlled entities receive the notification?" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

// This is a special pause block - one that awaits a notification
// from the specified behavior
class CQuestBehaviorNotificationBlock : public CQuestGraphBlock, public IQuestBehaviorCtrlScopedBlock
{
	DECLARE_ENGINE_CLASS( CQuestBehaviorNotificationBlock, CQuestGraphBlock, 0 )

protected:
	// runtime data
	TInstanceVar< Uint32 >								i_expectedNotificationsCount;
	TInstanceVar< Uint32 >								i_receivedNotificationsCount;

	// static data
	TDynArray< SQuestBehaviorNotification* >	m_notifications;

public:
	CQuestBehaviorNotificationBlock();

	// Returns the tag the block is configured with
	TagList GetNPCTags() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Octagon; }
	virtual Color GetClientColor() const { return Color( 207, 153, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Behavior control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;

#endif

	//! Called by the quests system whenever a behavior notification is received
	void OnNotification( const CBehaviorGraph* behavior, const CName& notification );

	// ------------------------------------------------------------------------
	// IQuestBehaviorCtrlScopedBlock implementation
	// ------------------------------------------------------------------------
	virtual CQuestBehaviorCtrlBlock* GetParentBehaviorBlock();

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	
private:
	Uint32 GetControlledEntitiesCount() const;
	Bool IsFulfilled( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestBehaviorNotificationBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_notifications, TXT( "Notifications" ) )
END_CLASS_RTTI()
