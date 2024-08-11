/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questExternalScenePlayer.h"

#include "questScenePlayer.h"
#include "../core/gameSave.h"

RED_DEFINE_STATIC_NAME( ExternalDialog );

void CQuestExternalScenePlayer::Activate( Bool enableFlag )
{
	if ( enableFlag == false )
	{
		m_externalDialogs.Clear();
		m_activeDialogs.Clear();
	}
}

void CQuestExternalScenePlayer::RegisterDialog( CQuestScenePlayer& player, const CGUID& hostBlockGUID, const TagList& actorsTags )
{
	const TDynArray< CName >& tags = actorsTags.GetTags();
	Uint32 count = tags.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		ExternalDialogs::iterator it = m_externalDialogs.Find( tags[ i ] );
		if ( it == m_externalDialogs.End() )
		{
			m_externalDialogs.Insert( tags[ i ], DialogsStack() );
			it = m_externalDialogs.Find( tags[ i ] );
		}

		// find an entry where we can insert the dialog
		DialogsStack& stack = it->m_second;
		Bool blockFound = false;
		for ( DialogsStack::iterator stackIt = stack.Begin(); stackIt != stack.End(); ++stackIt )
		{
			if ( stackIt->m_player == nullptr && stackIt->m_hostBlockGUID == hostBlockGUID )
			{
				stackIt->m_player = &player;
				blockFound = true;
				break;
			}
		}
		if ( !blockFound )
		{
			stack.PushBack( DialogDef( hostBlockGUID, &player ) );
		}
	}

	m_activeDialogs.AddTags( actorsTags );
}

void CQuestExternalScenePlayer::RemoveDialogInternal( CQuestScenePlayer* player )
{
	TDynArray< CName > tagsToRemove;
	for ( ExternalDialogs::iterator it = m_externalDialogs.Begin(); it != m_externalDialogs.End(); ++it )
	{
		DialogsStack& stack = it->m_second;

		while ( stack.Remove( DialogDef( CGUID(), player ) ) ) {}

		if ( stack.Empty() )
		{
			tagsToRemove.PushBack( it->m_first );
		}
	}

	for ( TDynArray< CName >::const_iterator it = tagsToRemove.Begin(); it != tagsToRemove.End(); ++it )
	{
		m_externalDialogs.Erase( *it );
	}

	// update list of tags that have interaction dialogs assigned
	m_activeDialogs.Clear();
	for ( ExternalDialogs::const_iterator it = m_externalDialogs.Begin(); it != m_externalDialogs.End(); ++it )
	{
		m_activeDialogs.AddTag( it->m_first );
	}
}

Bool CQuestExternalScenePlayer::StartDialog( const CName& actorTag, const String& inputName /*= String::EMPTY */ )
{
	ExternalDialogs::iterator it = m_externalDialogs.Find( actorTag );
	if ( it == m_externalDialogs.End() )
	{
		return false;
	}
	else
	{
		DialogsStack& stack = it->m_second;
		ASSERT( !stack.Empty() );

		CQuestScenePlayer* questScenePlayer = stack.Back().m_player;

		ASSERT( questScenePlayer != NULL && "Trying to play a dialog that's not been restored during a load" );
		if ( questScenePlayer && questScenePlayer->IsReadyForStart() )
		{
			questScenePlayer->Play( inputName );
			return true;
		}
		else
		{
			return false;
		}
	}
}

Bool CQuestExternalScenePlayer::AreThereAnyDialogsForActor( const CActor& actor )
{
	if ( actor.GetTags().Empty() )
	{
		return false;
	}
	else
	{
		Bool areThereDialogs = TagList::MatchAny( actor.GetTags(), m_activeDialogs );
		return areThereDialogs;
	}
}

void CQuestExternalScenePlayer::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME(CQuestExternalScenePlayer) );

	// save the dialogs stack - excluding the player instances - upon a load
	// those will be filled when the respective blocks
	// will start registering the players
	Uint32 count = m_externalDialogs.Size();
	saver->WriteValue( CNAME( tagsCount ), count );

	for ( ExternalDialogs::const_iterator tagIt = m_externalDialogs.Begin(); tagIt != m_externalDialogs.End(); ++tagIt )
	{
		CGameSaverBlock block( saver, CNAME(ExternalDialog) );

		saver->WriteValue( CNAME( tag ), tagIt->m_first );

		const DialogsStack& stack = tagIt->m_second;
		count = stack.Size();
		saver->WriteValue( CNAME( dialogsCount ), count );
		for ( DialogsStack::const_iterator dialogIt = stack.Begin(); dialogIt!= stack.End(); ++dialogIt )
		{
			saver->WriteValue( CNAME( guid ), dialogIt->m_hostBlockGUID );
		}
	}

	END_TIMER_BLOCK( time )
}

void CQuestExternalScenePlayer::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(CQuestExternalScenePlayer) );

	// at this point we want to recreate both the tags and the dialogs
	// collection, recreating their respective contents while leaving
	// only the player instances undefined - that will be taken care
	// of by the respective dialog blocks that will register
	// them when they are recreated
	Uint32 tagsCount = loader->ReadValue( CNAME( tagsCount ), (Uint32)0 );
	for ( Uint32 tagIdx = 0; tagIdx < tagsCount; ++tagIdx )
	{
		CGameSaverBlock block( loader, CNAME(ExternalDialog) );

		CName tag = loader->ReadValue( CNAME( tag ), CName::NONE );
		ASSERT( tag != CName::NONE );
		if ( tag == CName::NONE )
		{
			// invalid tag loaded
			continue;
		}

		// load the stack structure
		DialogsStack stack;
		Uint32 dialogsCount = loader->ReadValue( CNAME( dialogsCount ), (Uint32)0 );
		for( Uint32 dialogIdx = 0; dialogIdx < dialogsCount; ++dialogIdx )
		{
			CGUID guid = loader->ReadValue( CNAME( guid ), CGUID() );
			stack.PushBack( DialogDef( guid ) );
		}

		// insert the stack
		m_externalDialogs.Insert( tag, stack );
		m_activeDialogs.AddTag( tag );
	}
}
