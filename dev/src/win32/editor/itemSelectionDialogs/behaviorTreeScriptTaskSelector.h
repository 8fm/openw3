#pragma once

#include "mappedClassSelectorDialog.h"

class CEdBehaviorTreeScriptTaskSelectorDialog : public CEdMappedClassSelectorDialog
{
public:
	CEdBehaviorTreeScriptTaskSelectorDialog( wxWindow* parent, CClassHierarchyMapper& hierarchy, IBehTreeTaskDefinition* defaultSelected = nullptr );

protected:
	virtual Bool IsSelectable( CClass* classId ) const override;
};
