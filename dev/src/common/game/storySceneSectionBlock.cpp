#include "build.h"
#include "storySceneSectionBlock.h"
#include "storySceneSection.h"
#include "storySceneChoice.h"

#include "storySceneGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneSectionBlock )

CStorySceneSectionBlock::CStorySceneSectionBlock(void)
	: m_section( NULL )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CStorySceneSectionBlock::GetBlockName() const
{
	// Use the section name
	return m_section ? m_section->GetName() : TXT("Section");
}

Color CStorySceneSectionBlock::GetClientColor() const
{
	if( m_section && m_section->IsGameplay() )
	{
		return Color( 73, 122, 84 );
	}
	else if ( m_section && m_section->GetChoice() )
	{
		CStorySceneChoice* choice = m_section->GetChoice();
		for ( Uint32 i = 0; i < choice->GetNumberOfChoiceLines(); ++i )
		{
			CStorySceneChoiceLine* choiceLine = choice->GetChoiceLine( i );
			if ( choiceLine->HasMemo() )
			{
				return  Color( 255, 188, 102 );
			}
		}
	}
	return TBaseClass::GetClientColor();
}

void CStorySceneSectionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Build layout only for valid sections
	if ( m_section == NULL )
	{
		return;
	}

	if ( m_section->GetNumberOfInputPaths() != 1 )
	{
		for ( Uint32 j = 0; j < m_section->GetNumberOfInputPaths(); ++j )
		{
			String inputName = String::Printf( TXT( "In %d" ), j + 1 );
			String outputName = String::Printf( TXT( "Out %d" ), j + 1 );

			CreateSocket( StorySceneGraphSocketSpawnInfo( CName( inputName ), m_section->GetInputPathLinkElement( j ), LSD_Input ) );
			CreateSocket( StorySceneGraphSocketSpawnInfo( CName( outputName ), m_section->GetInputPathLinkElement( j ), LSD_Output ) );
		}
	}
	else
	{
		// Every section always has an input and output sockets
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_section, LSD_Input ) );
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( Out ), m_section, LSD_Output ) );

		// Sections with choices also have outputs representing the choice options
		CStorySceneChoice* choice = Cast<CStorySceneChoice>( m_section->GetChoice() );
		if ( choice )
		{
			// Add a socket for each choice option
			const Uint32 numChoices = choice->GetNumberOfChoiceLines();
			for ( Uint32 i=0; i<numChoices; ++i )
			{
				CStorySceneChoiceLine* line = choice->GetChoiceLine( i );


				String choiceLine = line->GetChoiceLine();
				ReplaceUnicodeCharsWithAscii( choiceLine.TypedData(), choiceLine.GetLength() );

				CreateSocket( StorySceneGraphSocketSpawnInfo( CName( choiceLine ), line, LSD_Output ) );	 // WTF? Why such socket name?
			}
		}
	}

	if ( m_section->HasInterception() )
	{
		StorySceneGraphSocketSpawnInfo interceptSocketSpawnInfo( CNAME( Intercept ), NULL, LSD_Variable );
		interceptSocketSpawnInfo.m_placement = LSP_Bottom;
		interceptSocketSpawnInfo.m_isMultiLink = true;

		CreateSocket( interceptSocketSpawnInfo );
	}
}

#endif

void CStorySceneSectionBlock::SetSection( CStorySceneSection* newValue )
{
	ASSERT( !m_section );
	ASSERT( newValue );

	// Change section
	m_section = newValue;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Rebuild sockets
	OnRebuildSockets();
	InvalidateLayout();
#endif
}

void CStorySceneSectionBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Add listener to the bounded scene data
	if ( m_section != NULL )
	{
		// Add listener to the section
		//m_section->SetParent( this );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneSectionBlock::OnChoiceLineChanged( CStorySceneChoiceLine* choiceLine )
{
	// Update name of the related socket
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CStorySceneGraphSocket* socket = Cast< CStorySceneGraphSocket >( m_sockets[i] );
		if ( socket )
		{
			CStorySceneLinkElement* element = socket->GetLinkElement();
			if ( element == choiceLine )
			{
				String choiceLineStr = choiceLine->GetChoiceLine();
				ReplaceUnicodeCharsWithAscii( choiceLineStr.TypedData(), choiceLineStr.GetLength() );

				// Update the choice caption
				socket->SetName( CName( choiceLineStr ) );
			}
		}
	}

	// Redraw block
	InvalidateLayout();
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneSectionBlock::OnDestroyed()
{
	TBaseClass::OnDestroyed();

	m_section->GetScene()->RemoveSection( m_section );
}

void CStorySceneSectionBlock::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	CStoryScene* scene = FindParent< CStoryScene >();
	if ( scene == NULL )
	{
		return ;
	}

	if ( m_section != NULL )
	{
		scene->AddSectionAtPosition( m_section, -1 );
	}
}

/*

\param wasCopied True when dealing with copy-paste operation. False - when dealing with cut-paste operation.
*/
void CStorySceneSectionBlock::OnPasted( Bool wasCopied )
{
	TBaseClass::OnPasted( wasCopied );

	if ( m_section == NULL )
	{
		return;
	}
	
	CStoryScene* scene = FindParent< CStoryScene >();
	if ( scene == NULL )
	{
		return ;
	}

	m_section->SetParent( scene );
	m_section->SetScenesElementsAsCopy( wasCopied );
	
	// If we're dealing with copy-paste operation then we have to make copies unique.
	if ( wasCopied == true )
	{
		m_section->SetName( m_section->GetName() + TXT( "(copy)" ) );
		m_section->MakeUniqueElementsCopies();
	}
	// else we're dealing with cut-paste operation and there's no need to do anything special
}

#endif
