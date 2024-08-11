/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class INormalBlendDataSource : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( INormalBlendDataSource, CObject );

public:
	INormalBlendDataSource();

	virtual const Float* GetWeights() const = 0;

	virtual void Tick( Float dt ) = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( INormalBlendDataSource );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();
