/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "itemSelectionDialogs/itemSelectorDialogBase.h"

class CEdAudioEventSelectorDialog : public CEdItemSelectorDialog< String >
{
public:
	CEdAudioEventSelectorDialog( wxWindow* parent, TDynArray< String >* events, const String& selectedEvent );
	~CEdAudioEventSelectorDialog();

private:
	virtual void Populate() override;

private:
	String					m_selectedEvent;
	TDynArray< String >*	m_events;
};

