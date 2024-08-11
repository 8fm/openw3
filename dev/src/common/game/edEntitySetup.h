/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IEdEntitySetupEffector : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( IEdEntitySetupEffector )

public:
	IEdEntitySetupEffector()													{ EnableReferenceCounting( true ); }

	virtual void			OnSpawn( CEntity* entity )							= 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IEdEntitySetupEffector )
	PARENT_CLASS( IScriptable )
END_CLASS_RTTI()

class CEdSpawnEntitySetupEffector : public IEdEntitySetupEffector
{
	DECLARE_RTTI_SIMPLE_CLASS( CEdSpawnEntitySetupEffector )
protected:
	TSoftHandle< CEntityTemplate >	m_template;
	Vector							m_localPosition;
	EulerAngles						m_localOrientation;
	Bool							m_detachTemplate;
	TagList							m_extraTags;
public:
	CEdSpawnEntitySetupEffector()
		: m_localPosition( Vector::ZEROS )
		, m_localOrientation( 0.f, 0.f, 0.f )
		, m_detachTemplate( false )												{}
	~CEdSpawnEntitySetupEffector()												{}

	void					OnSpawn( CEntity* entity ) override;
};

BEGIN_CLASS_RTTI( CEdSpawnEntitySetupEffector )
	PARENT_CLASS( IEdEntitySetupEffector )
	PROPERTY_EDIT( m_template, TXT("Entity to spawn" ) )
	PROPERTY_EDIT( m_localPosition, TXT("Entity local position") )
	PROPERTY_EDIT( m_localOrientation, TXT("Entity local orientation") )
	PROPERTY_EDIT( m_detachTemplate, TXT("Detach template") )
	PROPERTY_EDIT( m_extraTags, TXT("Extra entity tags") )
END_CLASS_RTTI()