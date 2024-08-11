// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneControlPartsUtil.h"
#include "storySceneLinkElement.h"
#include "storySceneControlPart.h"
#include "storySceneInput.h"
#include "storySceneLinkHub.h"
#include "storySceneSection.h"
#include "storySceneChoice.h"
#include "storySceneCutsceneSection.h"
#include "storySceneFlowCondition.h"
#include "storySceneFlowSwitch.h"
#include "storySceneRandomizer.h"
#include "storySceneScriptingBlock.h"

// =================================================================================================
namespace StorySceneControlPartUtils {
// =================================================================================================

/*
Returns control part from link.

Most of the time the link itself is a control part. However, the link may be a path that belongs
to multipath section in which case the link is not a control part and we need to take it's parent.
*/
const CStorySceneControlPart* GetControlPartFromLink( const CStorySceneLinkElement* link )
{
	const CStorySceneControlPart* controlPart = 0;

	if ( link )
	{
		controlPart = Cast< const CStorySceneControlPart >( link );

		// This handles the case in which link is one of paths in multipath section. Downcasting such link
		// to CStorySceneControlPart* gives us 0 because link is not a CStorySceneControlPart*. We need to
		// use link parent to get to "real" control part.
		if ( !controlPart && link->GetParent() )
		{
			controlPart = Cast< const CStorySceneControlPart>( link->GetParent() );
		}
	}

	return controlPart;
}

CStorySceneControlPart* GetControlPartFromLink( CStorySceneLinkElement* link )
{
	return const_cast< CStorySceneControlPart* >( GetControlPartFromLink( static_cast< const CStorySceneLinkElement* >( link ) ) );
}

void GetNextConnectedElements( const CStorySceneLinkElement* linkElem, TDynArray< const CStorySceneLinkElement* >& out )
{
	if( linkElem->IsA< CStorySceneSection >() )
	{
		const CStorySceneSection* section = static_cast< const CStorySceneSection* >( linkElem );
		for ( Uint32 j = 0; j < section->GetNumberOfInputPaths(); ++j )
		{
			CStorySceneLinkElement* nextLinkElement = section->GetInputPathLinkElement( j )->GetNextElement();
			if ( nextLinkElement )
			{
				out.PushBack( nextLinkElement );
			}
		}

		const CStorySceneChoice* choice = section->GetChoice();
		if( choice )
		{
			Uint32 numChoices = choice->GetNumberOfChoiceLines();
			for ( Uint32 cl=0; cl<numChoices; cl++ )
			{
				const CStorySceneChoiceLine* choiceLine = choice->GetChoiceLine( cl );
				if ( choiceLine->GetNextElement() )
				{
					out.PushBack( choiceLine->GetNextElement() );
				}
			}
		}
	}
	else if ( linkElem->IsA< CStorySceneFlowCondition >() )
	{
		const CStorySceneFlowCondition* flowCondition = static_cast< const CStorySceneFlowCondition* >( linkElem );
		CStorySceneLinkElement* trueLink = flowCondition->GetTrueLink();
		CStorySceneLinkElement* falseLink = flowCondition->GetFalseLink();
		if ( trueLink && trueLink->GetNextElement() )
		{
			out.PushBack( trueLink->GetNextElement() );
		}
		
		if ( falseLink && falseLink->GetNextElement() )
		{
			out.PushBack( falseLink->GetNextElement() );
		}
			
	}
	else if ( linkElem->IsA< CStorySceneFlowSwitch >() )
	{
		const CStorySceneFlowSwitch* flowSwitch = static_cast< const CStorySceneFlowSwitch* >( linkElem );
		const TDynArray< CStorySceneFlowSwitchCase* >& cases = flowSwitch->GetCases();

		for( TDynArray< CStorySceneFlowSwitchCase* >::const_iterator it = cases.Begin(), end = cases.End(); it != end; ++it )
		{
			if( (*it) && (*it)->m_thenLink && (*it)->m_thenLink->GetNextElement() )
			{
				out.PushBack( (*it)->m_thenLink->GetNextElement() );
			}
		}
	}
	else if ( linkElem->IsA< CStorySceneRandomizer >() )
	{
		const CStorySceneRandomizer* part = static_cast< const CStorySceneRandomizer* >( linkElem );

		const TDynArray< CStorySceneLinkElement* > & outputs = part->GetOutputs();
		for ( Uint32 i = 0; i < outputs.Size(); ++i )
		{
			CStorySceneLinkElement * output = outputs[ i ];
			if ( output )
			{
				out.PushBack( output );
			}
		}
	}
	else if ( linkElem->IsA< CStorySceneScript >() )
	{
		const CStorySceneScript* sceneScript = static_cast< const CStorySceneScript* >( linkElem );
		const TDynArray< CStorySceneLinkElement* >& links = sceneScript->GetOutLinks();
		for ( Uint32 i = 0; i < links.Size(); ++i )
		{
			out.PushBack( links[i]->GetNextElement() );
		}		
		CStorySceneLinkElement* defaultLink = sceneScript->GetNextElement();
		if ( defaultLink )
		{
			out.PushBack( defaultLink );
		}
	}
	else
	{
		CStorySceneLinkElement* nextLinkElement = linkElem->GetNextElement();

		if ( nextLinkElement )
		{
			out.PushBack( nextLinkElement );
		}

		if ( linkElem->GetParent() && linkElem->GetParent()->IsA< CStorySceneLinkElement >() )
		{
			CStorySceneLinkElement* parentLinkElement = static_cast< CStorySceneLinkElement* >( linkElem->GetParent() );
			out.PushBackUnique( parentLinkElement );
		}
	}
}

namespace
{
	void InternalGetControlPartsFromLink( CStorySceneLinkElement* link, Bool sectionWithInputs, TDynArray< const CStorySceneLinkElement*>& out )
	{
		CObject* parent = link->GetParent();
		if	( parent && 
			(	Cast< CStorySceneFlowCondition >( parent ) ||
			Cast< CStorySceneFlowSwitchCase >( parent ) ||
			Cast< CStorySceneScript >( parent ) ||
			Cast< CStorySceneRandomizer >( parent ) ||
			Cast< CStorySceneCutsceneSection >( parent ) ||
			( Cast< CStorySceneSection >( parent ) )
			) 
			)
		{
			link = Cast< CStorySceneLinkElement >( parent );
		}

		if ( parent && parent->GetParent() && Cast< CStorySceneChoice >( parent ) )
		{
			link = Cast< CStorySceneLinkElement >( parent->GetParent() );
		}

		if ( link )
		{
			SCENE_ASSERT( !link->IsExactlyA< CStorySceneLinkElement >() );
			SCENE_ASSERT( !link->IsExactlyA< CStorySceneControlPart >() );

			out.PushBack( link );
		}
	}
}

void GetPrevConnectedElements( const CStorySceneLinkElement* elem, TDynArray< const CStorySceneLinkElement*>& out, Int32 selectedInput )
{
	const TDynArray< CStorySceneLinkElement* >* prevElements = nullptr;

	Bool sectionWithInputs = false;
	const CStorySceneSection* section = Cast< const CStorySceneSection >( elem );
	if ( section && section->GetInputPathLinks().Size() > 0 )
	{
		prevElements = &(section->GetInputPathLinks());
		sectionWithInputs = true;
	}
	else
	{
		prevElements = &(elem->GetLinkedElements());
	}

	const Uint32 numPrevElements = prevElements->Size();

	if ( sectionWithInputs && selectedInput != -1 )
	{
		if ( CStorySceneLinkElement* link = (*prevElements)[selectedInput] )
		{
			const TDynArray< CStorySceneLinkElement* >& subInputs = link->GetLinkedElements();

			const Uint32 numSubInputs = subInputs.Size();
			for ( Uint32 j=0; j<numSubInputs; ++j )
			{
				if ( CStorySceneLinkElement* subLink = subInputs[ j ] )
				{
					InternalGetControlPartsFromLink( subLink, false, out );
				}
			}
		}		
	}

	for ( Uint32 i=0; i<numPrevElements; ++i )
	{
		if ( CStorySceneLinkElement* link = (*prevElements)[i] )
		{
			if ( sectionWithInputs && selectedInput == -1 )
			{
				const TDynArray< CStorySceneLinkElement* >& subInputs = link->GetLinkedElements();

				const Uint32 numSubInputs = subInputs.Size();
				for ( Uint32 j=0; j<numSubInputs; ++j )
				{
					if ( CStorySceneLinkElement* subLink = subInputs[ j ] )
					{
						InternalGetControlPartsFromLink( subLink, false, out );
					}
				}
			}
			else
			{
				InternalGetControlPartsFromLink( link, sectionWithInputs, out );
			}
		}
	}
}

void GetPrevConnectedElements( const CStorySceneLinkElement* elem, TDynArray< const CStorySceneLinkElement*>& out )
{
	GetPrevConnectedElements( elem, out, -1 );
}

/*
Evaluates scene graph node.

\param evalNode Node to evaluate. Must not be nullptr.
\return EvalSceneNodeResult object describing evaluation result. EvalSceneNodeResult::resultNode
will be nullptr if evalNode can't be evaluated or if it doesn't link to any other nodes.

Evaluation will fail when:
a) evalNode is a multi-path CStorySceneSection as there's no way to find out which path to follow.
   To go further, evaluate CStorySceneLinkElement node representing specific section path.
b) evalNode is a CStorySceneSection with choice as there's no way to find out which path to follow.
   To go further, evaluate CStorySceneChoiceLine node representing specific choice line.
c) evalNode is a CStorySceneScript as this case is not yet supported.

CStorySceneFlowCondition, CStorySceneFlowSwitch
Conditions associated with CStorySceneFlowCondition and CStorySceneFlowSwitch are evaluated to choose appropriate path
(note that it's ok if chosen path leads to nowhere - in this case resultNode will be nullptr). However, no conditions
are evaluated if evalNode is already a specific path of CStorySceneFlowCondition and CStorySceneFlowSwitch.

CStorySceneRandomizer
Evaluating CStorySceneRandomizer will result in random path being chosen. However, if evalNode is already a specific path
of CStorySceneRandomizer then that path is directly taken.

Note that resultNode may be CStorySceneLinkElement that represents specific path of multi-path section but it will never
be multi-path section itself. Use GetControlPartFromLink() to resolve resultNode representing path of multi-path section.
*/
EvalSceneNodeResult EvalSceneGraphNode( const CStorySceneLinkElement* evalNode )
{
	RED_FATAL_ASSERT( evalNode, "EvalSceneGraphNode(): evalNode must not be nullptr." );

	EvalSceneNodeResult result;
	result.argNode = evalNode;

	if( evalNode->IsA< CStorySceneSection >() )
	{
		const CStorySceneSection* section = static_cast< const CStorySceneSection* >( evalNode );

		if( section->GetNumberOfInputPaths() == 1 && !section->GetChoice() )
		{
			result.resultNode = section->GetNextElement();
			result.resolution = EvalSceneNodeResult::EResolution::R_Success;
		}
		else
		{
			// section has a choice or is a multi-path section - we don't know which link to follow
			result.resultNode = nullptr;
			result.resolution = EvalSceneNodeResult::EResolution::R_CantEvaluate;
		}
	}
	else if( evalNode->IsA< CStorySceneFlowCondition >() )
	{
		const CStorySceneFlowCondition* flowCond = static_cast< const CStorySceneFlowCondition* >( evalNode );

		result.resultNode = flowCond->IsFulfilled()? flowCond->GetTrueLink()->GetNextElement() : flowCond->GetFalseLink()->GetNextElement();
		result.resolution = EvalSceneNodeResult::EResolution::R_Success;
	}
	else if( evalNode->IsA< CStorySceneFlowSwitch >() )
	{
		const CStorySceneFlowSwitch* flowSwitch = static_cast< const CStorySceneFlowSwitch* >( evalNode );

		Int32 chosenPath = -1;
		result.resultNode = flowSwitch->ChoosePathToFollow( chosenPath )->GetNextElement();
		result.resolution = EvalSceneNodeResult::EResolution::R_Success;
	}
	else if( evalNode->IsA< CStorySceneRandomizer >() )
	{
		const CStorySceneRandomizer* randomizer = static_cast< const CStorySceneRandomizer* >( evalNode );

		if( !randomizer->GetOutputs().Empty() )
		{
			Int32 linkIndex = -1;
			result.resultNode = randomizer->GetRandomOutput( linkIndex )->GetNextElement();
		}
		else
		{
			result.resultNode = nullptr;
		}

		result.resolution = EvalSceneNodeResult::EResolution::R_Success;
	}
	else if( evalNode->IsA< CStorySceneScript >() )
	{
		// evaluating script nodes is not yet supported
		result.resultNode = nullptr;
		result.resolution = EvalSceneNodeResult::EResolution::R_CantEvaluate;
	}
	else if( evalNode->IsExactlyA< CStorySceneLinkElement >()
			 || evalNode->IsA< CStorySceneInput >()
			 || evalNode->IsA< CStorySceneOutput >()
			 || evalNode->IsA< CStorySceneLinkHub >()
			 || evalNode->IsA< CStorySceneChoiceLine >() )
	{
		result.resultNode = evalNode->GetNextElement();
		result.resolution = EvalSceneNodeResult::EResolution::R_Success;
	}
	else
	{
		RED_FATAL( "EvalSceneGraphNode(): encountered unknown CStorySceneLinkElement." );
	}

	return result;
}

// =================================================================================================
} // namespace StorySceneUtils
// =================================================================================================
