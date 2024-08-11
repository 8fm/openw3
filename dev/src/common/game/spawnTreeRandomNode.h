/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "spawnTreeCompositeMember.h"



class CSpawnTreeSelectRandomNode : public ISpawnTreeCompositeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeSelectRandomNode, ISpawnTreeCompositeNode, 0 );

protected:
	GameTime								m_rerollDelay;

	TInstanceVar< GameTime >				i_rerollTime;
	TInstanceVar< Uint16 >					i_currentActiveChild;

public:
	CSpawnTreeSelectRandomNode()
		: m_rerollDelay( GameTime::DAY )													{}

	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						UpdateLogic( CSpawnTreeInstance& instance ) override;


	// IEdSpawnTreeNode interface
	String						GetEditorFriendlyName() const override;

	// Instance buffer interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;

	// Saving state
	Bool						IsNodeStateSaving( CSpawnTreeInstance& instance ) const override;
	void						SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	Bool						LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeSelectRandomNode );
	PARENT_CLASS( ISpawnTreeCompositeNode );
	PROPERTY_CUSTOM_EDIT( m_rerollDelay,	TXT("Timeout to restore spawn capabilities to an entry that was completely cleared"), TXT( "GameTimePropertyEditor" ));
END_CLASS_RTTI();