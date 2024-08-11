#pragma once

#include "spawnTreeInitializer.h"

class ISpawnTreeInitializerGuardAreaBase : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeInitializerGuardAreaBase, ISpawnTreeInitializer );
protected:
	Float							m_pursuitRange;

	virtual void					Configure( CAreaComponent*& guardArea, CAreaComponent*& pursuitArea, CSpawnTreeInstance* instance ) const = 0;
public:
	ISpawnTreeInitializerGuardAreaBase()
		: m_pursuitRange( -1.f )														{}

	ISpawnTreeInitializer::EOutput	Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	Bool							IsConflicting( const ISpawnTreeInitializer* initializer ) const override;
};


BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeInitializerGuardAreaBase );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY_EDIT( m_pursuitRange, TXT("Override pursuit range (<0 to leave out current one)") )
END_CLASS_RTTI();


class CSpawnTreeInitializerGuardArea : public ISpawnTreeInitializerGuardAreaBase
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerGuardArea, ISpawnTreeInitializerGuardAreaBase, 0 );
protected:
	CName												m_guardAreaTag;
	CName												m_pursuitAreaTag;
	Bool												m_findAreasInEncounter;

	TInstanceVar< THandle< CAreaComponent > >			i_guardArea;
	TInstanceVar< THandle< CAreaComponent > >			i_pursueArea;
	
	void							Configure( CAreaComponent*& guardArea, CAreaComponent*& pursuitArea, CSpawnTreeInstance* instance ) const override;
public:
	CSpawnTreeInitializerGuardArea()													{}

	String							GetEditorFriendlyName() const override;

	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerGuardArea )
	PARENT_CLASS( ISpawnTreeInitializerGuardAreaBase )
	PROPERTY_EDIT( m_guardAreaTag, TXT("Tag for the guard area") )
	PROPERTY_EDIT( m_pursuitAreaTag, TXT("Tag for the guard retreat area") )
	PROPERTY_EDIT( m_findAreasInEncounter, TXT("Only look for tagged areas inside given encounter") )
END_CLASS_RTTI()

class CSpawnTreeInitializerGuardAreaByHandle : public ISpawnTreeInitializerGuardAreaBase
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerGuardAreaByHandle, ISpawnTreeInitializerGuardAreaBase, 0 )

protected:
	mutable EntityHandle			m_guardArea;
	mutable EntityHandle			m_pursuitArea;

	void							Configure( CAreaComponent*& guardArea, CAreaComponent*& pursuitArea, CSpawnTreeInstance* instance ) const override;
public:
	String							GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerGuardAreaByHandle )
	PARENT_CLASS( ISpawnTreeInitializerGuardAreaBase )
	PROPERTY_EDIT( m_guardArea, TXT("Guard area") )
	PROPERTY_EDIT( m_pursuitArea, TXT("Pursuit area") )
END_CLASS_RTTI()
