/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

// This block has the capability of disabling another currently
// active block (or a group of blocks) thus disabling further
// quest control flow in the branch the blocks were on.
class CQuestCutControlBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestCutControlBlock, CQuestGraphBlock, 0 )

protected:
	Bool m_permanent;

public:

	CQuestCutControlBlock() : m_permanent( true ) { m_name = TXT("Cut control"); }

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
	virtual Color GetClientColor() const { return Color( 214, 138, 100 ); }
	virtual String GetCaption() const { return TXT("Cut control"); }
	virtual String GetBlockCategory() const { return TXT( "Flow control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif
	
	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const override;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const override;

	Bool IsPermanent() const { return m_permanent; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestCutControlBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_permanent, TXT( "Should block be disabled permanently?" ) );
END_CLASS_RTTI()
