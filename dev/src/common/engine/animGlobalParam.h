
#pragma once

#include "entityTemplateParams.h"
#include "actorInterface.h"
#include "../core/2darray.h"

enum ESkeletonType : CEnum::TValueType;

class CAnimGlobalParam : public CEntityTemplateParam, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CAnimGlobalParam, CEntityTemplateParam, 0 );

protected:
	ESkeletonType	m_skeletonType;
	CName			m_defaultAnimationName;

	CName			m_customMimicsFilter_Full;
	CName			m_customMimicsFilter_Lipsync;

	CName			m_animTag;
	CName			m_sfxTag;

public:
	CAnimGlobalParam();

	void CopyFrom( const CAnimGlobalParam* rhs )
	{
		m_skeletonType = rhs->m_skeletonType;
		m_defaultAnimationName = rhs->m_defaultAnimationName;
		m_customMimicsFilter_Full = rhs->m_customMimicsFilter_Full;
		m_customMimicsFilter_Lipsync = rhs->m_customMimicsFilter_Lipsync;
		m_animTag = rhs->m_animTag;
		m_sfxTag = rhs->m_sfxTag;
	}

public:
	ESkeletonType GetSkeletonType() const;
	const CName& GetDefaultAnimationName() const;
	const CName& GetCustomMimicsFilterFullName() const;
	const CName& GetCustomMimicsFilterLipsyncName() const;
	const CName& GetSfxTag() const;
	const CName& GetAnimTag() const;
	void SetSfxTag( const CName& tag );

	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );

};

BEGIN_CLASS_RTTI( CAnimGlobalParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_skeletonType, TXT("Skeleton type") );
	PROPERTY_EDIT( m_defaultAnimationName, TXT("Default animation name") );
	PROPERTY_EDIT( m_customMimicsFilter_Full, TXT("Custom mimics filter name - full mimics") );
	PROPERTY_EDIT( m_customMimicsFilter_Lipsync, TXT("Custom mimics filter name - lipsync only") );
	PROPERTY_EDIT( m_animTag , TXT("Anim tag") );
	PROPERTY_CUSTOM_EDIT( m_sfxTag , TXT("Sfx tag"), TXT("2daValueSelection")  );
END_CLASS_RTTI();
