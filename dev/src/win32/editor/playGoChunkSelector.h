
#pragma once

#include "selectionEditor.h"

class CEdPlayGoChunkSelector : public CEdMappedSelectionEditor
{
public:
	CEdPlayGoChunkSelector( CPropertyItem* item );

private:
	virtual void FillMap( TDynArray< TPair< String, String > >& map ) override;
};
