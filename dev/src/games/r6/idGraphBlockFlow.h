/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"


//------------------------------------------------------------------------------------------------------------------
// Block that waits
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockFlow : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockFlow, CIDGraphBlock, 0 )

private:
	TInstanceVar< Float >	i_timePassed;
	Float					m_timeWaiting;

public:
	CIDGraphBlockFlow() {};

	void OnInitInstance( InstanceBuffer& data ) const;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnRebuildSockets();

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Flow ); }

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockFlow );
	PARENT_CLASS( CIDGraphBlock );
	PROPERTY_EDIT( m_timeWaiting, TXT("Time waiting on this block, set 0 or less for infinite wait") );
END_CLASS_RTTI();
