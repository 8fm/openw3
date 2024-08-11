/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "aiProfile.h"
#include "..\engine\entityTemplateParams.h"

// CAIPresetRedefinitionParam
class CAIPresetParam : public CAITemplateParam
{
	DECLARE_ENGINE_CLASS( CAIPresetParam, CAITemplateParam, 0 );

protected:
	TDynArray< THandle< IAIParameters > > m_redefinitionParameters;
public:
	CAIPresetParam() {}
	const TDynArray< THandle< IAIParameters > > & GetRedefinitionParameters() const { return m_redefinitionParameters; }
};

BEGIN_CLASS_RTTI( CAIPresetParam );
	PARENT_CLASS( CAITemplateParam );
	PROPERTY_INLINED( m_redefinitionParameters, TXT("Collections of parameters to be injected by the preset") );
END_CLASS_RTTI();
