/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"


//------------------------------------------------------------------------------------------------------------------
// Output point of the dialog thread that ends the whole dialog
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockOutputTerminate : public CIDGraphBlockOutput
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockOutputTerminate, CIDGraphBlockOutput, 0 )

public:
	CIDGraphBlockOutputTerminate() {};

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( OutputTerminate ); }

	//! Get client color
	virtual Color GetClientColor() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockOutputTerminate );
	PARENT_CLASS( CIDGraphBlockOutput );
END_CLASS_RTTI();
