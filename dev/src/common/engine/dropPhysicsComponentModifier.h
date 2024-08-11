/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CDropPhysicsComponent;

class IDropPhysicsComponentModifier
{
public:
	virtual void DropPhysicsComponentOnInitialized( CDropPhysicsComponent* dropPhysicsComponent ) = 0;
	virtual void DropPhysicsComponentOnFinalize( CDropPhysicsComponent* dropPhysicsComponent ) = 0;
};

///////////////////////////////////////////////////////////

/// Drop Physics Component modifier
class CDropPhysicsComponentModifierManager
{
private:
	THashMap< Uint32, TDynArray<IDropPhysicsComponentModifier*> > m_modifiers;

	friend class CDropPhysicsComponent;

public:
	CDropPhysicsComponentModifierManager(){}
	virtual ~CDropPhysicsComponentModifierManager(){}

	Bool RegisterModifier( const String& entityTemplateName, IDropPhysicsComponentModifier* dropPhysicsComponentModifier );
	Bool UnregisterModifier( const String& entityTemplateName, IDropPhysicsComponentModifier* dropPhysicsComponentModifier );
	Bool IsModifierRegistered( const String& entityTemplatePath ) const;

private:
	virtual void DropPhysicsComponentOnInitialized( CDropPhysicsComponent* dropPhysicsComponent );
	virtual void DropPhysicsComponentOnFinalize( CDropPhysicsComponent* dropPhysicsComponent );
};

typedef TSingleton< CDropPhysicsComponentModifierManager, TDefaultLifetime, TCreateUsingNew > SDropPhysicsComponentModifierManager;
