/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// List of texture groups
class CTextureGroupSelectionList : public CListSelection
{
public:
	CTextureGroupSelectionList( CPropertyItem* item );
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};