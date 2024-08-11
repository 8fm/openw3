#include "build.h"
#include "storySceneFlowSwitchBlock.h"

#include "storyScene.h"
#include "storySceneGraphSocket.h"
#include "storySceneFlowSwitch.h"
#include "storySceneLinkHub.h"
#include "questCondition.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneFlowSwitchBlock )

CStorySceneFlowSwitchBlock::CStorySceneFlowSwitchBlock()
	: m_switch( nullptr )
{

}

CStorySceneControlPart* CStorySceneFlowSwitchBlock::GetControlPart() const 
{
	return m_switch;
}

void CStorySceneFlowSwitchBlock::SetControlPart( CStorySceneControlPart* part )
{
	m_switch = Cast< CStorySceneFlowSwitch >( part );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneFlowSwitchBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_switch )
	{				
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_switch, LSD_Input ) );

		const TDynArray< CStorySceneFlowSwitchCase* >& cases = m_switch->GetCases();

		TDynArray< CStorySceneFlowSwitchCase* >::const_iterator it = cases.Begin();
		TDynArray< CStorySceneFlowSwitchCase* >::const_iterator end = cases.End();

		for(; it != end ;++it)
		{				
			if( (*it) != NULL && (*it)->m_whenCondition != NULL)
			{			
				LOG_GAME(TXT("Add socket"));			
				CreateSocket( StorySceneGraphSocketSpawnInfo( (*it)->m_whenCondition->GetName(), (*it)->m_thenLink, LSD_Output ) );
			}
		}
		String defaultName(TXT("default"));
		CreateSocket( StorySceneGraphSocketSpawnInfo( CName( defaultName ), m_switch->GetDefaultLink(), LSD_Output ) );
	}
}

EGraphBlockShape CStorySceneFlowSwitchBlock::GetBlockShape() const
{
	return GBS_Octagon;
}

Color CStorySceneFlowSwitchBlock::GetClientColor() const
{
	return Color( 76, 130, 191 );
}

String CStorySceneFlowSwitchBlock::GetCaption() const
{
	return m_description;
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneFlowSwitchBlock::OnPropertyPostChange( IProperty* property )
{
	//TBaseClass::OnPropertyPostChange( property );	
	OnRebuildSockets();	
}

void CStorySceneFlowSwitchBlock::OnDestroyed()
{
	if ( m_switch )
	{
		m_switch->GetScene()->RemoveControlPart( m_switch );
	}
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneLinkHubBlock )

CStorySceneLinkHubBlock::CStorySceneLinkHubBlock() 
	: m_hub( nullptr )
{

}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CStorySceneLinkHubBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_hub )
	{	
		for( Uint32 i = 0; i < m_hub->GetNumSockets(); i++ )
		{
			CreateSocket( StorySceneGraphSocketSpawnInfo( CName( String( TXT("In") ) + ToString( i ) ), m_hub, LSD_Input ) );
		}

		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( Out ), m_hub, LSD_Output ) );
	}
}

void CStorySceneLinkHubBlock::OnDestroyed()
{
	if ( m_hub )
	{
		m_hub->GetScene()->RemoveControlPart( m_hub );
	}
}

void CStorySceneLinkHubBlock::OnPropertyPostChange( IProperty* property )
{
	//TBaseClass::OnPropertyPostChange( property );	
	OnRebuildSockets();	
}
#endif

void CStorySceneLinkHubBlock::SetControlPart( CStorySceneControlPart* part )
{
	m_hub = Cast< CStorySceneLinkHub >( part );
}

CStorySceneControlPart* CStorySceneLinkHubBlock::GetControlPart() const
{
	return m_hub;
}
