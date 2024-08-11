#include "build.h"
#include "behaviorTreeBlockTypeSelector.h"

#include "../../../common/game/behTreeNode.h"

CEdBehaviorTreeBlockTypeSelectorDialog::CEdBehaviorTreeBlockTypeSelectorDialog( wxWindow* parent, CClassHierarchyMapper& hierarchy, CClass* rootClass )
	: CEdMappedClassSelectorDialog( parent, hierarchy, TXT( "/Frames/BehaviorTreeBlockSelectorDialog" ), rootClass ? rootClass : IBehTreeNodeDefinition::GetStaticClass() )
{}

Bool CEdBehaviorTreeBlockTypeSelectorDialog::IsSelectable( CClass* classId ) const 
{
	if ( false == __super::IsSelectable( classId ) )	 // IsAbstract()
	{
		return false;
	}

	IBehTreeNodeDefinition* defaultObject = classId->GetDefaultObject< IBehTreeNodeDefinition >();
	if ( !defaultObject )
	{
		return false;
	}

	return defaultObject->IsUsableInGame( GCommonGame );
}

Bool CEdBehaviorTreeDecoratorBlockTypeSelectorDialog::IsSelectable( CClass* classId ) const
{
	if ( false == __super::IsSelectable( classId ) )
	{
		return false;
	}

	IBehTreeNodeDefinition* defaultObject = classId->GetDefaultObject< IBehTreeNodeDefinition >();
	if ( !defaultObject )
	{
		return false;
	}

	return defaultObject->CanAddChild();
}
