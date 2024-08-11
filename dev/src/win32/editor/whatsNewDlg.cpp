/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/webview.h>
#include "whatsNewDlg.h"
#include "..\versionControl\versionControlP4.h"
#include "../../common/core/configFileManager.h"
#define MAIN_THREAD_CHECK() ASSERT( SIsMainThread(), TXT("Perforce operation should be done only from the main thread") )

#ifdef _DEBUG
#pragma comment(lib, "wxmsw29ud_webview")
#else
#pragma comment(lib, "wxmsw29u_webview")
#endif

// Closing the WhatsNew dialog
BEGIN_EVENT_TABLE( CEdWhatsNewDlg, wxDialog )
	EVT_BUTTON( XRCID("m_closeButton"), CEdWhatsNewDlg::OnClose )
	EVT_BUTTON( XRCID("m_moreButton"), CEdWhatsNewDlg::OnMore )
	EVT_CLOSE( CEdWhatsNewDlg::OnClosed )
END_EVENT_TABLE()

CEdWhatsNewDlg::CEdWhatsNewDlg( wxWindow* parent, Bool forceShow )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("WhatsNewDlg") );
	
	wxPanel* webViewContainer	= XRCCTRL( *this, "WebViewContainer", wxPanel );
	webViewContainer->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	
	// Information goes here
	m_whatsNewDisplay = wxWebView::New( webViewContainer, wxID_ANY );
	webViewContainer->GetSizer()->Add( m_whatsNewDisplay, 1, wxEXPAND );

	m_amountOfChangeListToShow = 10;
	m_showText = false;
	CSourceControlP4* p4 = dynamic_cast< CSourceControlP4* >( GVersionControl );
	if( p4 )
	{
		if(!p4->GetLastLocalChangelistNumber( m_updatedToChangeList ))
		{
			m_updatedToChangeList = 0;
		}
		else
		{
			// Getting the value from the ini file for the last cl number
			// If there is none we say it is 467428
			// After that we update it to m_updatedToChangeList
			if( GetChangeListNumber( m_fromChangeList ) )
			{
				SetChangeListNumber( m_updatedToChangeList );		
				SUserConfigurationManager::GetInstance().SaveAll();
			}
			else
			{
				SetChangeListNumber( 467428 );
				SUserConfigurationManager::GetInstance().SaveAll();
				m_fromChangeList = 467428;
			}
			if( forceShow )
			{
				m_fromChangeList = 467428;
			}
			
			AddNewInformationToWebView();
		}
	}

	// Checking the value of show again checkbox
	Bool enabled;
	if( this->GetDontShowAgainValue( enabled ) )
	{
		XRCCTRL( *this, "m_dontShowCheckBox", wxCheckBox )->SetValue( enabled );
	}

	// Resize to fit the contents.
	this->SetSize( 400, 500 );
	wxString clNumber = wxString::Format( wxT("%i"), m_updatedToChangeList );
	
	this->SetTitle( this->GetTitle() + TXT2(APP_VERSION_MAJOR) + TXT(".") + TXT2(APP_VERSION_MINOR) + TXT(".") + clNumber );
	
	CenterOnParent();
	//AddNewInformationToWebView();
}

