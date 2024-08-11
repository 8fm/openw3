/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockText.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idInterlocutor.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockText )

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
String CIDGraphBlockText::GetCaption() const
{	
	String caption;
	
	// Add block name to the caption
	if ( m_name )
	{
		caption = TXT("[");
		caption += m_name.AsString() + TXT("]\n");
	}	

	// Limit number of displayed lines to 4. If it's more than 4, display first 3 lines and the last one
	Uint32 lineLimit = m_lines.Size();
	if ( lineLimit > 4 )
	{
		lineLimit = 3;
	}

	// Add lines...
	for ( Uint32 i = 0; i < lineLimit; ++i )
	{
		if ( !caption.Empty() )
		{
			caption += TXT("\n");
		}
		caption += TrimCaptionLine( m_lines[ i ].m_speaker.AsString() + TXT(": ") + m_lines[ i ].m_text.GetString() );
	}

	// Add last line if needed
	if ( lineLimit < m_lines.Size() )
	{
		const Uint32 i = m_lines.Size() - 1; // safe, because lineLimit > 4
		caption += TXT("\n...\n");
		caption += TrimCaptionLine( m_lines[ i ].m_speaker.AsString() + TXT(": ") + m_lines[ i ].m_text.GetString() );
	}

	// Return pre-made caption
	return caption;
}


void CIDGraphBlockText::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) );
}

void CIDGraphBlockText::OnPropertyPostChange( IProperty* prop ) 
{
	RefreshVoiceovers();

	TBaseClass::OnPropertyPostChange( prop );
}

void CIDGraphBlockText::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_nextLine ] = 0;
}

void CIDGraphBlockText::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_nextLine;

	RefreshVoiceovers(); // temp hack, just for the editor
}

const CIDGraphBlock* CIDGraphBlockText::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true*/ ) const
{
	// restart the block
	InstanceBuffer& data = topicInstance->GetInstanceData();
	data[ i_nextLine ] = 0;

	RED_ASSERT( m_lines.Size() > 0, TXT("Text block %s has no lines, makes no sense"), GetName().AsString() );
	if( m_lines.Size() > 0 && m_lines[ 0 ].m_speaker )
	{
		CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( m_lines[ 0 ].m_speaker );
		RED_ASSERT( interlocutor, TXT("Interlocutor not found on block %s"), GetName().AsString() );
		PlayNextLine( interlocutor, topicInstance );
	}

	// pass to base class
	return TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
}

const CIDGraphBlock* CIDGraphBlockText::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	InstanceBuffer&	data		= topicInstance->GetInstanceData();
	const Uint32	lineLimit	= m_lines.Size();
	Uint32			curLine		= data[ i_nextLine ] - 1;

	if ( curLine > lineLimit )
	{
		// nothing to do in here, we already said everything and already activated the output once
		// we shouldn't do it more than once - so if we're still in this block for some reason
		// (for example: if the only connected block is choice) - then we should just stay here.
		return this;
	}

	Uint32						dialogInstance	= topicInstance->GetDialogInstanceID();
	CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( m_lines[ curLine ].m_speaker );

	// If for some reason, there is no interlocutor, consider the block ended ( we could just skip the line but this should not even happen
	if( !interlocutor )
	{
		RED_LOG( Dialog, TXT("Interlocutor not found on block %s"), GetName().AsString() );		
		return TryToPlayNextLine( interlocutor, topicInstance, timeDelta );
	}

	// Get the state of the line being played
	EIDLineState	lineState	= interlocutor->GetLineStatus( m_lines[ curLine ], dialogInstance );
	switch( lineState )
	{
	case DILS_Queued:				
		break;
	case DILS_Playing:				
		break;
	case DILS_Completed:			
		return TryToPlayNextLine( interlocutor, topicInstance, timeDelta );
	case DILS_CancelledOrNotPlayed:	
		return TryToPlayNextLine( interlocutor, topicInstance, timeDelta );
	default:
		R6_ASSERT( false, TXT("Missing line state to implement in the class CIDGraphBlockText") );
		break;
	}

	return this;
 }

const CIDGraphBlock* CIDGraphBlockText::ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate /*= true*/ ) const
{
	return TBaseClass::ActivateOutput( topicInstance, timeDelta, output, evaluate );
}

