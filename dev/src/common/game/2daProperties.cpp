#include "build.h"
#include "2daProperties.h"
#include "actionPoint.h"
#include "../../common/core/gatheredResource.h"

void CActionPointCategories2dPropertyOwner::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = &SActionPointResourcesManager::GetInstance().Get2dArray();
	valueProperties.m_valueColumnName = TXT("Category");
}

Bool CActionPointCategories2dPropertyOwner::SortChoices() const
{
	return true;
}

void CAttitude2dPropertyOwner::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array =  &SAttitudesResourcesManager::GetInstance().Get2dArray();
	valueProperties.m_descrColumnName = TXT( "GroupName" );
	valueProperties.m_valueColumnName = TXT( "GroupName" );
}

Bool CAttitude2dPropertyOwner::SortChoices() const
{
	return true;
}

