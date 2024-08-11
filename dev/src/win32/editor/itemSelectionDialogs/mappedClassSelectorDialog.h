#pragma once

#include "itemSelectorDialogBase.h"
#include "../classHierarchyMapper.h"


class CEdMappedClassSelectorDialog 
	: public CEdItemSelectorDialog< CClass >
{
public:
	CEdMappedClassSelectorDialog( wxWindow* parent, const CClassHierarchyMapper& hierarchy, const Char* configPath, CClass* rootClass, CClass* defaultSelected = NULL )
		: CEdItemSelectorDialog( parent, configPath )
		, m_hierarchy( hierarchy )
		, m_root( rootClass )
		, m_defaultSelected( defaultSelected )
	{}

	~CEdMappedClassSelectorDialog()
	{}

protected:
	virtual void Populate() override;

	virtual Bool IsSelectable( CClass* classId ) const;

	CClassHierarchyMapper m_hierarchy;
	CClass* m_root;
	CClass* m_defaultSelected;
};