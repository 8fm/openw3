/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/distanceChecker.h"
#include "behTreeCluePathData.h"

////////////////////////////////////////////////////////////////////////
// Definition

class CBehTreeDecoratorCluePathInstance;
class CBehTreeDecoratorCluePathDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorCluePathDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorCluePathInstance, CluePathDecorator );

public:

	CBehTreeDecoratorCluePathDefinition();
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

private:

	THandle< CEntityTemplate >	m_clueTemplate;
	CName						m_clueTemplate_var;
	CBehTreeValInt				m_maxClues;
	CBehTreeValFloat			m_cluesOffset;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorCluePathDefinition );
PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
PROPERTY_EDIT( m_clueTemplate, TXT( "Template of clue entity" ) );
PROPERTY_EDIT( m_clueTemplate_var, TXT( "Clue template AI param name" ) );
PROPERTY_EDIT( m_maxClues, TXT( "Maximum number of spawned clues" ) );
PROPERTY_EDIT( m_cluesOffset, TXT( "Minimal offset between two clues" ) );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance

class CBehTreeDecoratorCluePathInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

public:
	typedef CBehTreeDecoratorCluePathDefinition Definition;

	CBehTreeDecoratorCluePathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	void OnDestruction() override;
	void Update() override;
	
protected:

	Bool ShouldLeaveClue();
	void LeaveClue();

	THandle< CEntityTemplate >	m_clueTemplate;
	CEntityDistanceChecker		m_distanceChecker;
	CBehTreeCluePathDataPtr		m_cluePathData;
};


