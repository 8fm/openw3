/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockEvents : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockEvents, CIDGraphBlock, 0 );

protected:
	CIDGraphSocket*		m_output;

public:
	CIDGraphBlockEvents()		{ };
	
	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Events ); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockEvents )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY( m_output )
END_CLASS_RTTI()
