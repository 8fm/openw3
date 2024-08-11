#pragma once

#include "classSelectorDialog.h"

class CEdEntityClassSelectorDialog : public CEdClassSelectorDialog
{
public:
	CEdEntityClassSelectorDialog( wxWindow* parent, const CClass* defaultSelected );
	~CEdEntityClassSelectorDialog();
};
