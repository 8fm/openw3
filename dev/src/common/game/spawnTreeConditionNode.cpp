#include "build.h"
#include "spawnTreeConditionNode.h"

#include "spawnCondition.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeConditionNode );

Bool CSpawnTreeConditionNode::TestConditions( CSpawnTreeInstance& instance ) const
{
	for( Uint32 i = 0, n = m_conditions.Size(); i < n; ++i )
	{
		ISpawnCondition* condition = m_conditions[ i ].Get();

		if( condition && !condition->Test( instance ) )
		{
			return false;
		}
	}
	return TBaseClass::TestConditions( instance );
}
//void CSpawnTreeConditionNode::OnPostLoad()
//{
//	TBaseClass::OnPostLoad();
//
//	for( Uint32 i = 0; i < m_conditions.Size(); )
//	{
//		if ( m_conditions[ i ] == NULL )
//		{
//			m_conditions.RemoveAt( i );
//		}
//		else
//		{
//			++i;
//		}
//	}
//}
Color CSpawnTreeConditionNode::GetBlockColor() const
{
	return Color( 150, 150, 30 );
}
String CSpawnTreeConditionNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("Condition") );
	return STR;
}
String CSpawnTreeConditionNode::GetBitmapName() const
{
	static const String STR( TXT("IMG_SPAWNTREE_CONDITION") );
	return STR;
}