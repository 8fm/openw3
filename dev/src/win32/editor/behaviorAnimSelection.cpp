/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorAnimSelection.h"
#include "behaviorProperties.h"

CBehaviorAnimSelection::CBehaviorAnimSelection( CPropertyItem* propertyItem )
	: IAnimationSelectionEditor( propertyItem )
{	
}

CAnimatedComponent* CBehaviorAnimSelection::RetrieveAnimationComponent() const
{
	// Get the animation node we are in
	CBehaviorGraphNode *animationNode = m_propertyItem->GetRootObject( 0 ).As< CBehaviorGraphNode >();
	ASSERT( animationNode );

	CBehaviorGraphInstance* instance = m_propertyItem->GetPage()->QueryBehaviorEditorProperties()->GetBehaviorGraphInstance();
	ASSERT( instance );

	// Make sure animated component exists for the graph we are editing
	return instance->GetAnimatedComponentUnsafe();
}
