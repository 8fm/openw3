#pragma once

#include "..\..\common\game\container.h"



//////////////////////////////////////////////////////////////////////////////////////////////
class CBeehiveEntity : public W3Container
{
	DECLARE_ENGINE_CLASS( CBeehiveEntity, W3Container, 0 );

public:

	// Called after streamable components have been streamed in and their own OnStreamIn calls have been made
	virtual void OnStreamIn() override;

	// Called before streamable components are to be streamed out, before the components' own OnStreamOut is called
	virtual void OnStreamOut() override;
};

BEGIN_CLASS_RTTI( CBeehiveEntity );
	PARENT_CLASS( W3Container );
END_CLASS_RTTI();
