#include "build.h"
#include "dialogSectionTitle.h"


Bool CEdDialogSectionTitle::CustomArrowTraverseRule( wxKeyEvent &event )
{
	return  event.GetKeyCode() == WXK_RETURN;
}
