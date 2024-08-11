
#pragma once

#include "animatedEntity.h"

class CBgCutsceneEntity : public CAnimatedEntity
{
	DECLARE_ENGINE_CLASS( CBgCutsceneEntity, CAnimatedEntity, 0 )

protected:
	THandle< CCutsceneTemplate >	m_csTemplate;

public:
	CBgCutsceneEntity();

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif

#ifndef NO_EDITOR_ENTITY_VALIDATION
	Bool OnValidate( TDynArray< String >& log ) const;
#endif

protected:
	virtual Bool InitializeAnimatedEntity() override;
	virtual void OnInitialized();

#ifndef NO_EDITOR
	virtual void CheckCutsceneActors();
#endif
};

BEGIN_CLASS_RTTI( CBgCutsceneEntity );
	PARENT_CLASS( CAnimatedEntity );
	PROPERTY_EDIT( m_csTemplate, TXT("") );
END_CLASS_RTTI();
