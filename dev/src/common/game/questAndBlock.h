/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questVariedInputsBlock.h"


class CQuestAndBlock : public CQuestVariedInputsBlock
{
	DECLARE_ENGINE_CLASS( CQuestAndBlock, CQuestVariedInputsBlock, 0 )

private:
	// instance variables
	TInstanceVar< TDynArray< CName > >		i_activatedInputs;	//!< List with names of all activated inputs
	
public:
	CQuestAndBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual String GetCaption() const { return TXT("AND"); }

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 151, 111, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Logical" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
	
	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;
};

BEGIN_CLASS_RTTI( CQuestAndBlock )
	PARENT_CLASS( CQuestVariedInputsBlock )
END_CLASS_RTTI()

