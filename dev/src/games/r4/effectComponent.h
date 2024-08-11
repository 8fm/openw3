
#pragma once

#include "../../common/engine/component.h"

class CR4EffectComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CR4EffectComponent, CComponent, 0 )

private:
	CName m_effectName;
	EntityHandle m_effectTarget;
	CName m_targetBone;
public:
	CR4EffectComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual Bool CheckShouldSave() const override { return true; }
	virtual Bool UsesAutoUpdateTransform() override { return false; }

	virtual void OnSaveGameplayState( IGameSaver* saver ) override;
	virtual void OnLoadGameplayState( IGameLoader* loader ) override;

private:
	void PlayEffect();
	void StopEffect();

	void funcPlayEffect( CScriptStackFrame& stack, void* result );
	void funcStopEffect( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4EffectComponent )
PARENT_CLASS( CComponent )
PROPERTY( m_effectName );
PROPERTY( m_effectTarget );
PROPERTY( m_targetBone );
NATIVE_FUNCTION( "PlayEffect", funcPlayEffect );
NATIVE_FUNCTION( "StopEffect", funcStopEffect );
END_CLASS_RTTI()

