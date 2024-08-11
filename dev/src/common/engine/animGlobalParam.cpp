
#include "build.h"
#include "animGlobalParam.h"
#include "actorInterface.h"
#include "../core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CAnimGlobalParam );

CGatheredResource resSfxEnum( TXT("sounds\\gameplay\\sfx_enum.csv"), RGF_Startup );	

CAnimGlobalParam::CAnimGlobalParam()
	: m_skeletonType( ST_Man )
{

}

ESkeletonType CAnimGlobalParam::GetSkeletonType() const
{
	return m_skeletonType;
}

const CName& CAnimGlobalParam::GetDefaultAnimationName() const
{
	return m_defaultAnimationName;
}

const CName& CAnimGlobalParam::GetCustomMimicsFilterFullName() const
{
	return m_customMimicsFilter_Full;
}

const CName& CAnimGlobalParam::GetCustomMimicsFilterLipsyncName() const
{
	return m_customMimicsFilter_Lipsync;
}

const CName& CAnimGlobalParam::GetSfxTag() const
{
	return m_sfxTag;
}

const CName& CAnimGlobalParam::GetAnimTag() const
{
	return m_animTag;
}

void CAnimGlobalParam::SetSfxTag( const CName& tag )
{
	m_sfxTag = tag;
}

void CAnimGlobalParam::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resSfxEnum.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Name");
}
