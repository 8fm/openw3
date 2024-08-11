/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "entityEventsNamesProvider.h"
#include "../../common/core/gatheredResource.h"

CGatheredResource resEntityEventNames			( TXT("gameplay\\globals\\entityeventsnames.csv"), RGF_Startup );	

void IEntityEventsNamesProvider::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resEntityEventNames.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Event name");		
}
