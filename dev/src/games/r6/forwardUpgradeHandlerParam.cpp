#include "build.h"
#include "forwardUpgradeHandlerParam.h"

#include "../../common/core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CForwardUpgradeHandlerParam );

CGatheredResource resForwardUpgradeEventNames			( TXT("gameplay\\globals\\upgradeevents.csv"), RGF_Startup );	

void CForwardUpgradeHandlerParam::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resForwardUpgradeEventNames.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Event name");	
}
