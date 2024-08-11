#pragma once
#include "aiParameters.h"
////////////////////////////////////////////////////////////////
// CAIRedefinitionParameters
class CAIRedefinitionParameters : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIRedefinitionParameters );
};

BEGIN_CLASS_RTTI( CAIRedefinitionParameters )
	PARENT_CLASS( IAIParameters )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////
// ICustomValAIParameters
class ICustomValAIParameters : public CAIRedefinitionParameters
{
	typedef CAIRedefinitionParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( ICustomValAIParameters );
private:
	
public:
	void SetCNameValue( CName name );
};

BEGIN_CLASS_RTTI( ICustomValAIParameters )
	PARENT_CLASS( CAIRedefinitionParameters )
END_CLASS_RTTI()