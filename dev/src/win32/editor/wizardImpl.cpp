/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "wizardImpl.h"
#include "../../Common/game/aiPresetParam.h"
#include "../../Common/game/aiRedefinitionParameters.h"

IMPLEMENT_ENGINE_CLASS( CAiPresetWizardData );
IMPLEMENT_ENGINE_CLASS( CCustomParamWizardData );
IMPLEMENT_ENGINE_CLASS( CWizardOptionData );

void CAiPresetWizardData::CommitData( CObject* arg )
{
	if( m_entityTemplate && arg )
	{
		ASSERT( Cast< CAIPresetsTemplateParam >(arg) );
		CAIPresetsTemplateParam* aiPresetsTemplateParam = static_cast< CAIPresetsTemplateParam* >( arg );
		if( aiPresetsTemplateParam )
		{
			aiPresetsTemplateParam->m_templateList.PushBackUnique( m_entityTemplate );
		}
	}
}

//////////////////////////////////////////////////////////
// CCustomParamWizardData
void CCustomParamWizardData::CommitData( CObject* arg )
{
	if ( arg ) 
	{
		ASSERT( Cast< CAIPresetsTemplateParam >(arg) );
		CAIPresetsTemplateParam* aiPresetsTemplateParam = static_cast< CAIPresetsTemplateParam* >( arg );
		if( aiPresetsTemplateParam )
		{
			THandle< ISerializable > clonedParams = m_customValAIParameters->Clone( aiPresetsTemplateParam );
			ICustomValAIParameters* customValAIParameters = static_cast< ICustomValAIParameters* >( clonedParams.Get() );
			customValAIParameters->SetCNameValue( CName( m_value ) );
			aiPresetsTemplateParam->AddCustomValParameters( customValAIParameters );
		}
	}
}