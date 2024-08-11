/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"


///////////////////////////////////////////////////////////////////////////////

class CEdAchievementSelection : public ISelectionEditor									
{
public:
	CEdAchievementSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};

protected:
	virtual void FillChoices();
};