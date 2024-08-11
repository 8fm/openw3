#pragma once

#include "itemSelectorDialogBase.h"

class CEdInventoryCategorySelector : public CEdItemSelectorDialog< CName >
{
public:
	CEdInventoryCategorySelector( wxWindow* parent, const CName& defaultSelected );
	~CEdInventoryCategorySelector();

private:
	virtual void Populate() override;

	TDynArray< CName > m_categories;
	CName m_defaultSelected;
};
