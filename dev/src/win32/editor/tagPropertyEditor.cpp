/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "tagEditor.h"
#include "tagMiniEditor.h"
#include "tagPropertyEditor.h"
#include "tagListUpdater.h"

CTagPropertyEditor::CTagPropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
    , m_tagEditorExt( NULL )
    , m_ctrlText( NULL )
{
	ASSERT( propertyItem->GetPropertyType()->GetName() == TTypeName< TagList >::GetTypeName() );

	// Load icon
    m_iconDeleteTag = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_iconTagEditor = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
}

void CTagPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Calculate placement, hacked !
    wxRect valueRect;
	valueRect.y = propertyRect.y + 3;
	valueRect.height = propertyRect.height - 3;
	valueRect.x = propertyRect.x + 2;
	valueRect.width = propertyRect.width - propertyRect.height * 2 - 2;

	// Create text editor
    m_ctrlText = new wxTextCtrlEx
	(
		m_propertyItem->GetPage(),
		wxID_ANY,
		wxEmptyString,
		valueRect.GetTopLeft(),
		valueRect.GetSize(),
		(
			wxNO_BORDER |
			wxTE_PROCESS_ENTER
		)
	);

	m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlText->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_ctrlText->SetSelection( -1, -1 );
	m_ctrlText->SetFocus();

	outSpawnedControls.PushBack( m_ctrlText );

	if( m_propertyItem->GetProperty() && !m_propertyItem->GetProperty()->IsReadOnly() )
	{
		// Add button to spawn tag editor
		m_propertyItem->AddButton( m_iconTagEditor, wxCommandEventHandler( CTagPropertyEditor::OnSpawnTagEditorExt ), this );

		// Add button to delete tag editor
		m_propertyItem->AddButton( m_iconDeleteTag, wxCommandEventHandler( CTagPropertyEditor::OnDeleteTag ), this );
	}
}

void CTagPropertyEditor::CloseControls()
{
    if ( m_ctrlText )
	{
		m_ctrlText->Destroy();
		m_ctrlText = NULL;
	}
}

Bool CTagPropertyEditor::GrabValue( String& displayValue )
{
	// Read value
	TagList tagList;
	displayValue = TXT( "[ No tags ]" );
	if ( m_propertyItem->Read( &tagList ) )
	{
		displayValue = tagList.ToString();
	}

    if ( m_ctrlText )
	{
		Int32 caret = m_ctrlText->GetInsertionPoint();

		if( caret > 1 )
		{
			Char prev = m_ctrlText->GetValue()[ caret - 2 ];
			if( prev == L';' )
			{
				++caret;
			}
		}

        String editValue;
		
		if( tagList.GetTags().Size() > 0 )
		{
			editValue = displayValue.MidString( 2, displayValue.GetLength() - 4 ) + TXT( "; " );
		}

		m_ctrlText->ChangeValue( editValue.AsChar() );
		m_ctrlText->SetInsertionPoint( caret );
	}

	// Overridden
	return true;
}

Bool CTagPropertyEditor::SaveValue()
{
    if (m_ctrlText)
    {
        TDynArray<CName> tagsNames;

        String sTags = m_ctrlText->GetValue().wc_str();
        
        TDynArray<String> tags;
        String(sTags).Slice(tags, TXT(";"));
    
        TDynArray<String>::iterator tag_curr = tags.Begin(),
                                    tag_last = tags.End();
        for(; tag_curr != tag_last; ++tag_curr)
        {
            (*tag_curr).Trim();
            if (!tag_curr->Empty())
                tagsNames.PushBackUnique( CName( *tag_curr ) );
        }

		TagList tagList( tagsNames );
		for ( Int32 i=0; i<m_propertyItem->GetNumObjects(); ++i )
		{
			m_propertyItem->Write( &tagList, i );
		}
        m_propertyItem->GrabPropertyValue();
    }
	return true;
}

void CTagPropertyEditor::OnTagsSaved( wxCommandEvent &event )
{
	// Grab tags
	ASSERT( m_tagEditorExt );

	// Update tags
	TagList tagList;
	if ( m_propertyItem->Read( &tagList ) )
	{
		// Set tags
		tagList.SetTags( m_tagEditorExt->GetTags() );

		// Write value
		for ( Int32 i=0; i<m_propertyItem->GetNumObjects(); ++i )
		{
			m_propertyItem->Write( &tagList, i );
		}
		m_propertyItem->GrabPropertyValue();
	}

	// Unlink tag editor
	m_tagEditorExt = NULL;
}

void CTagPropertyEditor::OnTagsCanceledExt( wxCommandEvent &event )
{
	// Unlink tag editor
	ASSERT( m_tagEditorExt );
    m_tagEditorExt = NULL;
}

void CTagPropertyEditor::OnSpawnTagEditorExt( wxCommandEvent &event )
{
	if( !m_tagEditorExt )
	{
		TagList tagList;
		tagList.FromString( m_ctrlText->GetValue().wc_str() );

		// Open tag editor
		m_tagEditorExt = new CEdTagEditor( m_propertyItem->GetPage(), tagList );
		m_tagEditorExt->Bind( wxEVT_TAGEDITOR_OK, &CTagPropertyEditor::OnTagsSaved, this );
		m_tagEditorExt->Bind( wxEVT_TAGEDITOR_CANCEL, &CTagPropertyEditor::OnTagsCanceledExt, this );

		m_tagEditorExt->ShowModal();
	}
}

void CTagPropertyEditor::OnDeleteTag( wxCommandEvent &event )
{
    Int32 carretPos = m_ctrlText->GetInsertionPoint();
    String tags = m_ctrlText->GetValue().wc_str();

    String leftString = tags.LeftString(carretPos);
    size_t  leftPos;
	Bool leftFound = leftString.FindSubstring(TXT(";"), leftPos, true);
    
    if (!leftFound) 
	{
		leftPos = 0;
	}
    else            
	{
		leftPos += 1;
	}

    String rightString = tags.MidString(carretPos);

    size_t rightPos;
	Bool rightFound = rightString.FindSubstring(TXT(";"),rightPos, false);
    if (!rightFound) 
	{
		rightPos = rightString.GetLength();
	}
    else             
	{
		rightPos += 1;
	}

    String tag = leftString.MidString(leftPos) + rightString.LeftString(rightPos);
    tag.Trim();

    leftString  = leftString.LeftString(leftPos);
    rightString = rightString.MidString(rightPos);

    if (!rightFound && tag.Empty())
    {
        leftString = leftString.StringBefore(TXT(";"), true).StringBefore(TXT(";"), true);
        leftString.Trim();
        if (!leftString.Empty())
            leftString += TXT("; ");
    }
    else
        if (!leftString.Empty())
            leftString += TXT(" ");

    carretPos = leftString.GetLength();
    
    m_ctrlText->SetValue((leftString + rightString).AsChar());
    m_ctrlText->SetSelection(carretPos, carretPos);
    m_ctrlText->SetFocus();
}

