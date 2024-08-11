/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityTargetingAction.h"

class IEffectManagmentPerformableAction : public IEntityTargetingAction
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEffectManagmentPerformableAction, IEntityTargetingAction )
};

BEGIN_ABSTRACT_CLASS_RTTI( IEffectManagmentPerformableAction )
	PARENT_CLASS( IEntityTargetingAction )
END_CLASS_RTTI()

class CPlayEffectPerformableAction : public IEffectManagmentPerformableAction
{
	DECLARE_ENGINE_CLASS( CPlayEffectPerformableAction, IEffectManagmentPerformableAction, 0 )
protected:
	CName							m_effectName;
	CName							m_boneName;

	void							PerformOnEntity( CEntity* entity ) override;
};

BEGIN_CLASS_RTTI( CPlayEffectPerformableAction )
	PARENT_CLASS( IEffectManagmentPerformableAction )
	PROPERTY_EDIT( m_effectName, TXT( "Effects name" ) )
	PROPERTY_EDIT( m_boneName, TXT("(optional) Bone name for effect") )
END_CLASS_RTTI()

class CStopAllEffectsPerformableAction : public IEffectManagmentPerformableAction
{
	DECLARE_ENGINE_CLASS( CStopAllEffectsPerformableAction, IEffectManagmentPerformableAction, 0 )
protected:
	void							PerformOnEntity( CEntity* entity ) override;
};

BEGIN_CLASS_RTTI( CStopAllEffectsPerformableAction )
	PARENT_CLASS( IEffectManagmentPerformableAction )
END_CLASS_RTTI()

class CStopEffectPerformableAction : public IEffectManagmentPerformableAction
{
	DECLARE_ENGINE_CLASS( CStopEffectPerformableAction, IEffectManagmentPerformableAction, 0 )
protected:
	CName							m_effectName;

	void							PerformOnEntity( CEntity* entity ) override;
};

BEGIN_CLASS_RTTI( CStopEffectPerformableAction )
	PARENT_CLASS( IEffectManagmentPerformableAction )
	PROPERTY_EDIT( m_effectName, TXT( "Effects name" ) )
END_CLASS_RTTI()