#include "build.h"
#include "storySceneInput.h"
#include "storySceneInputBlock.h"
#include "storySceneGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneInputBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CStorySceneInputBlock::GetBlockName() const
{
	return m_input ? m_input->GetName() : TXT("Input");
}

void CStorySceneInputBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_input )
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( Out ), m_input, LSD_Output ) );
}

#endif

void CStorySceneInputBlock::OnNameChanged( CStorySceneSection* sender )
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Just update layout of the section
	InvalidateLayout();
#endif
}

void CStorySceneInputBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Add listener to the bounded scene data
	if ( m_input )
	{
		m_input->AddListener( this );
		//m_input->SetParent( this );
	}
}

void CStorySceneInputBlock::SetInput( CStorySceneInput* input )
{ 
	ASSERT(!m_input);
	ASSERT(input);

	m_input = input;
	m_input ->AddListener( this );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	// Rebuild sockets
	OnRebuildSockets();
	InvalidateLayout();

#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CStorySceneInputBlock::GetTitleColor() const
{
	return Color::LIGHT_GREEN;
}

void CStorySceneInputBlock::OnDestroyed()
{
	if ( m_input != NULL )
	{
		m_input->GetScene()->RemoveControlPart( m_input );
	}
}
#endif