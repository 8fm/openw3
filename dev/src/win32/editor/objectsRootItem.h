/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CObjectsRootItem : public CPropertyItemClass
{
protected:
	TDynArray< STypedObject >	m_objects;
	CClass*						m_baseClass;

public:
	CObjectsRootItem( CEdPropertiesPage* page, CBasePropItem* parent );
	~CObjectsRootItem();

	void SetObjects( const TDynArray< STypedObject > &objects  );

	virtual void DrawLayout( wxDC& dc );
	virtual void UpdateLayout( Int32& yOffset, Int32 x, Int32 width );
	virtual Int32 GetLocalIndent() const;

public:

	virtual void Expand();

	virtual Int32 GetNumObjects() const;

	virtual STypedObject GetRootObject( Int32 objectIndex ) const override;
	virtual STypedObject GetParentObject( Int32 objectIndex ) const override;

	virtual Bool IsReadOnly() const;
	virtual Bool IsInlined() const;
	virtual String GetName() const;
	virtual String GetCustomEditorType() const;
};
