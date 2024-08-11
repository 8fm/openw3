#include "build.h"
#include "storySceneFlowCondition.h"
#include "storySceneControlPartsUtil.h"

#include "questCondition.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneFlowCondition )

CStorySceneFlowCondition::CStorySceneFlowCondition(void)
	: m_trueLink( NULL )
	, m_falseLink( NULL )
	, m_questCondition( NULL )
{
#ifndef NO_EDITOR
	m_trueLink = CreateObject< CStorySceneLinkElement >( this );
	m_falseLink = CreateObject< CStorySceneLinkElement >( this );
#endif
}

Bool CStorySceneFlowCondition::IsFulfilled() const
{
	if ( m_questCondition != NULL )
	{
		Bool questConditionResult = false;
		m_questCondition->Activate();
		questConditionResult = m_questCondition->IsFulfilled();
		m_questCondition->Deactivate();
		return questConditionResult;
	}
	return true;
}

void CStorySceneFlowCondition::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		CStorySceneControlPart* trueContolPart = StorySceneControlPartUtils::GetControlPartFromLink( m_trueLink->GetNextElement() );
		CStorySceneControlPart* falseControlPart = StorySceneControlPartUtils::GetControlPartFromLink( m_falseLink->GetNextElement() );

		if ( trueContolPart != NULL )
		{
			trueContolPart->CollectControlParts( controlParts, visitedControlParts );
		}
		if ( falseControlPart != NULL ) 
		{
			falseControlPart->CollectControlParts( controlParts, visitedControlParts );
		}
	}
}

void CStorySceneFlowCondition::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< CStorySceneLinkElement* >::iterator iter = m_linkedElements.Begin();
		iter != m_linkedElements.End(); ++iter )
	{
		(*iter)->OnConnected( linkedToElement );
	}
}

void CStorySceneFlowCondition::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< CStorySceneLinkElement* >::iterator iter = m_linkedElements.Begin();
		iter != m_linkedElements.End(); ++iter )
	{
		(*iter)->OnDisconnected( linkedToElement );
	}
}

void CStorySceneFlowCondition::InitializeWithDefaultCondition()
{
}

void CStorySceneFlowCondition::OnPostLoad()
{
	// Remove all broken links
	ValidateLinks();
}


#ifndef NO_EDITOR

Bool CStorySceneFlowCondition::SupportsOutputSelection() const
{
	return true;
}

void CStorySceneFlowCondition::ToggleSelectedOutputLinkElement()
{
	m_trueFalseEditor = !m_trueFalseEditor;
}

Uint32 CStorySceneFlowCondition::GetSelectedOutputLinkElement() const
{
	return m_trueFalseEditor ? 0 : 1;
}

#endif
