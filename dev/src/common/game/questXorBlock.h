/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questVariedInputsBlock.h"


class CQuestXorBlock : public CQuestVariedInputsBlock
{
	DECLARE_ENGINE_CLASS( CQuestXorBlock, CQuestVariedInputsBlock, 0 )

private:
	// instance variables
	TInstanceVar< Bool >		i_wasInputActivated;	//!< Flag specifying if an input was activated
	TInstanceVar< Bool >		i_wasOutputActivated;

public:
	CQuestXorBlock() { m_name = TXT("XOR"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 173, 86, 86 ); }
	virtual String GetCaption() const { return TXT("XOR"); }
	virtual String GetBlockCategory() const { return TXT( "Logical" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;
};

BEGIN_CLASS_RTTI( CQuestXorBlock )
	PARENT_CLASS( CQuestVariedInputsBlock )
END_CLASS_RTTI()