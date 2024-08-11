
#pragma once

#include "selectionEditor.h"

class CEdLanguageSelector : public CEdMappedSelectionEditor
{
public:
	enum EType
	{
		eText,
		eSpeech
	};

public:
	CEdLanguageSelector( CPropertyItem* item, EType type );

private:
	virtual void FillMap( TDynArray< TPair< String, String > >& map ) override;

private:
	EType m_type;
};