String CEdWhatsNewDlg::BuildWebString( String prefix, String description, String user, String workspace )
{
	// A list of icons depending on the prefix 
	String iconString;
	if(prefix == TXT("!F"))
	{
		iconString = TXT("\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABmJLR0QAvgC+AL61rUCbAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3QoDDToC+Q4+GAAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAACYklEQVQ4y5WTT0hUURTGv/PemzfPGU1z8F/SIhciFEJhSKspdBGFDrVuWYThqpUGSSCBLtpJENRQu5hySAShKChJi1ADtUaNyn9vpsZJmnFmct57954Ww6BFVn6bA+ec33fOhXuAvyg1AhX/kLIDWND51AgO/Zb7fyWHsZQcxtNdQeZgPiZCOJN5fzmTeXsutRaCHwBWHuzCKPoQYSv+2M7FhmxzEKGd+rTP9+EB0CotpIVAjhlZYtS5atqbSWaYIFivDvjn7gy12QIRAEWKCkPV4WXGpOY48DIQLNnf4lUNX1pYmU12srqn0u8jbQ8zJBm+pgpP9cIAQ82qqtuQTtZIr8+XsJRHNFtgzc6h07D5Xnl9oIytuCRXuaroFYBrr0IAjLJGWXmwvBZiQ3LuK9ZXx1y2La8Q4RMV3jIxgK7axrPXfAdOEZGuwNinkFZKAMBOkpGLSZF8I1OLYcVcNO8evoQLAKBMB/MGTZ3oi06FbyTmQ5oUG4C1BrbiYCsOWAnIzDxSK0/02JIZtjLoAIDpIEAAMH0LaLyYN5q9rcfqjnVUqXqZIK2YAAKLtOR0RPk4OUjxuKOd6AYXGA3Ygl9dh0vR3NWaY25KOwrBpBMpTMSCOC0UtdjwGN9rAEQLzC9f2dFx2uWtgGRBycSKNjP+yJwZC8dT30wNcJNRWoUfNtp2vAUhEJBCwfLcbDYy9fpmdNVq+BKzGiITL4PLH97lQAbgIAAAo/15hrYbvOjDAgHLloWe1h6Mb68968Vxd5HaKxxx1N8F448bEKGZgfYCPNq/NanlKp47tjhJhPrtzE8vdhlon5U6egAAAABJRU5ErkJggg==\"");
	}
	else if(prefix == TXT("!B"))
	{
		iconString = TXT("\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAAHeSURBVDhPnZNdSFNhGMd/53ByZ37kKAnbAotdCGaEQV8YBUFehgy68CKQukyI7oKoiyC6LhAkkCgrKqyLEkYSSYFLYVGOWBiam1tO57Z2tnXmXJ5T50TD6YxT/8vnff8fz/s8r6D/Av+JQV8PolXuWp/+kU7crg7rAoIglLye+nuw2STamk6C0cK/wPvumt431qyr+bxJs9yCYf8y0Euy9ja7ay5il2UzkWWBN4E7FBwPUCJ1HGntLrVjSeBt8AmZ+keEYj48e4fK3t0UCIbHmY74Kw7EHxwmZR8gvezFpV+mscFZLhCKfiKk3mNKGSSeWCw7/Dg1ypzUy5L0iuTMLjoPXV1nIj6bPIVScwut4QWvp6+grfy+k0hFCWt3KcpjzIaznHA/rJhQ3L/9EsrCMknlA5t3Bng8foF0Ks6cPsRKtZeFxThbC2docR+oLHC4pQtn8TrfchKzUR+1zvfMpAJ8UW4QiUVIzzfhOdi34cIKf/5C//BpFPk51TYVR/0OstmvJDIi7Vvuc7TNg6ZpiOL6oZUqZzsGINfK98ImMmqMXFGiSm03yQZWr/LqOKUERnEpr3NzZA9a1Wf44aJ73yiN25zGum8oUJZJtgucPz6Bw3aM5rpzJvlv7sbZTxMI6cNVncPTAAAAAElFTkSuQmCC\"");
	}
	else if(prefix == TXT("!T"))
	{
		iconString = TXT("\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3QoDDTITW2eU4gAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAACcklEQVQ4y5VSz0sUcRT/vJlZZ2ZH0NmwtZQOnQTbSwYeOgTRIYiQ/oBORbUSeVu2DSJckFVJcRPXVkbo1CU8efBQ2qmTkQezQyotuD9gV1hwf7gz853XodYkLetzejz4/HiPD/AXWJYl4wRIfyA2x7uWZV34bffvsCwrY1nW2/9KkJpNAQDS6blbzNzB7PWn03NXAGAmlTpZIPwgDABwXed2i6rqLS1qq+s6gwAwGA4fEaAXyaQfRNeYucLwGuyhRkTnFZ8v2d3VdRYAstls0XGcO8z8BYAOQCMig5k/Kh6zwczziiS1kuSrssR113V9Znv76UAgAACoVqsd2Wx2WlGUmiSRRpB04YlWABcVT5KKbNsPG8CrYDAYME0ThmHAMAz4/X4GAFVVEQwGz9XrdVQqFeRyOQghYrIsbVPzlkQiEZUV5Vmot1ft6u5mv99PsvyjBkII7O/v8+7uLn1aW/OqlT0rFntyDwCk8fFxAEA0Gk3YjcbzzxsbXKvVSNM0aJoGXdeh6zqYmdbX11Eul99A0CAAjI6OHrgffDUej+cXFxe5UChwqVTiUqnExWKRV1ZWeHh4WMzPzhAAjIyMAACUn+4AgMfRqI+ZO03ThG3bvLq6SoqiIBQKcVtbGzGztL1TOAMgF4vFfgk0wUQ3PCFg2zYWFhY4n8/vAFA2v2529l3qAxHBde2bAF4eWyRPiAHHdbG8vFzOZDLTjuP0OK7Ts7W9Nbe0tLQnhIAQYgAAIpHIMQmYL9u2/U4I8XRycvJDcz+VnLqf+ZZ5LctynIiuAsDY2NhRASLqB9CYmJioHXYZejQEAO8jkch1AKcOc74DmZUo31vAXXoAAAAASUVORK5CYII=\"");
	}
	else if(prefix == TXT("!D"))
	{
		iconString = TXT("\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsIAAA7CARUoSoAAAALJSURBVDhPVVNNSFRRFP7ufW/GcXJ0tL9NuQhDhbISWmWbCoN+cJWLsiwoMLAoRME21qqwstQCaxNpBJFthGoptPMnFEMoTGgh+Zszb/yZn3fvO533Rke9M8O7c+653/nO970DWluKn3NtVTTf+4DstZjjOOvH3jPyuY1mHp+j2GIkExfuLm5FsfL6AgLT41ChIKw9J1F44xUENtbCp4fIGuoCaQfxwG4E735FqGAnRJLLzbaUY1tyHkiugMiEP5CFSNEZ7K1Lg8z0NCF7uBtK2xDCgGkKWEYI+S1jEFPNhyi4PAVHE5Qp4dd+KFqF8EnESy/Cl1cA49tzSMFQ0p+mRIDBf5eUD1LvKIeKRpDiBH9KIMUfQT44K0Du0g+YE/2QSsCmLKYPLiQg3Dasf1DFpzmX16+ntQgNdcPM2wVbKJhMXEDDUQ44g8u5EYPb4z3HRSwK60g19jd9SAO44YnWq8gZfguRm8+XmCUHXdYu3TRrxQETWI7AKruEkuZ3nj4ewLrWf7oaIPufQQZzMhc9ME4wSMJJLGPp6GWUNrzJ2CM3OQWSnKQImnvlNvnHdbkNV+AU11F8YDjG5isbDCZf3obxpRM6HOCiW3C9Cw4DZkkCWXFEj9WgrLnHi3st/H5xB6KvHQiz0mwlcVXhCpD+enoQb7TU8DOQE7ORqKjBgXs9MK4VOvdVbyucsAklTbYsBYcZCC6ZiLCpcQUZYCBHsueunQImu2JMjmB2+i/E4PkQhewlyGwfG8TokhOUhlrUMKsbWcAE9MdOiO0+ZpPW22OXtBE3w5DlfTFQ0WGkmJbbKKU0kgsa+mwtiutbUVLfAVV5BWrOZjH53BV3xcaqyMW+93Nuf0S2JhqsyqPvJ0BjFaCfHTcz07Y+j+NPbtHocdDIKdBAVT5Z0Vj6FVrPTCZsGr9+kEYf1W0Z4c0TPdHeSAOVuVvG/T/AsspfE+jVHgAAAABJRU5ErkJggg==\"");
	}
	
	String baseString = TXT("<div class=\"entry\"><left><img width=\"16\" height=\"16\" title=\"\" alt=\"\" src=");
	String endString = TXT("/>")+description.StringAfter(prefix)+TXT("</left><br>");
	String userString = TXT("<small style='display: block; text-align: right; margin-top: 2px'>") + user + TXT(" <font color='#505050'>(") + workspace + TXT(")</font></small></div>");

	String returnString =  baseString+iconString+endString+userString;
	return returnString;
}

