/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Class list selection
class CAppearanceListSelection : public CListSelection
{
	CEntityTemplate * m_template;

public:
	CAppearanceListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};