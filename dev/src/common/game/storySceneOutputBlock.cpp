#include "build.h"
#include "storySceneOutputBlock.h"
#include "storyScene.h"
#include "storySceneGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneOutputBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CStorySceneOutputBlock::GetBlockName() const
{
	if ( m_output != NULL )
	{
		return m_output->GetName();
	}
	return TXT( "Output" );
}

void CStorySceneOutputBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_output, LSD_Input ) );
}

Color CStorySceneOutputBlock::GetTitleColor() const
{
	if ( m_output )
	{
		if ( m_output->ShouldEndWithBlackscreen() && m_output->GetGameplayCameraBlendTime() > 0.0f )
		{
			// something wrong
			return Color( 255, 70, 70 );
		}
	}
	return Color::LIGHT_RED;
}

//! Get client color
Color CStorySceneOutputBlock::GetClientColor() const
{
	if ( m_output )
	{
		if ( m_output->ShouldEndWithBlackscreen() && m_output->GetGameplayCameraBlendTime() > 0.0f )
		{
			// something wrong
			return Color( 255, 70, 70 );
		}
		else if ( m_output->ShouldEndWithBlackscreen() )
		{
			return Color::BLACK;
		}
		else if ( m_output->GetGameplayCameraBlendTime() > 0.0f )
		{
			return Color( 73, 122, 84 );
		}
	}
	return TBaseClass::GetClientColor();
}

void CStorySceneOutputBlock::OnDestroyed()
{
	if ( m_output != NULL )
	{
		m_output->GetScene()->RemoveControlPart( m_output );
	}
}
#endif

void CStorySceneOutputBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	if ( m_output != NULL )
	{
		//m_output->SetParent( this );
	}
}

