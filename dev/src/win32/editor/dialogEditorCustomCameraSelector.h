/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"

class CEdCustomCameraSelector : public ISelectionEditor									
{
public:
	CEdCustomCameraSelector( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};
