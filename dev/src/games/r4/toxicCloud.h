#pragma once

#include "..\..\common\game\gameplayEntity.h"

//////////////////////////////////////////////////////////////////////////////////////////////
class W3ToxicCloud : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( W3ToxicCloud, CGameplayEntity, 0 );

public:

	// Called after streamable components have been streamed in and their own OnStreamIn calls have been made
	virtual void OnStreamIn() override;

	// Called before streamable components are to be streamed out, before the components' own OnStreamOut is called
	virtual void OnStreamOut() override;
};

BEGIN_CLASS_RTTI( W3ToxicCloud );
	PARENT_CLASS( CGameplayEntity );
END_CLASS_RTTI();
