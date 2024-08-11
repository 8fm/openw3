/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

// A quest block capable of playing an interactive
class CQuestGraphBlockInteractiveDialog : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestGraphBlockInteractiveDialog, CQuestGraphBlock, 0 )

private:
	// instance data
	TInstanceVar< TGenericPtr >			i_request;

	// properties data
	TSoftHandle< CInteractiveDialog >	m_dialog;

public:
	CQuestGraphBlockInteractiveDialog() { m_name = TXT("Interactive dialog"); }

	// !IQuestBlockWithScene interface

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 79, 129, 189 ); }
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
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

private:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void RebuildSceneInterfaceSockets();
#endif

};

BEGIN_CLASS_RTTI( CQuestGraphBlockInteractiveDialog )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_dialog, TXT( "Interactive dialog file" ) )
END_CLASS_RTTI()

class CQuestInteractiveDialogHelper
{
public:

	static SDialogStartRequest* PlayDialogForPlayer( const TSoftHandle<CInteractiveDialog>& dialog );

	static EIDPlayState IsDialogPlaying( SDialogStartRequest* info );
};
