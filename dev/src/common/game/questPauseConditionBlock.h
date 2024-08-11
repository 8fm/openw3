/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


///////////////////////////////////////////////////////////////////////////////

class IQuestCondition;

///////////////////////////////////////////////////////////////////////////////

class CQuestPauseConditionBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestPauseConditionBlock, CQuestGraphBlock, 0 )

private:
	// instance data
	TInstanceVar< TDynArray< IQuestCondition* > > i_activeConditions;

	// block data
	TDynArray< IQuestCondition* > m_conditions;

	// garbage collector support
	mutable TDynArray< IQuestCondition* > m_gcList;

public:
	CQuestPauseConditionBlock() { m_name = TXT("Pause"); }

	//! CObject interface
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Octagon; }
	virtual Color GetClientColor() const { return Color( 155, 187, 89 ); }
	virtual String GetBlockCategory() const { return TXT( "Flow control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	
#endif

	virtual void OnSerialize( IFile& file );
	RED_INLINE TDynArray< IQuestCondition* >& GetConditions() { return m_conditions; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;

private:
	//! Resets the runtime conditions
	void ResetRuntimeConditions( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestPauseConditionBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_conditions, TXT( "Conditions that needs to be fulfilled" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
