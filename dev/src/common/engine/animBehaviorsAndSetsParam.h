
#pragma once

#include "entityTemplateParams.h"
#include "behaviorGraph.h"

class CAnimBehaviorsParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CAnimBehaviorsParam, CEntityTemplateParam, 0 );

protected:
	String									m_name;
	String									m_componentName;
	TDynArray< SBehaviorGraphInstanceSlot > m_slots;

public:
	void SetName( const String& name )	{ m_name = name; }
	const String& GetName() const		{ return m_name; }

	void SetComponentName( const String& name )	{ m_componentName = name; }
	const String& GetComponentName() const		{ return m_componentName; }

	const TDynArray< SBehaviorGraphInstanceSlot >& GetSlots() const;
	void GetSlots( TDynArray< SBehaviorGraphInstanceSlot* >& slots );
};

BEGIN_CLASS_RTTI( CAnimBehaviorsParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_name, TXT("Name") );
	PROPERTY_EDIT( m_componentName, TXT("Component name") );
	PROPERTY_EDIT( m_slots, TXT("Behaviors") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimAnimsetsParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CAnimAnimsetsParam, CEntityTemplateParam, 0 );

public:
	typedef TDynArray< THandle< CSkeletalAnimationSet > >	TAnimationSets;

protected:
	String				m_name;
	String				m_componentName;
	TAnimationSets		m_animationSets;

public:
	void SetName( const String& name )	{ m_name = name; }
	const String& GetName() const		{ return m_name; }

	void SetComponentName( const String& name )	{ m_componentName = name; }
	const String& GetComponentName() const		{ return m_componentName; }

	const TAnimationSets& GetAnimationSets() const;
};

BEGIN_CLASS_RTTI( CAnimAnimsetsParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_name, TXT("Name") );
	PROPERTY_EDIT( m_componentName, TXT("Component name") );
	PROPERTY_EDIT( m_animationSets, TXT("Animation sets") );
END_CLASS_RTTI();
