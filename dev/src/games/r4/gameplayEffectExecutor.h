#pragma once

class IGameplayEffectExecutor : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IGameplayEffectExecutor, CObject );

protected:
	IGameplayEffectExecutor() {};
};

BEGIN_CLASS_RTTI( IGameplayEffectExecutor );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();