/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idResource.h"
#include "idTopic.h"
#include "idBasicBlocks.h"
#include "../../common/engine/localizationManager.h"

IMPLEMENT_ENGINE_CLASS( SIDInterlocutorDefinition )
IMPLEMENT_ENGINE_CLASS( SIDActorDefinition )
IMPLEMENT_ENGINE_CLASS( CInteractiveDialog )

CInteractiveDialog::CInteractiveDialog()
	: m_autoFinish( true )
{
}

const CIDTopic* CInteractiveDialog::FindTopicByName( CName name ) const
{
	for ( Uint32 i = 0; i < m_topics.Size(); ++i )
	{
		if ( m_topics[ i ]->GetName() == name )
		{
			return m_topics[ i ];
		}
	}
	return NULL;
}

void CInteractiveDialog::OnPostLoad()
{
	// Pass to the base class						   
	TBaseClass::OnPostLoad();

	// TODO: this shouldn't be here...
	// OnPostChange() is called to recompile data layout of each topic.
	// Data layout is something that needs to be baked for each platform once, and should happen in cooker.
	// So, in that case, the resource should already have offsets for all the TInstanceVar<>s in place once it's loaded.
	// BUT - we have no cooker yet. AND we need it working in the editor somehow.
	// So, for now, I'll just leave it in the OnPostLoad() method, even if it's not pretty - it's safe for now.

	// touch all topics
	for ( Uint32 i = 0; i < m_topics.Size(); ++i )
	{
		m_topics[ i ]->OnPostChange();	
	} 

	#ifndef NO_EDITOR
	if ( GIsEditor && !GIsEditorGame )
	{
		for ( BlockIterator< CIDGraphBlock > it( this ); it; ++it )
		{
			( *it )->OnEditorPostLoad();
		}
	}
	#endif
}

void CInteractiveDialog::OnPreSave()
{
	TBaseClass::OnPreSave();

#ifndef NO_EDITOR
	SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );
#endif
}

void CInteractiveDialog::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( Uint32 i = 0; i < m_topics.Size(); ++i )
	{
		if ( m_topics[ i ] )
		{
			m_topics[ i ]->GetLocalizedStrings( localizedStrings );
		}
	}
}

CName CInteractiveDialog::GetVoiceTagByInterlocutorId( CName interlocutorId ) const
{
	// subject to change?
	for ( Uint32 i = 0; i < m_interlocutors.Size(); ++i )
	{
		if ( m_interlocutors[ i ].m_interlocutorId == interlocutorId )
		{
			return m_interlocutors[ i ].m_voiceTagToMatch ? m_interlocutors[ i ].m_voiceTagToMatch : interlocutorId;
		}
	}
	return interlocutorId;
}

#ifndef NO_EDITOR
void CInteractiveDialog::SwapTopics( Uint32 currPosition, Uint32 newPosition )
{
	// editor code
	R6_ASSERT( GIsEditor );
	R6_ASSERT( currPosition < m_topics.Size() && newPosition < m_topics.Size() );

	m_topics.Swap( currPosition, newPosition );
	MarkModified();
}

CIDTopic* CInteractiveDialog::NewTopic( CName topicName, Uint32 topicIndex )
{
	R6_ASSERT( GIsEditor && !GIsEditorGame );
	CIDTopic* topic = CreateObject< CIDTopic > ( this );
	if ( topic )
	{
		topic->OnCreatedInEditor();
		topic->SetName( topicName );
		m_topics.Insert( topicIndex, topic );
		MarkModified();
	}

	return topic;
}

void CInteractiveDialog::RemoveTopic( Uint32 topicIndex )
{
	R6_ASSERT( GIsEditor );
	CIDTopic* topic = m_topics[ topicIndex ];
	if ( topic )
	{
		topic->SetParent( nullptr );
		m_topics.RemoveAt( topicIndex );
		MarkModified();
	}
}

void CInteractiveDialog::AddInterlocutor( const SIDInterlocutorDefinition& def )
{
	R6_ASSERT( GIsEditor );
	m_interlocutors.PushBackUnique( def );
}

#endif // ifndef NO_EDITOR

Bool CInteractiveDialog::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == CNAME( actors ) )
	{
		static CName typeName( GetTypeName< TDynArray< SIDInterlocutorDefinition > >() );
		CVariant var( typeName, readValue.GetData() );  
		var.AsType( m_interlocutors );
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

