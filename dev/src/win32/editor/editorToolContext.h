/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdEditorToolContext
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
protected:
	Color m_edTerrainCurrentColor;
public:
	Color& edTerrainCurrentColor(){ return m_edTerrainCurrentColor; }

	void setEdTerrainCurrentColor( Color &c ){ m_edTerrainCurrentColor = c; }

	CEdEditorToolContext();
};
