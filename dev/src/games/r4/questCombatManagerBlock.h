/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/engine/globalEventsManager.h"

class CTaggedActorsListener : public IGlobalEventsListener
{
	DECLARE_RTTI_SIMPLE_CLASS( CTaggedActorsListener )
protected:
	Bool							m_isDirty;
	Bool							m_wasRegistered;
public:
	CTaggedActorsListener()
		: m_isDirty( false )
		, m_wasRegistered( false )												{}

	~CTaggedActorsListener();

	Bool							IsDirty()									{ return m_isDirty; }
	void							ClearDirty()								{ m_isDirty = false; }

	void							Register( const TagList& tagList );
	void							Unregister( const TagList& tagList );

	// IGlobalEventsListener
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;
};

BEGIN_NODEFAULT_CLASS_RTTI( CTaggedActorsListener )
END_CLASS_RTTI()

class IQuestCombatManagerBaseBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( IQuestCombatManagerBaseBlock, CQuestGraphBlock, 0 )
protected:
	typedef TDynArray< THandle< CActor > >	ActorList;

	TagList									m_npcTags;

	Bool									m_overrideGuardArea;
	CName									m_guardAreaTag;
	CName									m_pursuitAreaTag;
	Float									m_pursuitRange;

	TInstanceVar< ActorList >				i_effectedActors;
	TInstanceVar< CTaggedActorsListener >	i_changesListener;

	mutable THandle< IAITree >				m_cachedTree;

	void									ApplyToActor( CActor* actor ) const;
	void									ClearForActor( CActor* actor ) const;

	void									EffectActors( InstanceBuffer& data ) const;

public:
	IQuestCombatManagerBaseBlock();

	virtual void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void							OnInitInstance( InstanceBuffer& instanceData ) const override;
	virtual void							OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
	virtual void							OnExecute( InstanceBuffer& data ) const override;
	virtual void							OnDeactivate( InstanceBuffer& data ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
private:
	//! CGraphBlock interface
	virtual void							OnRebuildSockets() override;
	virtual String							GetBlockName() const override;
	virtual EGraphBlockShape				GetBlockShape() const override;
	virtual Color							GetClientColor() const override;
	virtual String							GetBlockCategory() const override;
	virtual Bool							CanBeAddedToGraph( const CQuestGraph* graph ) const override;
#endif // NO_EDITOR_GRAPH_SUPPORT
};

BEGIN_CLASS_RTTI( IQuestCombatManagerBaseBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_npcTags, TXT("NPC tags") )
	PROPERTY_EDIT( m_overrideGuardArea, TXT("Turn on override guard area functionality") )
	PROPERTY_EDIT( m_guardAreaTag, TXT("Guard area tag (if 'overrideGuardArea' set)") )
	PROPERTY_EDIT( m_pursuitAreaTag, TXT("Pursuit area tag (if 'overrideGuardArea' set). Leave empty if you don't want to touch pursuit area") )
	PROPERTY_EDIT( m_pursuitRange, TXT("Pursuit range, leave '-1' for default (if 'overrideGuardArea' set)") )
END_CLASS_RTTI()

