#include "build.h"
#include "storyScene.h"
#include "storySceneFlowSwitch.h"
#include "storySceneControlPartsUtil.h"

#include "questCondition.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneFlowSwitchCase )
IMPLEMENT_ENGINE_CLASS( CStorySceneFlowSwitch )

CStorySceneFlowSwitchCase :: CStorySceneFlowSwitchCase()
{	
	m_thenLink = CreateObject< CStorySceneLinkElement >( this );	
}

void CStorySceneFlowSwitchCase::OnPropertyPostChange( IProperty* property )
{	
	CObject* p = GetParent();
	if( p!= NULL )
	{
		p->OnPropertyPostChange( property );
	}
}

void CStorySceneFlowSwitch::NotifyAboutSocketsChange()
{
	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
}

CStorySceneFlowSwitch :: CStorySceneFlowSwitch()
{
	LOG_GAME(TXT("CStorySceneFlowSwitch"));
	m_defaultLink = CreateObject< CStorySceneLinkElement >( this );	
	LOG_GAME(TXT("CStorySceneFlowSwitch end"));
}
void CStorySceneFlowSwitch::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		for( TDynArray< CStorySceneFlowSwitchCase* >::iterator i = m_cases.Begin(); i != m_cases.End(); ++i)
		{
			if( (*i) != NULL && (*i)->m_thenLink != NULL )
			{
				CStorySceneControlPart* controlPart = StorySceneControlPartUtils::GetControlPartFromLink( (*i)->m_thenLink->GetNextElement() );
				if( (*i)->m_thenLink->GetNextElement() != NULL && controlPart != NULL )
				{
					controlPart->CollectControlParts( controlParts, visitedControlParts );
				}
			}
		}
		if( m_defaultLink )
		{
			CStorySceneControlPart* controlPart = StorySceneControlPartUtils::GetControlPartFromLink( m_defaultLink->GetNextElement() );
			if( controlPart )
			{
				controlPart->CollectControlParts( controlParts, visitedControlParts );
			}
		}
	}
}

void CStorySceneFlowSwitch::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< CStorySceneLinkElement* >::iterator iter = m_linkedElements.Begin();
		iter != m_linkedElements.End(); ++iter )
	{
		(*iter)->OnConnected( linkedToElement );
	}
}

void CStorySceneFlowSwitch::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< CStorySceneLinkElement* >::iterator iter = m_linkedElements.Begin();
		iter != m_linkedElements.End(); ++iter )
	{
		(*iter)->OnDisconnected( linkedToElement );
	}
}

void CStorySceneFlowSwitch::OnPropertyPostChange( IProperty* property )
{	
	NotifyAboutSocketsChange();	
}



CStorySceneLinkElement* CStorySceneFlowSwitch::ChoosePathToFollow( Int32& chosenPath ) const
{
	chosenPath = 0;

	for( TDynArray< CStorySceneFlowSwitchCase* >::const_iterator it = m_cases.Begin(), end = m_cases.End(); it != end ; ++it, ++chosenPath )
	{//change to asserts
		if((*it) == NULL)
		{
			LOG_GAME(TXT("it is null"));
		}
		if((*it)->m_thenLink == NULL)
		{
			LOG_GAME(TXT("thenLink is null"));
		}
		if((*it)->m_whenCondition == NULL)
		{
			LOG_GAME(TXT("whenCondition is null"));
		}

		if( (*it) != NULL && (*it)->m_thenLink != NULL && (*it)->m_whenCondition != NULL )
		{
			Bool questConditionResult = false;
			(*it)->m_whenCondition->Activate();
			questConditionResult = (*it)->m_whenCondition->IsFulfilled();
			(*it)->m_whenCondition->Deactivate();
			if(questConditionResult)
			{
				return (*it)->m_thenLink;
			}
		}
	}

	chosenPath = -1;
	return m_defaultLink;
}

