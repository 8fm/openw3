#pragma once

class CR4MapPinEntity: public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CR4MapPinEntity, CGameplayEntity, 0 );

public:
	CR4MapPinEntity()
		: m_radius( 0 )
		, m_ignoreWhenExportingMapPins( false )
	{}

protected:
	CName			m_entityName;
	LocalizedString	m_customName;
	float			m_radius;
	Bool			m_ignoreWhenExportingMapPins;

public:
	const CName&	GetEntityName()	const				{ return m_entityName; }
	void			SetEntityName( const CName& name )	{ m_entityName = name; }
	Uint32			GetCustomNameId()					{ return m_customName.GetIndex(); }
	float			GetRadius()	const					{ return m_radius; }
	void			SetRadius( float radius )			{ m_radius = radius; }
	Bool			ShouldBeIgnoredWhenExportingMapPins() { return m_ignoreWhenExportingMapPins; }
};

BEGIN_CLASS_RTTI( CR4MapPinEntity )
	PARENT_CLASS( CGameplayEntity )	
	PROPERTY_EDIT( m_entityName,		TXT( "Entity name" ) )
	PROPERTY_CUSTOM_EDIT( m_customName,	TXT( "Custom name" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_EDIT( m_radius,			TXT( "Pin radius" ) )
	PROPERTY_EDIT( m_ignoreWhenExportingMapPins, TXT( "Ignore when exporting map pins" ) )
END_CLASS_RTTI()
