/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "questGraphBlock.h"

class CQuestFastForwardCommunitiesBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestFastForwardCommunitiesBlock, CQuestGraphBlock, 0 )
protected:
	Bool								m_manageBlackscreen;
	Bool								m_respawnEveryone;
	Bool								m_dontSpawnHostilesClose;
	Float								m_timeLimit;

	TInstanceVar< EngineTime >			i_timeLimit;

public:
	CQuestFastForwardCommunitiesBlock()
		: m_manageBlackscreen( true )
		, m_respawnEveryone( false )
		, m_dontSpawnHostilesClose( false )
		, m_timeLimit( -1.f )																{ m_name = TXT( "Fast forward communities" ); }


	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const override;

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
	virtual void OnExecute( InstanceBuffer& data ) const override;
	virtual void OnDeactivate( InstanceBuffer& data ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const override;
	virtual Color GetClientColor() const override;
	virtual void OnRebuildSockets() override;
	virtual String GetBlockCategory() const override;
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const override;

#endif
};


BEGIN_CLASS_RTTI( CQuestFastForwardCommunitiesBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_manageBlackscreen, TXT( "Should block manage blackscreen (or its managed by some other logic)." ) )
	PROPERTY_EDIT( m_respawnEveryone, TXT("Despaw everyony when block is started.") )
	PROPERTY_EDIT( m_dontSpawnHostilesClose, TXT("Don't spawn hostiles close.") )
	PROPERTY_EDIT(  m_timeLimit, TXT("Execution time limit. (<0 for no limit)") );
END_CLASS_RTTI()