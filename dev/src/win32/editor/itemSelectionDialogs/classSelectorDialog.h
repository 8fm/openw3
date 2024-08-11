/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "itemSelectorDialogBase.h"

class CEdClassSelectorDialog : public CEdItemSelectorDialog< CClass >
{
public:
	CEdClassSelectorDialog( 
		wxWindow* parent, const CClass* baseClass, const CClass* defaultClass, Bool showRoot, 
		const String& configPath = TXT( "/Frames/ClassSelectorDialog" ),
		const String& title      = TXT( "Class selector" )
		);

	~CEdClassSelectorDialog();

protected:
	virtual void Populate() override;
	
private:
	Bool IsSelectedByDefault( const CClass* c ) const;

	const CClass*	m_baseClass;
	const CClass*	m_defaultClass;
	Bool			m_showRoot;
};
