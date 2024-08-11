/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "rewardsSummary.h"
#include "rewardEditor.h"

BEGIN_EVENT_TABLE( CRewardsSummary, wxHtmlWindow )
END_EVENT_TABLE()

CRewardsSummary::CRewardsSummary( wxWindow* parent ) : wxHtmlWindow( parent )
{
	Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( CRewardsSummary::OnLinkClicked ), NULL, this );
}

void CRewardsSummary::CreateSummary( CRewardIterator& it )
{
	Uint32 totalRewards = 0;
	Uint32 totalGold = 0;
	Uint32 totalExp = 0;

	for( ; it; ++it )
	{
		++totalRewards;
		totalGold += (*it).m_gold;
		totalExp += (*it).m_experience;
	}

	wxString html;
	html = wxString::Format( wxT("Summary generated for a total of %i rewards.<br>Total gold: &nbsp %i<br>Total experience: &nbsp %i"), totalRewards, totalGold, totalExp );

	SetPage( html );
}

void CRewardsSummary::OnLinkClicked( wxHtmlLinkEvent& event )
{

}