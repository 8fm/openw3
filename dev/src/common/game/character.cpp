#include "build.h"
#include "character.h"

IMPLEMENT_ENGINE_CLASS( CCharacter );

CCharacter::CCharacter()
#ifndef FINAL
	: m_parentCharacter( nullptr )
#endif
{
	m_guid = CGUID::Create();
}

#ifndef FINAL
void CCharacter::SetParentCharacter( CCharacter* newParent )
{
	// if property is the same as in old parent or
	// there is no parent and property isn't set
	// then copy this property from new parent
#define SET_DERIVED_PROP( prop, emptyCheck, clearing ) \
	if ( ( m_parentCharacter && ( prop == m_parentCharacter->prop ) ) || \
		( !m_parentCharacter && emptyCheck ) ) \
	{ \
		if ( newParent ) \
		{ \
			prop = newParent->prop; \
		} \
		else \
		{ \
			clearing; \
		} \
	} \

	SET_DERIVED_PROP( m_voiceTag, m_voiceTag.Empty(), m_voiceTag = CName::NONE )
	SET_DERIVED_PROP( m_tags, m_tags.Empty(), m_tags.Clear(); )
	SET_DERIVED_PROP( m_entityTemplate, m_entityTemplate.IsEmpty(), m_entityTemplate = TSoftHandle< CEntityTemplate >(); )

#undef SET_DERIVED_PROP

	m_parentCharacter = newParent;
}

Bool CCharacter::IsInherited( const CProperty* prop )
{
	RED_FATAL_ASSERT( prop, "Property can't be null" );

	const CCharacter* parent = GetParentCharacter();
	if ( parent == nullptr )
	{
		parent = GetClass()->GetDefaultObject< CCharacter >();
	}

	IRTTIType* type = prop->GetType();
	const void* data1 = prop->GetOffsetPtr( this );
	const void* data2 = prop->GetOffsetPtr( parent );
	return type->Compare( data1, data2, 0 );
}

void CCharacter::UpdateInheritedProperty( const CProperty* prop )
{
	const CCharacter* parent = GetParentCharacter();
	if ( parent == nullptr )
	{
		return;
	}

	IRTTIType* type = prop->GetType();
	void* data1 = prop->GetOffsetPtr( this );
	const void* data2 = prop->GetOffsetPtr( parent );

	type->Copy( data1, data2 );
}
#endif