// From Change List to Another Change List.
void CEdWhatsNewDlg::AddNewInformationToWebView( )
{
	if ( m_updatedToChangeList == 0 )
	{
		wxString newWebString = "YOU NEED TO BE CONNECTED TO PERFORCE TO SEE WHATS NEW.";
		m_whatsNewDisplay->SetPage( (newWebString), wxEmptyString );
		m_showText = true;
		return;
	}

	Uint32 theLastCL = m_fromChangeList;
	TDynArray< ChangelistDescription >	m_list;
	Uint32 ctr = 0;

	m_list.Resize( m_amountOfChangeListToShow );
	Float progressUpdateAmount = 100.0f/(Float)m_amountOfChangeListToShow;

	String webString = TXT("<!DOCTYPE html><html><head><title></title><style>html,body { font-family: helvetica; font-size: 13px; padding: 0px; margin: 0px; } .entry { border-bottom: 1px solid gray; padding: 5px } .entry:hover { background: #F8F8F8 } </style></head><body>");
	String prefix = TXT("");

	TDynArray< ChangeDescription > listOfChangelists;
	
	// Array of Strings to look for
	const UINT32 textListSize = 4;
	String textToLookFor[textListSize] = {TXT("!F"),TXT("!B"),TXT("!T"),TXT("!D")};
	// Array of Paths to look in
	const UINT32 pathListSize = 2;
	String pathsToLookIn[pathListSize] = {TXT("//Red_engine/Main.Lava/dev/..."),TXT("//Red_engine/Main.Lava/bin/...")};

	CSourceControlP4* p4 = dynamic_cast< CSourceControlP4* >( GVersionControl );
	if( p4 )
	{
		// Checking the paths
		for( UINT32 p=0; p<pathListSize; p++ )
		{
			// Getting all the descriptions of the specified path and from CL to CL
			// Getting from where you last updated from.
			if(p4->GetChangelists( pathsToLookIn[p], theLastCL, m_updatedToChangeList, listOfChangelists ) )
			{
				// Nothing to do here just getting the data into the listOfChangeLists
			}
		}
		for( UINT32 i=0; i<listOfChangelists.Size(); ++i )
		{
			// Don't show more than what the user asked for
			if( m_list.Size()>ctr )
			{
				// Slice the description
				TDynArray< String > parts;
				listOfChangelists[i].m_desc.Slice( parts, TXT("\n") );

				// For each input we check if the new line starts with ! if not it will be discarded.
				for( UINT32 l=0; l<parts.Size(); l++ )
				{
					// Going through the lists of prefixes and only shows the one that we are interested in
					for( UINT32 t=0; t<textListSize; t++ )
					{
						if( parts[l].BeginsWith( textToLookFor[t] ) )
						{
							String description = parts[l];
							String user = listOfChangelists[i-1].m_user;
							String workspace = listOfChangelists[i-1].m_client;

							// Building the actual webstring to show in the UI
							webString += this->BuildWebString( textToLookFor[t], description, user, workspace );
							
							ctr++;
							// Update the progress bar
							XRCCTRL( *this, "m_progress", wxGauge )->SetValue( ctr*progressUpdateAmount );
							XRCCTRL( *this, "m_progress", wxGauge )->Refresh();
							XRCCTRL( *this, "m_progress", wxGauge )->Update();

							m_showText = true;
							break;
						}	
					}
				}
			}
			else
			{
				break;	
			}
		}
		webString += TXT("</body></html>");
		
		wxString newWebString( webString.AsChar(), wxConvUTF8 );
		m_whatsNewDisplay->SetPage( (newWebString), wxEmptyString );

		// Set the progress bar to 100% here
		XRCCTRL( *this, "m_progress", wxGauge )->SetValue( 100 );
		XRCCTRL( *this, "m_progress", wxGauge )->Refresh();
		XRCCTRL( *this, "m_progress", wxGauge )->Update();
	}
}

