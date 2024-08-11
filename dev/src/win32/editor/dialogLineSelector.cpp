// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogLineSelector.h"
#include "dialogEditor.h"
#include "../../common/game/storySceneLine.h"

// =================================================================================================
namespace {
// =================================================================================================

/*
Constructs text label representing specified line.

\param line Line for which to construct text label. May be nullptr.
\return Text label representing specified line.
*/
String GetLineLabel(CStorySceneLine* line)
{
	String label;

	if( line == nullptr )
	{
		label = TXT("None");
	}
	else
	{
		if( line->IsBackgroundLine() )
		{
			label = TXT( "(bg) " );
		}

		label += line->GetElementID();
		label += TXT( ": " );
		label += line->GetContent();
	}

	return label;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

/*
Ctor.
*/
CDialogLineSelector::CDialogLineSelector( CPropertyItem* propertyItem )
: ISelectionEditor( propertyItem )
{}

/*
Dtor.
*/
CDialogLineSelector::~CDialogLineSelector()
{}

Bool CDialogLineSelector::GrabValue( String& displayValue )
{
	CStorySceneLine* line = nullptr;
	m_propertyItem->Read( &line );

	displayValue = GetLineLabel( line );

	return true;
}

Bool CDialogLineSelector::SaveValue()
{
	CStorySceneLine* line = nullptr;

	// Get scene line associated with selected item.
	Int32 selection = m_ctrlChoice->GetSelection();
	if( selection >= 0 )
	{
		void* clientData = m_ctrlChoice->wxItemContainer::GetClientData( selection );
		line = static_cast< CStorySceneLine* >( clientData );
	}

	m_propertyItem->Write( &line );

	return true;
}

void CDialogLineSelector::FillChoices()
{
	// first item denotes no line
	m_ctrlChoice->Append( GetLineLabel( nullptr ).AsChar(), (void*)nullptr );

	// get section lines
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	const CStorySceneSection* section = sceneEditor->OnDialogLineSelector_GetCurrentSection();
	TDynArray< CAbstractStorySceneLine* > lines;
	section->GetLines( lines );

	// Create items for all lines. Put background lines before non-background lines.
	Uint32 numBackgroundLines = 0;
	for( auto it = lines.Begin(), end = lines.End(); it != end; ++it )
	{
		CStorySceneLine* line = Cast< CStorySceneLine >( *it );

		if( line )
		{
			String label = GetLineLabel( line );

			if( line->IsBackgroundLine() )
			{
				Uint32 pos = numBackgroundLines + 1; // + 1 because first entry is always none
				m_ctrlChoice->Insert( label.AsChar(), pos, line );
				++numBackgroundLines;
			}
			else
			{
				m_ctrlChoice->Append( label.AsChar(), line );
			}
		}
	}
}
