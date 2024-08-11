/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockChoice.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idThread.h"
#include "idCondition.h"
#include "idInterlocutor.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockChoice )
IMPLEMENT_ENGINE_CLASS( SIDOption )
IMPLEMENT_RTTI_ENUM( EIDChoiceType )


//------------------------------------------------------------------------------------------------------------------
// Choice option
//------------------------------------------------------------------------------------------------------------------
Bool SIDOption::CanOptionBeShown( CIDTopicInstance* topicInstance ) const
{
	const Bool conditionsMet = m_conditions == NULL ? true :m_conditions->IsFulfilled( topicInstance->GetDialogInstanceID() );
	if ( !conditionsMet )
	{
		return false;
	}

	const CEntity* relatedObject = m_relatedObject.Get();
	if ( relatedObject )
	{		
		CIDInterlocutorComponent* player = GCommonGame->GetSystem< CInteractiveDialogSystem > ()->GetPlayerInterlocutor();
		if ( !player )
		{
			return false;
		}

		const CEntity* playersInterest = player->GetEntityWithAttention();
		if( playersInterest )
		{
			return ( playersInterest->GetGUID() == relatedObject->GetGUID() );
		}
		else
		{
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
CIDGraphBlockChoice::CIDGraphBlockChoice()
	: m_clearLeftOption( true )
	, m_clearRightOption( true )
	, m_clearUpOption( true )
	, m_clearDownOption( true )
{
}

String CIDGraphBlockChoice::GetCaption() const
{
	String caption;
	
	// Add block name
	if ( m_name )
	{
		caption = TXT("[");
		caption += m_name.AsString() + TXT("]\n");
	}

	// Add the choices
	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		if ( !caption.Empty() )
		{
			caption += TXT("\n");
		}
		caption += TrimCaptionLine( m_options[ i ].m_text.GetString() ); 
	}

	return caption;
}

void CIDGraphBlockChoice::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );

	ValidateOptions();

	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		CreateSocket( SIDGraphSocketSpawnInfo( CName( GetOutputNameFor( m_options[ i ].m_hudPosition ) ), LSD_Output ) );
	}
}

Color CIDGraphBlockChoice::GetTitleColor() const
{
	return Color::LIGHT_BLUE;
}

EGraphBlockShape CIDGraphBlockChoice::GetBlockShape() const
{
	return GBS_Rounded;
}

Bool CIDGraphBlockChoice::IsOptionActive( CIDTopicInstance* topicInstance, Uint32 option ) const
{
	R6_ASSERT( option < m_options.Size() && option < 32 );

	InstanceBuffer& data = topicInstance->GetInstanceData();
	const Uint32 bitMask = data[ i_optionsActive ];
	return ( 0 != ( bitMask & ( 1 << option ) ) );  
}

void CIDGraphBlockChoice::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );
	data[ i_optionsActive ] = 0;
}

void CIDGraphBlockChoice::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_optionsActive;
}

void CIDGraphBlockChoice::OnEncoutered( CIDTopicInstance* topicInstance ) const
{
	RED_LOG( Dialog, TXT("Choice::OnEncoutered() block %s"), GetName().AsString().AsChar() );


	// Clear marked choices
	if ( m_clearLeftOption )	topicInstance->GetCurrentThread()->ClearChoiceOption( CHOICE_Left );
	if ( m_clearRightOption )	topicInstance->GetCurrentThread()->ClearChoiceOption( CHOICE_Right );
	if ( m_clearUpOption )		topicInstance->GetCurrentThread()->ClearChoiceOption( CHOICE_Up );
	if ( m_clearDownOption )	topicInstance->GetCurrentThread()->ClearChoiceOption( CHOICE_Down );
	
	// Add the choice options
	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		const CIDGraphSocket* output	= NULL;
		Int32 optionIndex = FindOptionIndex( m_options[i].m_hudPosition );
		if ( optionIndex >= 0 )
		{
			output	= FindOutputSocketByIndex( optionIndex );
		}

		topicInstance->GetCurrentThread()->AddChoiceOption( m_options[ i ], output, Int8( FindIndexOfOutputSocket( output ) ) );
	}
}

