
#pragma once

#include "cutsceneActor.h"

//////////////////////////////////////////////////////////////////////////
// This is file for cutscene hacks
//////////////////////////////////////////////////////////////////////////

class CCutsceneInstance;
class ICutsceneModifierInstance;

class ICutsceneModifier : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICutsceneModifier, CObject );

public:
	virtual ICutsceneModifierInstance* CreateInstance( CCutsceneInstance* instance ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( ICutsceneModifier );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class ICutsceneModifierInstance
{
public:
	virtual ~ICutsceneModifierInstance() {}
	virtual void OnPlayed( CCutsceneInstance* instance ) {}

	virtual void OnDestroyed( CCutsceneInstance* instance ) {}

	virtual void Update( CCutsceneInstance* instance, TDynArray< CutsceneActor >& actorList ) {}

	virtual Int32 FindActor( const String& name, const TDynArray< CutsceneActor >& actorList ) { return NULL; }
};

//////////////////////////////////////////////////////////////////////////

class CCutsceneModifierFreezer : public ICutsceneModifier
{
	DECLARE_ENGINE_CLASS( CCutsceneModifierFreezer, ICutsceneModifier, 0 );

public:
	virtual ICutsceneModifierInstance* CreateInstance( CCutsceneInstance* instance ) const;
};

BEGIN_CLASS_RTTI( CCutsceneModifierFreezer )
	PARENT_CLASS( ICutsceneModifier );
END_CLASS_RTTI();
