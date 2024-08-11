#pragma once
#include "..\..\common\game\gameplayentity.h"

//////////////////////////////////////////////////////////////////////////

class W3BoatSpawner : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( W3BoatSpawner, CGameplayEntity, 0 )
public:
	W3BoatSpawner() {}
	virtual ~W3BoatSpawner() {};

	// Called after streamable components have been streamed in and their own OnStreamIn calls have been made
	virtual void OnStreamIn() override;

	// Called before streamable components are to be streamed out, before the components' own OnStreamOut is called
	virtual void OnStreamOut() override;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( W3BoatSpawner );
	PARENT_CLASS( CGameplayEntity );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
