/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneExporter.h"

#include "../../../common/game/storySceneInput.h"
#include "../../../common/game/storySceneFlowCondition.h"
#include "../../../common/game/storySceneFlowSwitch.h"
#include "../../../common/game/storySceneSection.h"
#include "../../../common/game/storySceneLine.h"
#include "../../../common/game/storySceneComment.h"
#include "../../../common/game/storySceneControlPartsUtil.h"
#include "../../../common/game/storySceneChoice.h"

// =================================================================================================
namespace {
// =================================================================================================

// functions in this namespace
void GetSceneElements( const CStoryScene* storyScene, TDynArray< const CStorySceneLinkElement* >& outSceneElements );
void ReorderSectionsLeadingBackToSourceChoice( TDynArray< const CStorySceneLinkElement* >& sceneElements );
void ReorderSectionsDevelopingTopic( TDynArray< const CStorySceneLinkElement* >& sceneElements );
void ReorderChoiceAfterSection( TDynArray< const CStorySceneLinkElement* >& sceneElements, const CStorySceneSection* section, CStorySceneSection* choiceSection );

/*
Gets scene elements.

\param outSceneElements out Container to which to store scene elements. It's cleared before use.
*/
void GetSceneElements( const CStoryScene* storyScene, TDynArray< const CStorySceneLinkElement* >& outSceneElements )
{
	ASSERT( storyScene );

	outSceneElements.ClearFast();

	TDynArray< CStorySceneInput* > sceneInputs;
	storyScene->CollectControlParts( sceneInputs );

	for ( Uint32 i = 0; i < sceneInputs.Size(); ++i )
	{
		sceneInputs[ i ]->GetAllNextLinkedElements( outSceneElements );
	}

	TDynArray< CStorySceneSection* > sections;
	storyScene->CollectControlParts( sections );

	for ( auto it = sections.Begin(), end = sections.End(); it != end; ++it )
	{
		outSceneElements.PushBackUnique( *it );
	}

	// find first sections
	const Uint32 numInputs = sceneInputs.Size();
	TDynArray< CStorySceneControlPart* > firstSections;
	for ( Uint32 i = 0; i < numInputs; ++i )
	{
		CStorySceneControlPart* nextContolPart = StorySceneControlPartUtils::GetControlPartFromLink( sceneInputs[ i ]->GetNextElement() );
		if ( nextContolPart != NULL )
		{
			nextContolPart->CollectControlParts( firstSections );
		}
	}

	// remove all inputs so we can later put them in correct order
	for ( Uint32 i = 0; i < numInputs; ++i )
	{
		Int32 index = outSceneElements.GetIndex( sceneInputs[ i ] );

		// this would be strange if input that we haven't yet removed didn't exist in sceneElements
		ASSERT( index != -1 );

		outSceneElements.RemoveAt( index );
	}

	// remove all first sections so we can later put them in correct order
	const Uint32 numFirstSections = firstSections.Size();
	for ( Uint32 i = 0; i < numFirstSections; ++i )
	{
		Int32 index = outSceneElements.GetIndex( firstSections[ i ] );

		// this would be strage if first section we haven't yet removed didn't exist in sceneElements
		ASSERT( index != -1 );

		outSceneElements.RemoveAt( index );
	}

	// insert inputs at the beginning of sceneElements
	for ( Uint32 i = 0; i < numInputs; ++i )
	{
		outSceneElements.Insert( i, sceneInputs[ i ] );
	}

	// insert first sections after inputs in sceneElements
	for ( Uint32 i = 0; i < numFirstSections; ++i )
	{
		outSceneElements.Insert( numInputs + i, firstSections[ i ] );
	}

	ReorderSectionsLeadingBackToSourceChoice( outSceneElements );
	ReorderSectionsDevelopingTopic( outSceneElements );
}

/*
Makes sections that lead back to source choice-section come before sections that progress the scene.

E.g.:
                 -> section A1 -> choice-section A
                 -> section A2 -> choice-section A
choice-section A -> section A3 -> section X
                 -> section A4 -> section Y
                 -> section A5 -> choice-sectionA

The function will reorder scene elements "section A1", "section A2" and "section A5" - they will be put
right after scene element "choice-section A". Positions of scene elements "section A3" and "section A4"
will not be changed.
*/
void ReorderSectionsLeadingBackToSourceChoice( TDynArray< const CStorySceneLinkElement* >& sceneElements )
{
	// for each scene element
	const Uint32 numSceneElements = sceneElements.Size();
	for( Uint32 i = 0; i < numSceneElements; ++i )
	{
		// if scene element is a section with a choice
		const CStorySceneSection* section = Cast< const CStorySceneSection >( sceneElements[ i ] );
		if( section && section->GetChoice() )
		{
			const CStorySceneChoice* choice = section->GetChoice();
			const Uint32 numChoiceLines = choice->GetNumberOfChoiceLines();

			// get list of leading back sections
			TDynArray< CStorySceneSection* > sectionsLeadingBack;
			for( Uint32 j = 0; j < numChoiceLines; ++j )
			{
				const CStorySceneChoiceLine* choiceLine = choice->GetChoiceLine( j );

				// if choice line leads to an element that is a section which leads back to section owing the choice
				CStorySceneSection* nextSection = Cast< CStorySceneSection >( choiceLine->GetNextElement() );
				if( nextSection && nextSection->GetNextElement() == section )
				{
					// Note we use PushBackUnique() - this is because two choice lines may point to the same section.
					sectionsLeadingBack.PushBackUnique( nextSection );
				}
			}

			// reorder leading back sections
			for( Uint32 k = 0; k < sectionsLeadingBack.Size(); ++k )
			{
				CStorySceneSection* sectionToReorder = sectionsLeadingBack[ k ];
				Uint32 currentPosition = sceneElements.GetIndex( sectionToReorder );
				Uint32 desiredPosition = i + k + 1;

				ASSERT( ( currentPosition < i ) || ( currentPosition >= desiredPosition ) );

				// put section leading back in desired position
				sceneElements.RemoveAt( currentPosition );
				if( currentPosition < i )
				{
					--desiredPosition;
					--i;
				}
				sceneElements.Insert( desiredPosition, sectionToReorder );
			}
		}
	}
}

/*
Makes sections developing given topic appear right after section that introduces the topic.

E.g.:
                                                    -> section B1
                  -> section A1 -> choice-section B -> section B2
                                                    -> section B3
choice-section A
                                                    -> section C1
                  -> section A2 -> choice-section C -> section C2
                                                    -> section C3

Sections B1, B2 and B3 probably develop on the topic discussed in A1 so we want them to be right after A1.
Likewise, we want C1, C2 and C3 to be right after A2. Of course, this works for any number of choices.

Of course, the function has no way to check whether some sections really develop on given topic or not.
It tries to guess this by recognizing specific scene graph structure. No sensible results are guaranteed ;P

After reordering, elements will be in following order:

choice-section A

section A1
choice-section B
section B1
section B2
section B3

section A2
choice-section C
section C1
section C2
section C3
*/
void ReorderSectionsDevelopingTopic( TDynArray< const CStorySceneLinkElement* >& sceneElements )
{
	// Each time we find graph structure that we are looking for, we need to remember its root.
	// This is to prevent infinite loops moving two choice-sections back and forth (this would
	// be the case if any of sections B1, B2, B3, C1, C2, C3 led back to choice-section A).
	TDynArray< const CStorySceneSection* > roots;

	// for each scene element
	const Uint32 numSceneElements = sceneElements.Size();
	for( Uint32 i = 0; i < numSceneElements; ++i )
	{
		// if scene element is a section with a choice
		const CStorySceneSection* choiceSectionA = Cast< const CStorySceneSection >( sceneElements[ i ] );
		if( choiceSectionA && choiceSectionA->GetChoice() )
		{
			// for each choice line
			const Uint32 numChoiceLinesA = choiceSectionA->GetChoice()->GetNumberOfChoiceLines();
			for( Uint32 a = 0; a < numChoiceLinesA; ++a )
			{
				// if choice line leads to an element that is a section
				const CStorySceneSection* sectionAx = Cast< const CStorySceneSection >( choiceSectionA->GetChoice()->GetChoiceLine( a )->GetNextElement() );
				if( sectionAx )
				{
					// and that section leads to an element that is a another section with a choice (but not the one we're
					// coming from or one that was already identified as a root of graph structure we're looking for)
					CStorySceneSection* choiceSectionB = Cast< CStorySceneSection >( sectionAx->GetNextElement() );
					if( choiceSectionB && choiceSectionB->GetChoice() && choiceSectionB != choiceSectionA && !roots.Exist( choiceSectionB ) )
					{
						// Move choice section b and elements to which it leads - all of them should be right after sectionAx
						ReorderChoiceAfterSection( sceneElements, sectionAx, choiceSectionB );

						// choiceSectionA is a root of graph structure we were looking for
						roots.PushBack( choiceSectionA );
					}
				}
			}

			// After reordering, position of choice-section A might have changed.
			// We have to find it again to continue with the next element.
			i = sceneElements.GetIndex( choiceSectionA );
		}
	}
}

/*
Reorders elements so that specified choice-section and sections issuing directly from it appear right after specified section.
*/
void ReorderChoiceAfterSection( TDynArray< const CStorySceneLinkElement* >& sceneElements, const CStorySceneSection* section, CStorySceneSection* choiceSection )
{
	ASSERT( section );
	ASSERT( choiceSection );

	TDynArray< CStorySceneLinkElement* > elementsToReorder;

	// first element that is to be reordered is the choice itself
	elementsToReorder.PushBack( choiceSection );

	// gather rest of elements that are to be reordered
	const Uint32 numChoiceLines = choiceSection->GetChoice()->GetNumberOfChoiceLines();
	for( Uint32 i = 0; i < numChoiceLines; ++i )
	{
		CStorySceneLinkElement* nextSection = Cast< CStorySceneSection >( choiceSection->GetChoice()->GetChoiceLine( i )->GetNextElement() );
		if( nextSection )
		{
			elementsToReorder.PushBackUnique( nextSection ); // PushBackUnique() - becase two choice lines may point to the same section
		}
	}

	// get section index
	Int32 sectionIndex = sceneElements.GetIndex( section );
	ASSERT( sectionIndex != -1 );

	// reorder elements
	for( Uint32 i = 0, numElementsToReorder = elementsToReorder.Size(); i < numElementsToReorder; ++i )
	{
		CStorySceneLinkElement* elementToReorder = elementsToReorder[ i ];

		Int32 currentPosition = sceneElements.GetIndex( elementToReorder );
		ASSERT( currentPosition != -1 );

		Int32 desiredPosition = sectionIndex + i + 1;

		ASSERT( ( currentPosition < sectionIndex ) || ( currentPosition >= desiredPosition ) );

		sceneElements.RemoveAt( currentPosition );
		if( currentPosition < sectionIndex )
		{
			--desiredPosition;
			--sectionIndex;
		}
		// else
			// currentPostion >= desiredPosition so there's no need to adjust desiredPosition or sectionIndex
		sceneElements.Insert( desiredPosition, elementToReorder );
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

CStorySceneExporter::CStorySceneExporter()
{}

void CStorySceneExporter::ExportScene( CStoryScene* storyScene )
{
	if ( storyScene == NULL )
	{
		return;
	}

	if ( IsBatchedExport() == false )
	{
		DoBeginExport();
	}

	TDynArray< const CStorySceneLinkElement* > sceneElements;
	GetSceneElements( storyScene, sceneElements );

	const Uint32 numSceneElements = sceneElements.Size();
	for ( Uint32 j = 0; j < numSceneElements; ++j )
	{
		const CStorySceneFlowCondition* flowCondition = Cast< const CStorySceneFlowCondition >( sceneElements[ j ] );
		if ( flowCondition != NULL )
		{
			ExportFlowCondition( storyScene, flowCondition );
		}

		const CStorySceneFlowSwitch* flowSwitch = Cast< const CStorySceneFlowSwitch >( sceneElements[ j ] );
		if ( flowSwitch != NULL )
		{
			ExportFlowSwitch( storyScene, flowSwitch );
		}

		const CStorySceneSection* section = Cast< const CStorySceneSection >( sceneElements[ j ] );
		if ( section != NULL )
		{
			ExportSceneSection( storyScene, section );
		}
	}

	if ( IsBatchedExport() == false )
	{
		DoEndExport();
	}
}

void CStorySceneExporter::ExportSceneSection( CStoryScene* storyScene, const CStorySceneSection* section )
{
	DoExportSceneSectionHeader( section, storyScene );

	const Uint32 numElements = section->GetNumberOfElements();

	// Get index of last element that will be exported. We need to know this because this tells us when
	// to export links to following control parts. This is relevant only for sections without a choice.
	Uint32 lastExportedElementIndex = -1;
	if( !section->GetChoice() )
	{
		for ( Uint32 i = 0; i < numElements; ++i )
		{
			Uint32 index = numElements - 1 - i;
			const CStorySceneElement* element = section->GetElement( index );

			// Only CStorySceneLine and CStorySceneComment elements are exported so we check only them.
			if( Cast< CStorySceneLine >( element ) || Cast< CStorySceneComment >(element) )
			{
				lastExportedElementIndex = index;
				break;
			}
		}
	}

	for ( Uint32 i = 0; i < numElements; ++i )
	{
		const CStorySceneElement* element = section->GetElement( i );
		Bool exportLinksToFollowingControlParts = ( i == lastExportedElementIndex );

		if ( const CStorySceneLine* sceneLine = Cast< CStorySceneLine >( element ) )
		{
			DoExportStorySceneLine( sceneLine, storyScene, section, exportLinksToFollowingControlParts );
		}
		else if ( const CStorySceneComment* sceneComment = Cast<const  CStorySceneComment >( element ) )
		{
			DoExportStorySceneComment(sceneComment, storyScene, exportLinksToFollowingControlParts );
		}
	}

	const CStorySceneChoice* sectionChoice = section->GetChoice();
	if ( sectionChoice != NULL )
	{
		DoExportStorySceneChoice(sectionChoice, storyScene);
	}

	DoExportEmptyLine();
}
