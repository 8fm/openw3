/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "baseGroupItem.h"
#include "../../common/engine/materialParameter.h"

/// Root property for material shader object
class CMaterialGroupItem : public CBaseGroupItem
{
protected:
	IMaterial*										m_material;			//!< Material
	THashMap< CPropertyItem*, CMaterialParameter* >	m_childrenParams;	//!< Children parameters
	CName m_paramGroup;

public:
	CMaterialGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, IMaterial* material, CName paramGroup = CName::NONE );

	Bool IsInlined() const;

	virtual void Expand() override;
	virtual void Collapse() override;
	virtual String GetCaption() const override;

	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
};
