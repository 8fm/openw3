#include "build.h"

#include "componentSelectorDialog.h"
#include "../classHierarchyMapper.h"

CEdComponentSelectorDialog::CEdComponentSelectorDialog( wxWindow* parent )
	:	CEdClassSelectorDialog( parent, ClassID< CComponent >(), nullptr, false, TXT( "/Frames/ComponentSelectorDialog" ), TXT( "Components" ) )
{
}

CEdComponentSelectorDialog::~CEdComponentSelectorDialog()
{
}