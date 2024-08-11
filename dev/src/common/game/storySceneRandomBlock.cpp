#include "build.h"
#include "storySceneRandomBlock.h"
#include "storySceneGraphSocket.h"
#include "storyScene.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneRandomBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CStorySceneRandomBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_controlPart )
	{
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_controlPart, LSD_Input ) );

		const TDynArray< CStorySceneLinkElement* > & outputs = m_controlPart->GetOutputs();
		for ( Uint32 i = 0; i < outputs.Size(); ++i )
		{
			CreateSocket( StorySceneGraphSocketSpawnInfo( CName( TXT("Out ") + ToString(i) ), outputs[ i ], LSD_Output ) );
		}
	}
}

void CStorySceneRandomBlock::OnDestroyed()
{
	if ( m_controlPart != NULL )
	{
		m_controlPart->GetScene()->RemoveControlPart( m_controlPart );
	}
}
#endif

void CStorySceneRandomBlock::SetRandomPart( CStorySceneRandomizer* randomPart )
{
	ASSERT( ! m_controlPart );
	ASSERT( randomPart );
	m_controlPart = randomPart;
	//m_controlPart->SetParent( this );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}

