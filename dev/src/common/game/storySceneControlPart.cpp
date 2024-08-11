#include "build.h"
#include "storySceneControlPart.h"
#include "storySceneScriptingBlock.h"
#include "storySceneCutsceneSection.h"
#include "storySceneRandomizer.h"
#include "storySceneFlowCondition.h"
#include "storySceneFlowSwitch.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "storyScene.h"
#include "storySceneDialogset.h"
#include "storySceneInput.h"
#include "storySceneControlPartsUtil.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneControlPart );

void CStorySceneControlPart::CollectControlParts( TDynArray< CStorySceneControlPart* >& controlParts )
{
	TDynArray< const CStorySceneControlPart* > visitedControlParts;
	CollectControlPartsImpl( controlParts, visitedControlParts );
}

void CStorySceneControlPart::CollectControlParts( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	CollectControlPartsImpl( controlParts, visitedControlParts );
}

void CStorySceneControlPart::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );
		controlParts.PushBackUnique( this );
	}
}

CStoryScene* CStorySceneControlPart::GetScene() const
{
	return Cast< CStoryScene >( FindParent< CStoryScene >() );
}

void CStorySceneControlPart::GetAllNextLinkedElements( TDynArray< const CStorySceneLinkElement* >& elements ) const
{
	TDynArray< const CStorySceneLinkElement* > stack;
	TDynArray< const CStorySceneLinkElement* > linkedElements;
	stack.PushBack(this);

	while( !stack.Empty() )
	{
		const CStorySceneLinkElement* link = stack.PopBack();
		if( link )
		{
			elements.PushBackUnique( link );
			linkedElements.ClearFast();
			StorySceneControlPartUtils::GetNextConnectedElements( link, linkedElements );

			for ( Int32 i = 0; i < linkedElements.SizeInt(); ++i )
			{
				if ( linkedElements[i] && !elements.Exist( linkedElements[i] ) )
				{
					stack.PushBack( linkedElements[i] );
				}
			}
		}
	}
}

void CStorySceneControlPart::ValidateLinks( SBrokenSceneGraphInfo* graphInfo /*= NULL*/ )
{
	// Make sure that tere are no empty links;
	while( m_linkedElements.Remove( NULL ) ) {}
	
	// Make sure that all ingoing links are properly linked
	for ( Uint32 i=0; i<m_linkedElements.Size(); i++ )
	{
		ASSERT( m_linkedElements[i] );

		if ( m_linkedElements[i] != NULL )
		{
			CStorySceneLinkElement* otherElement = m_linkedElements[i];
			ASSERT( otherElement->GetNextElement() == this );
#ifndef NO_EDITOR
			if ( otherElement->GetNextElement() != this )
			{
				if ( graphInfo )
				{
					CStorySceneElement* parent = Cast< CStorySceneElement >( otherElement->GetParent() );
					if ( parent )
					{
						if ( GetScene() )
						{
							graphInfo->m_path = GetScene()->GetFriendlyName();
						}
						CStorySceneElement* element = Cast< CStorySceneElement >( otherElement->GetParent() );
						if ( element )
						{
							CStorySceneSection* section = element->GetSection();
							if ( section )
							{
								graphInfo->m_brokenLinks.PushBack( TPair< String, String >( section->GetName(), GetName() ) );
							}
						}
					}
				}
			}
#endif //NO_EDITOR
		}
		else
		{
			continue;
		}
	}
}

