#pragma once

#include "behTreeNodeSelector.h"
#include "behTreeArbitratorPriorities.h"

class CBehTreeNodeArbitratorInstance;

////////////////////////////////////////////////////////////////////////
struct SArbitratorQueryData
{
	DECLARE_RTTI_STRUCT( SArbitratorQueryData );
public:
	SArbitratorQueryData( IBehTreeNodeDefinition::Priority priorityCheck = 0 )
		: m_priority( priorityCheck )
		, m_queryResult( true )											{}	// notice that query results in true by default (eg. no arbitrator)
	IBehTreeNodeDefinition::Priority			m_priority;			
	Bool										m_queryResult;
};
BEGIN_CLASS_RTTI( SArbitratorQueryData );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Definition
// Arbitrator. Basically its selector but I'm quite sure it will get
// some custom features.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeArbitratorDefinition : public CBehTreeNodeSelectorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeArbitratorDefinition, CBehTreeNodeSelectorDefinition, CBehTreeNodeArbitratorInstance, Arbitrator );
protected:
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeArbitratorDefinition();
};

BEGIN_CLASS_RTTI(CBehTreeNodeArbitratorDefinition);
	PARENT_CLASS(CBehTreeNodeSelectorDefinition);
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeArbitratorInstance : public CBehTreeNodeBestScoreSelector
{
	typedef CBehTreeNodeBestScoreSelector Super;
public:
	typedef CBehTreeNodeArbitratorDefinition Definition;

	CBehTreeNodeArbitratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeBestScoreSelector( def, owner, context, parent )	{}

	Bool OnEvent( CBehTreeEvent& e ) override;


};

