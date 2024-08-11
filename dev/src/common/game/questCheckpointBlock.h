/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CQuestCheckpointBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestCheckpointBlock, CQuestGraphBlock, 0 )

private:
	enum ESavePhase
	{
		SP_None,
		SP_MakeSave,
		SP_WaitForSaveEnd,
		SP_Exit
	};

private:
	Bool					m_enableSaving;
	Bool					m_ignoreSaveLocks;

	TInstanceVar< Int32 >	i_savePhase;

public:

	CQuestCheckpointBlock() : m_enableSaving( true ), m_ignoreSaveLocks( false ) { m_name = TXT("Checkpoint"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_DoubleCircle; }
	virtual Color GetClientColor() const { return Color( 155, 187, 89 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;


	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;
};

BEGIN_CLASS_RTTI( CQuestCheckpointBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_enableSaving, TXT( "Enable saving of this checkpoint" ) )
	PROPERTY_EDIT( m_ignoreSaveLocks, TXT( "Should ignore save locks?" ) )
END_CLASS_RTTI()
