/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestScenePlayer;

class CQuestExternalScenePlayer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	struct DialogDef
	{
		CGUID					m_hostBlockGUID;
		CQuestScenePlayer*		m_player;
		DialogDef( const CGUID& hostBlockGUID = CGUID(), CQuestScenePlayer* player = NULL )
			: m_hostBlockGUID( hostBlockGUID )
			, m_player( player )
		{}

		RED_INLINE Bool operator==( const DialogDef& rhs ) const { return m_player == rhs.m_player; }
	};
private:
	typedef TDynArray< DialogDef >				DialogsStack;
	typedef THashMap< CName, DialogsStack >			ExternalDialogs;

private:
	ExternalDialogs	m_externalDialogs;
	TagList			m_activeDialogs;


	void RemoveDialogInternal( CQuestScenePlayer* player );
public:
	void Activate( Bool enableFlag );
	void RegisterDialog( CQuestScenePlayer& player, const CGUID& hostBlockGUID, const TagList& actorsTags );
	void UnregisterDialog( CQuestScenePlayer& player ){	RemoveDialogInternal( &player ); }
	
	Bool StartDialog( const CName& actorTag, const String& inputName = String::EMPTY );
	Bool AreThereAnyDialogsForActor( const CActor& actor );

	void ClearBrokenDialogs() { RemoveDialogInternal( nullptr ); }

	//! Save game
	void OnSaveGame( IGameSaver* saver );
	void OnLoadGame( IGameLoader* loader );
};