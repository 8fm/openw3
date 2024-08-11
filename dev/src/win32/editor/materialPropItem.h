/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Property item for material parameter
class CMaterialParameterItem : public CPropertyItem
{
protected:
	CMaterialParameter*		m_parameter;		//!< Parameter

public:
	//! Constructor
	CMaterialParameterItem( CEdPropertiesPage* page, CBasePropItem* parent );

	void Init( CMaterialParameter *param );

	//! Get parameter
	RED_INLINE CMaterialParameter* GetParameter() const { return m_parameter; }

	//! Get caption
	virtual String GetCaption() const;
};