void CIDGraphBlockChoice::OnPropertyPostChange( IProperty* prop )
{
	ValidateOptions();

	TBaseClass::OnPropertyPostChange( prop );
}

void CIDGraphBlockChoice::ValidateOptions()
{
	if ( m_options.Size() > CHOICE_Max )
	{
		m_options.Resize( CHOICE_Max ); 
	}

	Bool positionsTaken[ CHOICE_Max ];
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		positionsTaken[ i ] = false;
	}

	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		if ( positionsTaken[ m_options[ i ].m_hudPosition ] )
		{
			// ok, this position is already taken, lets find a free position
			Uint32 k;
			for ( k = 0; k < CHOICE_Max; ++k )
			{
				if ( positionsTaken[ k ] == false )
				{
					break;
				}
			}
			R6_ASSERT( k < CHOICE_Max );

			// reassign position
			m_options[ i ].m_hudPosition = EHudChoicePosition( k );
		}

		// mark this position as taken and continue
		positionsTaken[ m_options[ i ].m_hudPosition ] = true;
	}
}

const CIDGraphSocket* CIDGraphBlockChoice::FindOptionOutput( const SIDOption* option ) const
{
	if ( option )
	{
		ptrdiff_t index = m_options.GetIndex( *option );
		return FindOutputSocketByIndex( Int32( index ) );
	}

	return NULL;
}

String CIDGraphBlockChoice::GetOutputNameFor( EHudChoicePosition pos ) const
{
	return CEnum::ToString( pos ).StringAfter( TXT("_") );
}

void CIDGraphBlockChoice::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		localizedStrings.PushBack( LocalizedStringEntry( &m_options[ i ].m_text, TXT("Dialog option"), NULL ) );
	}
}

const CIDGraphBlock* CIDGraphBlockChoice::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true */ ) const
{
	// This block is IsRegular() == false
	// This means it can never get the input through ActivateInput() - use OnEncoutered() instead
	R6_ASSERT( false, TXT("This method shouldn't be ever called on this block. Please DEBUG.") );
	return NULL;
}

const CIDGraphBlock* CIDGraphBlockChoice::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	// This block is IsRegular() == false
	// This means it can never get the input through Evaluate() and should never be m_currentBlock in thread loop - use Update() instead
	R6_ASSERT( false, TXT("This method shouldn't be ever called on this block. Please DEBUG.") )
	return NULL;
}

const CIDGraphBlock* CIDGraphBlockChoice::ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate /*= true */ ) const
{
	/*
	Int8 optionIndex = Int8( FindIndexOfOutputSocket( output ) );
	R6_ASSERT( optionIndex >= 0 );

	// remember selected output
	InstanceBuffer& data = topicInstance->GetInstanceData();
	data[ i_currentOutput ] = optionIndex;

	// infrom thread to remove choices from hud
	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		if ( IsOptionActive( topicInstance, i ) )
		{
			topicInstance->GetCurrentThread()->ClearChoiceSlot( m_options[ i ].m_hudPosition ); 
		}
	}
	*/

	// pass to the base class
	return TBaseClass::ActivateOutput( topicInstance, timeDelta, output, evaluate );
}

const SIDOption* CIDGraphBlockChoice::FindOption( EHudChoicePosition position ) const
{
	Int32 idx = FindOptionIndex( position );
	return idx < 0 ? NULL : &m_options[ idx ];
}

Int32 CIDGraphBlockChoice::FindOptionIndex( EHudChoicePosition position ) const
{
	for ( Int32 i = 0; i < m_options.SizeInt(); ++i )
	{
		if ( m_options[ i ].m_hudPosition == position )
		{
			return i;
		}
	}
	return -1;
}

Bool CIDGraphBlockChoice::IsAnyOptionActive( CIDTopicInstance* topicInstance ) const
{
	for ( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		if ( IsOptionActive( topicInstance, i ) )
		{
			return true;
		}
	}
	return false;
}

#ifndef NO_EDITOR
	SIDOption* CIDGraphBlockChoice::FindOption( EHudChoicePosition position )
	{
		Int32 idx = FindOptionIndex( position );
		return idx < 0 ? NULL : &m_options[ idx ];
	}
#endif // ifndef NO_EDITOR