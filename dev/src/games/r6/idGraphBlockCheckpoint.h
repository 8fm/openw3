/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"


//------------------------------------------------------------------------------------------------------------------
// Block to restart a thread from
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockCheckpoint : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockCheckpoint, CIDGraphBlock, 0 )

private:

public:
	CIDGraphBlockCheckpoint() {};

	void OnInitInstance( InstanceBuffer& data ) const;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnRebuildSockets();

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Checkpoint ); }

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockCheckpoint );
	PARENT_CLASS( CIDGraphBlock );
END_CLASS_RTTI();
