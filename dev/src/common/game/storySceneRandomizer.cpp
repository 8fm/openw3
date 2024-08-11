#include "build.h"
#include "storySceneRandomizer.h"
#include "storyScene.h"
#include "storySceneSystem.h"
#include "storySceneControlPartsUtil.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneRandomizer )

RED_DEFINE_STATIC_NAME( outputs );

CStorySceneRandomizer::CStorySceneRandomizer()
{
#ifndef NO_EDITOR
	// Init two outputs
	m_outputs.PushBack( CreateObject< CStorySceneLinkElement >( this ) );
	m_outputs.PushBack( CreateObject< CStorySceneLinkElement >( this ) );
#endif
}

void CStorySceneRandomizer::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		for ( Uint32 i = 0; i < m_outputs.Size(); ++i )
		{
			const CStorySceneLinkElement * link = m_outputs[ i ];
			ASSERT( link != NULL );

			CStorySceneControlPart* nextPart = StorySceneControlPartUtils::GetControlPartFromLink( link->GetNextElement() );
			if ( nextPart != NULL )
			{
				nextPart->CollectControlParts( controlParts, visitedControlParts );
			}
		}
	}
}

CStorySceneLinkElement* CStorySceneRandomizer::GetRandomOutput( Int32& _outputindex) const
{
	Uint32 randomizerId = 0;
	CStoryScene* parentScene = GetScene();
	if ( parentScene != NULL )
	{
		randomizerId = parentScene->GetSceneIdNumber();
	}

	Uint8 lastValue = 255;
	if ( GCommonGame != NULL && GCommonGame->GetSystem< CStorySceneSystem >() != NULL )
	{
		lastValue = GCommonGame->GetSystem< CStorySceneSystem >()->GetLastRandomizerValue( randomizerId );
	}
	
	Uint8 outputIndex = 0; 

	if( m_outputs.Size() > 1 )
	{
		outputIndex = GEngine->GetRandomNumberGenerator().Get< Uint8 >( static_cast< Uint8 >( m_outputs.Size() ) );
		if( outputIndex == lastValue )
		{
			outputIndex = ( outputIndex + 1 ) % m_outputs.Size();
		}
	}	

	if ( GCommonGame != NULL && GCommonGame->GetSystem< CStorySceneSystem >() != NULL )
	{
		GCommonGame->GetSystem< CStorySceneSystem >()->SetLastRandomizerValue( randomizerId, outputIndex );
	}

	_outputindex = (Int32)outputIndex;

	return m_outputs[ outputIndex ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneRandomizer::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == CNAME( outputs ) )
	{
		for ( Uint32 i = 0; i < m_outputs.Size(); ++i )
		{
			if ( m_outputs[ i ] == NULL )
			{
				m_outputs[ i ] = CreateObject< CStorySceneLinkElement >( this );
			}
		}

		EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
	}
}

#endif