void CEdWhatsNewDlg::OnMore( wxCommandEvent &event )
{
	m_amountOfChangeListToShow += 10;
	m_fromChangeList = 467428;
	this->AddNewInformationToWebView();
}

Bool CEdWhatsNewDlg::GetDontShowAgainValue( Bool &enabled )
{
	return SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("DontShowAgain"), enabled );
}

Bool CEdWhatsNewDlg::SetDontShowAgainValue( Bool enabled )
{
	return SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("DontShowAgain"), enabled );
}

Bool CEdWhatsNewDlg::GetChangeListNumber( UINT32 &CLNumber )
{
	return SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("ChangeListNumber"), CLNumber );
}

Bool CEdWhatsNewDlg::SetChangeListNumber( UINT32 CLNumber )
{
	return SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("ChangeListNumber"), CLNumber );
}

void CEdWhatsNewDlg::OnClose( wxCommandEvent &event )
{
	Bool checked = XRCCTRL( *this, "m_dontShowCheckBox", wxCheckBox )->GetValue();
	SetDontShowAgainValue( checked );
	SUserConfigurationManager::GetInstance().SaveAll();
	Close();
}

void CEdWhatsNewDlg::OnClosed( wxCloseEvent &event )
{
	Bool checked = XRCCTRL( *this, "m_dontShowCheckBox", wxCheckBox )->GetValue();
	SetDontShowAgainValue( checked );
	SUserConfigurationManager::GetInstance().SaveAll();
	DestroyLater( this );
}
