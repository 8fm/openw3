/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CClassGroupItem : public CBaseGroupItem
{
protected:
	CClass*		m_class;

public:
	CClassGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CClass* classFilter );

	virtual String GetCaption() const;
	virtual void Expand();
	virtual Bool IsClassGroupItem() const override { return true; }

	inline CClass* GetGroupClass() const { return m_class; }
};
