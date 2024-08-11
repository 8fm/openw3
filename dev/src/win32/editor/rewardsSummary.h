/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CRewardIterator;

class CRewardsSummary : public wxHtmlWindow
{
	DECLARE_EVENT_TABLE();

public:
	CRewardsSummary( wxWindow* parent );

	void CreateSummary( CRewardIterator& it );

protected:
	void OnLinkClicked( wxHtmlLinkEvent& event );
};