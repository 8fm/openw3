#pragma once

#include "mappedClassSelectorDialog.h"

class CEdBehaviorTreeBlockTypeSelectorDialog : public CEdMappedClassSelectorDialog
{
public:
	CEdBehaviorTreeBlockTypeSelectorDialog( wxWindow* parent, CClassHierarchyMapper& hierarchy, CClass* rootClass = NULL );

protected:
	Bool IsSelectable( CClass* classId ) const override;
};

class CEdBehaviorTreeDecoratorBlockTypeSelectorDialog : public CEdBehaviorTreeBlockTypeSelectorDialog
{
public:
	CEdBehaviorTreeDecoratorBlockTypeSelectorDialog( wxWindow* parent, CClassHierarchyMapper& hierarchy, CClass* rootClass = NULL )
		: CEdBehaviorTreeBlockTypeSelectorDialog( parent, hierarchy, rootClass )	{}

protected:
	Bool IsSelectable( CClass* classId ) const override;

};
