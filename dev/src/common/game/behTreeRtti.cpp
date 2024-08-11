#include "build.h"

#include "behTreeRttiImplementation.h"

#include "behTreeNode.h"
#include "behTreeScriptedNode.h"
#include "behTreeNodeArbitrator.h"


void CBehNodesManager::RegisterInternal( FUNC_NODE_NAME niceName, CClass* classId, Bool internalClass )
{
	SBehTreeClassInfo classInfo;
	classInfo.m_niceName = niceName;
	classInfo.m_isInternalClass = internalClass;
	m_classes.Insert( classId->GetName(), classInfo );
}


CBehNodesManager CBehNodesManager::sm_Instance;

IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeScriptTerminalDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeScriptDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeArbitratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeScriptConditionalDecoratorDefinition );
