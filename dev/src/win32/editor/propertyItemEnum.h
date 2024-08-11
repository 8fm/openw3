/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemEnum : public CPropertyItem
{
public:
	CPropertyItemEnum( CEdPropertiesPage* page, CBasePropItem* parent );

	virtual void CreateMainControl() override;
};
