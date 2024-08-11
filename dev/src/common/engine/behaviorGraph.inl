#include "behaviorGraphContainerNode.h"

template< class T > RED_INLINE void CBehaviorGraph::GetNodesOfClass( TDynArray< T* >& nodes ) const
{
	class CFinder
	{
	public:
		CFinder() {}

		void Find( CBehaviorGraphNode* node, TDynArray< T* >& inputNodes ) 
		{
			if ( node && node->IsA< CBehaviorGraphContainerNode >() )
			{
				CBehaviorGraphContainerNode *containerNode = SafeCast< CBehaviorGraphContainerNode >( node );

				TDynArray< CGraphBlock* >& children = containerNode->GetConnectedChildren();

				for( Uint32 i=0; i<children.Size(); ++i )
				{
					if ( CBehaviorGraphNode* bgChild = Cast< CBehaviorGraphNode >( children[i] ) )
					{
						Find( bgChild, inputNodes );	
					}
				}
			}

			if ( node && node->IsA< T >() )
			{
				inputNodes.PushBack( SafeCast< T >(node) );
			}
		}
	};

	if ( m_rootNode )
	{
		CFinder finder;
		finder.Find( m_rootNode, nodes );
	}
}