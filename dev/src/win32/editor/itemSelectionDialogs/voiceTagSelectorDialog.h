#pragma once

#include "itemSelectorDialogBase.h"

class CEdVoiceTagSelectorDialog : public CEdItemSelectorDialog< CName >
{
public:
	CEdVoiceTagSelectorDialog( wxWindow* parent, const CName& defaultSelected );
	~CEdVoiceTagSelectorDialog();

private:
	virtual void Populate() override;

	CName m_defaultSelected;
	TDynArray< CName > m_tags;
};
