#pragma once

class CClass;

struct SPropertyRecurrentBrowserConf
{
	CClass* m_type;
	CClass* m_desiredParentType;
	Bool m_onlyEditable;
	TDynArray< const CClass* >* m_filterOut;
	TDynArray< const CObject* >* m_parents;
	TDynArray< void* >* m_instances;
	SPropertyRecurrentBrowserConf( CClass* type, TDynArray< const CObject* >* parents, TDynArray< void* >* instances, TDynArray< const CClass* >* filterOut );
};

class CPropertyRecurrentBrowser
{
	static void Find( Bool canUseObjProperty, const IRTTIType& objectClass, void* obj, CObject* parent, TSet< void* >& visited, const SPropertyRecurrentBrowserConf& conf );
public:
	static void Find( CObject* obj, const SPropertyRecurrentBrowserConf& conf );
};
