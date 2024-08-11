#include "build.h"
#include "behaviorTreeScriptTaskSelector.h"

#include "../../../common/game/behTreeTask.h"

CEdBehaviorTreeScriptTaskSelectorDialog::CEdBehaviorTreeScriptTaskSelectorDialog( wxWindow* parent, CClassHierarchyMapper& hierarchy, IBehTreeTaskDefinition* defaultSelected )
	: CEdMappedClassSelectorDialog( parent, hierarchy, TXT( "/Frames/BehaviorTreeScriptTaskSelectorDialog" ), IBehTreeTaskDefinition::GetStaticClass(), defaultSelected ? defaultSelected->GetClass() : NULL )
{

}

Bool CEdBehaviorTreeScriptTaskSelectorDialog::IsSelectable( CClass* classId ) const
{
	if ( !CEdMappedClassSelectorDialog::IsSelectable( classId ) )
	{
		return false;
	}

	IBehTreeTaskDefinition* taskDef = classId->GetDefaultObject< IBehTreeTaskDefinition >();
	if ( !taskDef )
	{
		return false;
	}

	CClass* instanceClass = taskDef->GetInstancedClass();
	if ( !instanceClass )
	{
		return false;
	}

	return true;
}