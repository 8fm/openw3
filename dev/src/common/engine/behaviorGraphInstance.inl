#include "behaviorGraphTopLevelNode.h"

template< class T > RED_INLINE void CBehaviorGraphInstance::GetNodesOfClass( TDynArray< T* >& nodes ) const
{
	ASSERT( m_root );
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->GetNodesOfClass( nodes );
}