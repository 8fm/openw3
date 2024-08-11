
#include "build.h"
#include "storySceneLinkHub.h"

#include "storySceneControlPartsUtil.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneLinkHub )

CStorySceneLinkHub::CStorySceneLinkHub() 
	: m_numSockets( 2 )
{
}

void CStorySceneLinkHub::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		CStorySceneControlPart* nextContolPart = StorySceneControlPartUtils::GetControlPartFromLink( GetNextElement() );
		if ( nextContolPart )
		{
			nextContolPart->CollectControlParts( controlParts, visitedControlParts );
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneLinkHub::OnPropertyPostChange( IProperty* property )
{
	m_numSockets = Clamp< Uint32 >( m_numSockets, 0, 10 );

	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
}

#endif

#ifndef NO_EDITOR

Bool CStorySceneLinkHub::SupportsInputSelection() const
{
	return true;
}

void CStorySceneLinkHub::ToggleSelectedInputLinkElement()
{
	++m_selectedLinkedElement;

	if ( m_selectedLinkedElement >= m_numSockets )
	{
		m_selectedLinkedElement = 0;
	}
}

#endif
