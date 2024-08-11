// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneSectionVariant.h"
#include "storySceneElement.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneSectionVariantElementInfo )
IMPLEMENT_ENGINE_CLASS( CStorySceneSectionVariant  );

Float CStorySceneSectionVariant::GetApprovedDuration( const String& elementId ) const
{
	Float approvedDuration = -1.0f;
	Bool found = m_elementIdToApprovedDuration.Find( elementId, approvedDuration );
	RED_FATAL_ASSERT( found, "CStorySceneSectionVariant::GetApprovedDuration() - element doesn't belong to this seciton variant.");
	return approvedDuration;
}

/*
Inserts element.

\param element Element to insert.
\param index Index at which to insert element. To insert element at the end, pass index == -1 or index == number of elements.

Duration of element is not calculated - element will not have its duration approved.
*/
void CStorySceneSectionVariant::InsertElement( const CStorySceneElement* element, Uint32 index /* = -1 */ )
{
	RED_FATAL_ASSERT( !m_elementIdToApprovedDuration.KeyExist( element->GetElementID() ), "CStorySceneSectionVariant::AddElement(): element with specified id already exists in this variant." );

	CStorySceneSectionVariantElementInfo elementInfo;
	elementInfo.m_elementId = element->GetElementID();
	elementInfo.m_approvedDuration = -1.0f;

	m_elementInfo.Insert( index == -1? m_elementInfo.Size() : index, elementInfo );
	m_elementIdToApprovedDuration.Insert( elementInfo.m_elementId, elementInfo.m_approvedDuration );
}

void CStorySceneSectionVariant::RemoveElement( const String& elementId )
{
	Uint32 elementIndex = -1;
	for( Uint32 iElement = 0, numElements = m_elementInfo.Size(); iElement < numElements; ++iElement )
	{
		if( m_elementInfo[ iElement ].m_elementId == elementId )
		{
			elementIndex = iElement;
			break;
		}
	}

	RED_FATAL_ASSERT( elementIndex != -1, "CStorySceneSectionVariant::RemoveElement(): element with specified id doesn't exist in this variant." );
	RED_FATAL_ASSERT( m_elementIdToApprovedDuration.KeyExist( elementId ), "CStorySceneSectionVariant::RemoveElement(): element with specified id doesn't exist in this variant." );

	m_elementInfo.RemoveAt( elementIndex );
	m_elementIdToApprovedDuration.Erase( elementId );
}
