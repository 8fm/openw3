#pragma once

#include "../engine/entityTemplateParams.h"

//////////////////////////////////////////////////////////////////////////

class CCharacterStatsParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CCharacterStatsParam, CGameplayEntityParam, 0 );

protected:
	TDynArray<CName>					m_abilities;

public:

	const TDynArray<CName> & GetAbilities() {return m_abilities; }
};

BEGIN_CLASS_RTTI( CCharacterStatsParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_abilities, TXT("Array of auto buffs"), TXT("AbilitySelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAutoEffectsParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CAutoEffectsParam, CGameplayEntityParam, 0 );

protected:
	TDynArray<CName>					m_autoEffects;

public:

	const TDynArray<CName> & GetAutoEffects() {return m_autoEffects; }
};

BEGIN_CLASS_RTTI( CAutoEffectsParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_autoEffects, TXT("Array of abilities"), TXT("EffectSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAIAttackRange;

class CAttackRangeParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CAttackRangeParam, CGameplayEntityParam, 0 );

protected:
	TDynArray<CAIAttackRange* >	m_attackRanges;

public:
	virtual void OnPostLoad() override;

	void AddAttackRange( CAIAttackRange* attackRange ) { m_attackRanges.PushBack( attackRange ); }
	const CAIAttackRange* GetAttackRange( const CName& attackRangeName = CName::NONE  ) const;
};

BEGIN_CLASS_RTTI( CAttackRangeParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_INLINED( m_attackRanges, TXT( "List of attack ranges" ) );
END_CLASS_RTTI();

class CAttackableArea : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CAttackableArea, CGameplayEntityParam, 0 );

protected:
	Vector	m_offset;
	Float	m_radius;
	Float	m_height;

public:
	RED_INLINE const Vector& GetOffset() const	{ return m_offset; }
	RED_INLINE Float GetRadius() const			{ return m_radius; }
	RED_INLINE Float GetHeight() const			{ return m_height; }
};

BEGIN_CLASS_RTTI( CAttackableArea )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_EDIT( m_offset, TXT( "Cylinder offset" ) )
	PROPERTY_EDIT( m_radius, TXT( "Cylinder radius" ) )
	PROPERTY_EDIT( m_height, TXT( "Cylinder height" ) )
END_CLASS_RTTI()

class CPlayerTargetPriority : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CPlayerTargetPriority, CGameplayEntityParam, 0 );

protected:
	Float	m_priority;

public:
	CPlayerTargetPriority() : m_priority( 1.f ) {}

	RED_INLINE Float GetPriority() const	{ return m_priority; }
};

BEGIN_CLASS_RTTI( CPlayerTargetPriority )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_EDIT( m_priority, TXT( "Priority of this target for player") )
END_CLASS_RTTI()

class CAlternativeDisplayName : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CAlternativeDisplayName, CGameplayEntityParam, 0 );

protected:

	LocalizedString m_altName;
	String			m_factID;

public:
	CAlternativeDisplayName() {}
	
	const LocalizedString& GetAltName() const { return m_altName; }
	LocalizedString& GetAltName() { return m_altName; }
	const String& GetFactID() const { return m_factID; }
};

BEGIN_CLASS_RTTI( CAlternativeDisplayName )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_CUSTOM_EDIT( m_altName, TXT( "A localized name of this entity" ), TXT( "EntityDisplayNameSelector" ) );
	PROPERTY_EDIT( m_factID, TXT( "Priority of this target for player") )
END_CLASS_RTTI()

class CBloodTrailEffect : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CBloodTrailEffect, CGameplayEntityParam, 0 );

protected:
	CName	m_effect;

public:
	CBloodTrailEffect() : m_effect( CName::NONE ) {}

	RED_INLINE const CName& GetEffectName() const { return m_effect; }

private:
	void funcGetEffectName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CBloodTrailEffect )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_EDIT( m_effect, TXT( "Effect played as a blood trail") )
	NATIVE_FUNCTION( "GetEffectName", funcGetEffectName )
END_CLASS_RTTI()