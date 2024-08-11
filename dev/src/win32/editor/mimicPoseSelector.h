/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "selectionEditor.h"

class CMimicComponent;

class CEdSceneMimicPoseSelector : public ISelectionEditor
{
protected:
	THandle< CMimicComponent >	m_headComponent;
	Bool						m_filterSelection;
	
public:
	CEdSceneMimicPoseSelector( CPropertyItem* item, Bool filterSelection );

	virtual void FillChoices();
};
