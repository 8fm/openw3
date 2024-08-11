#pragma once
#include "boidPointOfInterestComponent.h"

class CBoidPointOfInterestComponentScript : public CBoidPointOfInterestComponent
{
	DECLARE_ENGINE_CLASS( CBoidPointOfInterestComponentScript, CBoidPointOfInterestComponent, 0 );
public:
	CBoidPointOfInterestComponentScript();

	void OnUsed( Uint32 count, Float deltaTime ) override;
};

BEGIN_CLASS_RTTI( CBoidPointOfInterestComponentScript );
	PARENT_CLASS( CBoidPointOfInterestComponent );
END_CLASS_RTTI();