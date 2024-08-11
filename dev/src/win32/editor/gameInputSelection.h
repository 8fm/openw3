/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"


///////////////////////////////////////////////////////////////////////////////

class CGameInputSelection : public ISelectionEditor
{
public:
	CGameInputSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};

protected:
	virtual void FillChoices();
};

///////////////////////////////////////////////////////////////////////////////
