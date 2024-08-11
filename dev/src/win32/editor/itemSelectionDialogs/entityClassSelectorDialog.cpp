#include "build.h"

#include "entityClassSelectorDialog.h"
#include "../classHierarchyMapper.h"

CEdEntityClassSelectorDialog::CEdEntityClassSelectorDialog( wxWindow* parent, const CClass* defaultSelected )
	: CEdClassSelectorDialog( parent, ClassID< CEntity >(), defaultSelected, true, TXT( "/Frames/EntityClassSelectorDialog" ) )
{
}

CEdEntityClassSelectorDialog::~CEdEntityClassSelectorDialog()
{

}