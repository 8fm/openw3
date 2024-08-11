/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/loadingJobManager.h"
#include "entityTemplatePreloadedEffects.h"
#include "jobEffectPreloading.h"

struct CEntityTemplateCookedEffect
{
public:
	DECLARE_RTTI_STRUCT( CEntityTemplateCookedEffect );

	CEntityTemplateCookedEffect();
	CEntityTemplateCookedEffect( const CName& name, const CName& animName, const SharedDataBuffer& buffer );

	void LoadAsync( CEntityTemplatePreloadedEffects* preloadedEffects );
	void LoadSync( CEntityTemplatePreloadedEffects* preloadedEffects );

	RED_FORCE_INLINE const CName& GetName() const { return m_name; }
	RED_FORCE_INLINE const CName& GetAnimName() const { return m_animName; }

private:
	CName m_name;
	CName m_animName;

	SharedDataBuffer m_buffer;
};
BEGIN_CLASS_RTTI( CEntityTemplateCookedEffect )
	PROPERTY( m_name )
	PROPERTY( m_animName )
	PROPERTY( m_buffer )
END_CLASS_RTTI();

class CEntityTemplateCookedEffectLegacy
{
public:
	CName m_name;
	CName m_animName;
	Uint32 m_offset;
	Uint32 m_size;

	friend IFile& operator<<( IFile& file, CEntityTemplateCookedEffect& cookedEffect );
};
