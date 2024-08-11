#include "build.h"
#include "behTreeNodeComment.h"


//		((
//		\\``.
//		\_`.``-. 
//		( `.`.` `._  
//		 `._`-.    `._ 
//		   \`--.   ,' `. 
//		    `--._  `.  .`. 
//		     `--.--- `. ` `. 
//		         `.--  `;  .`._ 
//		           :-   :   ;. `.__,.,__ __ 
//		            `\  :       ,-(     ';o`>.
//		              `-.`:   ,'   `._ .:  (,-`,
//		                 \    ;      ;.  ,: 
//		             ,"`-._>-:        ;,'  `---.,---.
//		             `>'"  "-`       ,'   "":::::".. `-.
//		              `;"'_,  (\`\ _ `:::::::::::'"     `---.
//		               `-(_,' -'),)\`.       _      .::::"'  `----._,-"")
//		                   \_,': `.-' `-----' `--;-.   `.   ``.`--.____/ 
//		                     `-^--'                \(-.  `.``-.`-=:-.__)
//		                                            `  `.`.`._`.-._`--.)
//		                                                 `-^---^--.`--


IBehTreeNodeInstance* CBehTreeCommentDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_child == nullptr )
	{
		return nullptr;
	}
	return m_child->SpawnInstance( owner, context, parent );
}

Bool CBehTreeCommentDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( m_child )
	{
		return m_child->OnSpawn( node, context );
	}
	return false;
}

String CBehTreeCommentDefinition::GetNodeCaption() const
{
#ifndef RED_FINAL_BUILD
	return m_commentDescription;
#else
	return TBaseClass::GetNodeCaption();
#endif
}
Bool CBehTreeCommentDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeCommentDefinition::CanAddChild() const
{
	return m_child == NULL;
}
Int32 CBehTreeCommentDefinition::GetNumChildren() const
{
	return m_child ? 1 : 0;
}
IBehTreeNodeDefinition* CBehTreeCommentDefinition::GetChild( Int32 index ) const
{
	ASSERT( index == 0 );
	return m_child;
}
void CBehTreeCommentDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	ASSERT( node->GetParent() == this );
	ASSERT( !m_child );
	m_child = node;
}
void CBehTreeCommentDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_child == node )
	{
		m_child = NULL;
	}
}
Bool CBehTreeCommentDefinition::IsValid() const
{
	if ( m_child == NULL )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void CBehTreeCommentDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeCommentDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehTreeCommentDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child )
		m_child->OffsetNodesPosition( offsetX, offsetY );
}
#endif // NO_EDITOR_GRAPH_SUPPORT

CBehTreeCommentDefinition::eEditorNodeType CBehTreeCommentDefinition::GetEditorNodeType() const
{
	return NODETYPE_COMMENT;
}