const CIDGraphBlock*  CIDGraphBlockText::TryToPlayNextLine( CIDInterlocutorComponent* interlocutor, CIDTopicInstance* topicInstance, Float timeDelta ) const
{
	InstanceBuffer&	data		= topicInstance->GetInstanceData();
	Uint32			nextLine	= data[ i_nextLine ];
	const Uint32	lineLimit	= m_lines.Size();

	if( nextLine < lineLimit )
	{
		PlayNextLine( interlocutor, topicInstance );
		return this;
	}
	else
	{
		return EndBlock( topicInstance, timeDelta );
	}
}

void CIDGraphBlockText::PlayNextLine( CIDInterlocutorComponent* interlocutor, CIDTopicInstance* topicInstance ) const
{
	InstanceBuffer& data = topicInstance->GetInstanceData();
	Uint32 nextLine = data[ i_nextLine ];

	// interlocutor may have changed
	interlocutor = topicInstance->GetDialogInstance()->GetInterlocutor( m_lines[ nextLine ].m_speaker );

	if ( interlocutor )
	{
		interlocutor->TryToPlayLine( m_lines[ nextLine ], topicInstance->GetDialogInstanceID() );
	}
	else
	{
		RED_LOG( Dialog, TXT("Interlocutor %s not found while playing line %ld in %s"), 
			m_lines[ nextLine ].m_speaker.AsString().AsChar(), nextLine, GetFriendlyName().AsChar() );
	}

	data[ i_nextLine ]++;
}

const CIDGraphBlock*  CIDGraphBlockText::EndBlock( CIDTopicInstance* topicInstance, Float timeDelta ) const
{
	return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
}

void CIDGraphBlockText::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		localizedStrings.PushBack( LocalizedStringEntry( &m_lines[ i ].m_text, TXT("Dialog line"), Cast< CInteractiveDialog > ( GetParent()->GetParent() ), GetVoiceoverFileNameForLine( i ), GetStringKeyForLine( i ) ) );
	}
}

Color CIDGraphBlockText::GetClientColor() const
{
	return Color( 0xff444444 );
}

String CIDGraphBlockText::GetVoiceoverFileNameForLine( Uint32 lineIndex ) const
{
	CName voiceName = m_lines[ lineIndex ].m_speaker;
	const CInteractiveDialog *dialog = Cast< const CInteractiveDialog > ( GetRoot() );
	if ( dialog )
	{
		voiceName = dialog->GetVoiceTagByInterlocutorId( m_lines[ lineIndex ].m_speaker );
	}

	if ( !voiceName )
	{
		return String::EMPTY;
	}

	if ( m_lines[ lineIndex ].m_text.GetIndex() < 1 )
	{
		return String::EMPTY;
	}

	return String::Printf( TXT("%s_ID_%s_%09ld"), GCommonGame->GetGamePrefix(), voiceName.AsString().AsChar(), m_lines[ lineIndex ].m_text.GetIndex() );
}

String CIDGraphBlockText::GetStringKeyForLine( Uint32 lineIndex ) const
{
	// TODO: do we even need this?
	return String::Printf( TXT("idline%09ld"), m_lines[ lineIndex ].m_text.GetIndex() );
}

String CIDGraphBlockText::GetStringInCurrentLanguageForLine( Uint32 lineIndex ) const
{
	return m_lines[ lineIndex ].m_text.GetString();
}

void CIDGraphBlockText::RefreshVoiceovers()
{
#ifndef NO_EDITOR
	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		m_lines[ i ].m_exampleVoiceoverFile = GetVoiceoverFileNameForLine( i );
	}
#endif
}

Bool CIDGraphBlockText::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
#if VER_IDLINES_REFACTOR > VER_MINIMAL
	if ( propertyName == CNAME( lines ) )
	{
		TDynArray< SIDLine > oldTypeLines;
		if ( readValue.AsType( oldTypeLines ) )
		{
			if ( !oldTypeLines.Empty() )
			{
				m_lines.Resize( oldTypeLines.Size() );
				for ( Uint32 i = 0; i < oldTypeLines.Size(); ++i )
				{
					m_lines[ i ].m_speaker = oldTypeLines[ i ].m_speaker;
					m_lines[ i ].m_receiver = oldTypeLines[ i ].m_receiver;
					m_lines[ i ].m_interruptReceiver = oldTypeLines[ i ].m_interruptReceiver;
					m_lines[ i ].m_text = oldTypeLines[ i ].m_text;
				}
				return true;
			}
		}
	}
#endif
	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Color CIDGraphBlockText::GetTitleColor() const
{
	return Color::BROWN;
}
