#pragma once

#include "spawnTreeCompositeMember.h"

class CSpawnTreeConditionNode : public CSpawnTreeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeConditionNode, CSpawnTreeNode, 0 );
protected:
	TDynArray< THandle< ISpawnCondition > >			m_conditions;

	Bool				TestConditions( CSpawnTreeInstance& instance ) const override;

public:
	// IEdSpawnTreeNode interface
	Color				GetBlockColor() const override;
	String				GetEditorFriendlyName() const override;
	String				GetBitmapName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeConditionNode );
	PARENT_CLASS( CSpawnTreeNode );
	PROPERTY_INLINED( m_conditions, TXT("Conditions list") );
END_CLASS_RTTI();