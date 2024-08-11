#pragma once

#include "itemSelectorDialogBase.h"

class CEdInventoryItemSelector : public CEdItemSelectorDialog< CName >
{
public:
	CEdInventoryItemSelector( wxWindow* parent, const CName& defaultSelected, INamesListOwner* parentCategory = NULL );

	~CEdInventoryItemSelector();

private:
	virtual void Populate() override;

	TDynArray< CName > m_options;
	CName m_defaultSelected;

	INamesListOwner* m_parentCategory;
};
