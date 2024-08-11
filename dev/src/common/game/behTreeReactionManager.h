/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "behTreeReactionData.h"
#include "binaryStorage.h"

///////////////////////////////////////////////////////////////////////////////

class CReactionScene;

class CBehTreeReactionManager : public CObject
{	
	DECLARE_ENGINE_CLASS( CBehTreeReactionManager, CObject, 0 );	

	static Float CHATING_IN_AP_DISTANCE_SQR;

protected:
	TDynArray<CBehTreeReactionEventData*>	m_reactionEvents;
	TDynArray< THandle< CReactionScene > >	m_reactionScens;	
	TArrayMap< String, CName >				m_reactionSceneGroups;
	TDynArray< TPointerWrapper< CActor > >  m_actors;
	TDynArray< THandle< CNewNPC > >			m_rainAwareNPCs;
	TDynArray< THandle< CAreaComponent > >	m_suppressedAreas;
	Bool									m_suppressScaredReactions;



public:
	CBehTreeReactionManager();

	void Init();

	Bool CreateReactionEventIfPossible( CEntity* invoker, CName eventName, Float lifetime, Float distanceRange, Float broadcastInterval, Int32 recipientCount, Bool skipInvoker, Bool useCustomReactionCenter = false, const Vector& reactionCenter = Vector::ZEROS );

	CReactionScene* CreateReactionScene( CName name, CEntity* invoker, CBehTreeReactionEventData* sourceEvent );

	Bool CreateReactionEvent( CEntity* invoker, CName name, Float lifetime , Float distanceRange, Float broadcastInterval, Int32 recipientCount, Bool isScene, Bool skipInvoker, Bool useCustomReactionCenter = false, const Vector& reactionCenter = Vector::ZEROS );
	Bool RemoveReactionEvent( CEntity* invoker, CName name );
	Bool AddReactionEvent( CBehTreeReactionEventData* data );

	void OnSerialize( IFile& file ) override;
	virtual void Update();

	void AddReactionSceneGroup( const String&  voiceset, const CName group );	
	CName FindGroupForVoiceset( const String& voiceset );

	void AddRainAwareNPC( CNewNPC* npc );
	void RemoveRainAwareNPC( CNewNPC* npc );

	Bool SuppressReactions( Bool toggle, CName areaTag );
	void SuppressScaredReactions( Bool toggle ) { m_suppressScaredReactions = toggle; }

	static Bool IsScaryEvent( CName eventName );

protected:	

	void virtual FindActorsToBroadecast( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data );

	Bool DeleteReactionEvent( Uint32 index );
	void Broadcast( CBehTreeReactionEventData& data );
	void BroadcastReactionScene( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data );
	
	void funcCreateReactionEvent( CScriptStackFrame& stack, void* result );
	void funcAddReactionSceneGroup( CScriptStackFrame& stack, void* result );
	void funcCreateReactionEventCustomCenter( CScriptStackFrame& stack, void* result );
	void funcRemoveReactionEvent( CScriptStackFrame& stack, void* result );
	void funcInitReactionScene( CScriptStackFrame& stack, void* result );
	void funcSuppressReactions( CScriptStackFrame& stack, void* result );
	void funcCreateReactionEventIfPossible( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CBehTreeReactionManager );
	PARENT_CLASS( CObject );
	PROPERTY( m_reactionScens	);
	PROPERTY( m_reactionEvents	);
	NATIVE_FUNCTION( "AddReactionEvent"					, funcCreateReactionEvent				);
	NATIVE_FUNCTION( "RemoveReactionEvent"				, funcRemoveReactionEvent				);
	NATIVE_FUNCTION( "CreateReactionEvent"				, funcCreateReactionEvent				);
	NATIVE_FUNCTION( "CreateReactionEventCustomCenter"	, funcCreateReactionEventCustomCenter	);
	NATIVE_FUNCTION( "InitReactionScene"				, funcInitReactionScene					);
	NATIVE_FUNCTION( "AddReactionSceneGroup"			, funcAddReactionSceneGroup				);
	NATIVE_FUNCTION( "SuppressReactions"				, funcSuppressReactions					);
	NATIVE_FUNCTION( "CreateReactionEventIfPossible"	, funcCreateReactionEventIfPossible		);
END_CLASS_RTTI();
