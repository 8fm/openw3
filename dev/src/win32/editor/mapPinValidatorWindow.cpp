#include "build.h"
#include "mapPinValidatorWindow.h"

#include "../../common/core/depot.h"

BEGIN_EVENT_TABLE( CEdMapPinValidatorWindow, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("beginSearching"), CEdMapPinValidatorWindow::OnBeginSearching )
	EVT_BUTTON( XRCID("showDetails"), CEdMapPinValidatorWindow::OnShowDetails )
	EVT_LISTBOX_DCLICK( XRCID("invalidList"), CEdMapPinValidatorWindow::OnItemDoubleClicked )
END_EVENT_TABLE()

CEdMapPinValidatorWindow::CEdMapPinValidatorWindow( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("MapPinValidator"), true )
	, m_invalidList( XRCCTRL( *this, "invalidList", wxListBox ) )
	, m_foundText( XRCCTRL( *this, "foundText", wxStaticText ) )
{
}

CEdMapPinValidatorWindow::~CEdMapPinValidatorWindow()
{
	ClearData();
}

void CEdMapPinValidatorWindow::OnBeginSearching( wxCommandEvent& event )
{
	ClearData();

	// find all spawnsets
	TDynArray< CDiskFile* > foundFiles;
	GDepot->Search( TXT(".w2comm"), foundFiles );

	Uint32 filesFound = 0;
	Uint32 problemsFound = 0;

	// analyze them one by one, memorizing those that contain the phase name we're looking for
	for ( TDynArray< CDiskFile* >::iterator it = foundFiles.Begin(); it != foundFiles.End(); ++it )
	{
		CDiskFile* file = *it;
		if( file->GetDepotPath().BeginsWith( TXT("junk") ) )
		{
			continue;
		}

		if ( !file->IsLoaded() )
		{
			ResourceLoadingContext context;
			file->Load( context );
		}

		CCommunity* community = Cast< CCommunity >( file->GetResource() );
		if ( community )
		{
			Bool fileAdded = false;

			TDynArray< CSTableEntry >& commTable = community->GetCommunityTable();

			for ( TDynArray< CSTableEntry >::iterator it = commTable.Begin(); it != commTable.End(); ++it )
			{
				CSTableEntry& tableEntry = *it;

				for ( TDynArray< CSStoryPhaseEntry >::iterator jt = tableEntry.m_storyPhases.Begin(); jt != tableEntry.m_storyPhases.End(); ++jt )
				{
					CSStoryPhaseEntry& storyPhase = *jt;

					if( !storyPhase.m_startInAP )
					{
						if ( storyPhase.m_spawnPointTags.Empty() )
						{
							if( !fileAdded )
							{
								m_invalidList->Append( file->GetDepotPath().AsChar() );
								++filesFound;
								fileAdded = true;
							}

							++problemsFound;
						}
						else if( storyPhase.m_cachedMapPinPosition == Vector::ZEROS )
						{
							if( !fileAdded )
							{
								m_invalidList->Append( file->GetDepotPath().AsChar() );
								++filesFound;
								fileAdded = true;
							}

							++problemsFound;
						}
					}
				}
			}
		}
	}

	m_foundText->SetLabel( wxString::Format( _T("%i in %i files."), problemsFound, filesFound ) );
}

void CEdMapPinValidatorWindow::OnItemDoubleClicked( wxCommandEvent& event )
{
	wxTheFrame->GetAssetBrowser()->OpenFile( m_invalidList->GetStringSelection().t_str() );
}

void CEdMapPinValidatorWindow::OnShowDetails( wxCommandEvent& event )
{
	wxMessageBox( wxT("Not yet implemented, sorry.") );
}

void CEdMapPinValidatorWindow::ClearData()
{
	if( m_invalidList->HasClientUntypedData() )
	{
		for( Uint32 i = 0; i < m_invalidList->GetCount(); ++i )
		{
			void* data = m_invalidList->GetClientData( i );
			if( data )
			{
				delete data;
				data = NULL;
				m_invalidList->SetClientData( i, NULL );
			}
		}
	}

	m_invalidList->Clear();
	m_foundText->SetLabel( wxT("0") );
}