/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"
#include "idCondition.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockCondition : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockCondition, CIDGraphBlock, 0 );

protected:
	IDConditionList*	m_conditions;
	CIDGraphSocket*		m_trueOutput;
	CIDGraphSocket*		m_falseOutput;

public:
	CIDGraphBlockCondition() {}

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Condition ); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetClientColor() const;


	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockCondition )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_INLINED( m_conditions, TXT("Conditions to be met to pass the signal to true output. If no condition is specified, the output will be passed to true anyway.") )
	PROPERTY( m_trueOutput )
	PROPERTY( m_falseOutput )
END_CLASS_RTTI()
