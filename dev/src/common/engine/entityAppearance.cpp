/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityAppearance.h"
#include "entityTemplateParams.h"
#include "entityTemplate.h"
#include "entity.h"
#include "component.h"
#include "../core/dataError.h"
#include "../core/gameSave.h"


IMPLEMENT_ENGINE_CLASS( CEntityAppearance );
IMPLEMENT_ENGINE_CLASS( CComponentReference );
IMPLEMENT_ENGINE_CLASS( CEntityBodyPartState );
IMPLEMENT_ENGINE_CLASS( CEntityBodyPart );

//////////////////////////////////////////////////////////////////////////////////////////////////////

//! Initialize from component
CComponentReference::CComponentReference( const CComponent& component )
	: m_name( component.GetName() )
	, m_className( component.GetClass()->GetName() )
{}

//! Is this a reference to given component ?
Bool CComponentReference::IsReferenceTo( const CComponent &component ) const
{
	return m_className != CName::NONE && component.GetClass()->GetName() == m_className && m_name == component.GetName();
}

void CEntityBodyPartState::AddComponent( const CComponent & component )
{
	// Make sure we do not add twice the same reference
	for ( Uint32 i=0; i<m_componentsInUse.Size(); ++i )
	{
		if ( m_componentsInUse[i].IsReferenceTo( component ) )
		{
			return;
		}
	}

	// Add an reference
	::new ( m_componentsInUse ) CComponentReference( component );
}

void CEntityBodyPartState::RemoveComponent( const CComponent & component )
{
	for ( Uint32 i=0; i<m_componentsInUse.Size(); ++i )
	{
		if ( m_componentsInUse[i].IsReferenceTo( component ) )
		{
			m_componentsInUse.EraseFast( m_componentsInUse.Begin() + i );
			break;
		}
	}
}

void CEntityBodyPartState::RemoveComponent( Uint32 index )
{
	m_componentsInUse.EraseFast( m_componentsInUse.Begin() + index );
}

void CEntityBodyPartState::SetName( const CName & name )
{
	m_name = name;
}

Bool CEntityBodyPartState::UseComponent( const CComponent* component ) const
{
	if ( component )
	{
		for ( Uint32 i=0; i<m_componentsInUse.Size(); ++i )
		{
			if ( m_componentsInUse[i].IsReferenceTo( *component ) )
			{
				return true;
			}
		}
	}

	// Not referenced
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CEntityBodyPart::CEntityBodyPart()
	: m_wasIncluded( false )
{
}

CEntityBodyPart::CEntityBodyPart( const CEntityBodyPart& source, CEntityTemplate* baseTemplate )
	: m_name( source.m_name )
	, m_states( source.m_states )
	, m_wasIncluded( true )
{
}

void CEntityBodyPart::AddState( CEntityBodyPartState& state )
{
	m_states.PushBackUnique( state );
}

void CEntityBodyPart::RemoveState( CEntityBodyPartState& state )
{
	m_states.RemoveFast( state );
}

void CEntityBodyPart::SetName( const CName &name )
{
	m_name = name;
}

void CEntityBodyPart::SetWasIncluded()
{
	m_wasIncluded = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CEntityAppearance::CEntityAppearance( const CEntityAppearance& source, CEntityTemplate* baseTemplate )
	: m_name( source.m_name )
	, m_voicetag( source.m_voicetag )
	, m_wasIncluded( true )
	, m_useVertexCollapse( source.m_useVertexCollapse )
	, m_usesRobe( source.m_usesRobe )
	, m_includedTemplates( source.m_includedTemplates )
	, m_collapsedComponents( source.m_collapsedComponents )
{
	// Copy parameters
	for ( Uint32 i=0; i<source.m_appearanceParams.Size(); ++i )
	{
		CEntityTemplateParam* param = source.m_appearanceParams[i];
		if ( param )
		{
			CEntityTemplateParam* clonedParam = Cast< CEntityTemplateParam >( param->Clone( baseTemplate ) );
			if ( clonedParam )
			{
				m_appearanceParams.PushBack( clonedParam );
			}
		}
	}
}

void CEntityAppearance::SetName( const CName& name )
{
	m_name = name;
}

void CEntityAppearance::SetVoicetag( const CName & voicetag )
{
	m_voicetag = voicetag;
}

Bool CEntityAppearance::AddParameter( CEntityTemplateParam* param )
{
	if ( !param )
	{
		return false;
	}

	// verify that the param we want to add doesn't share a type
	// with an existing one
	const CName& newParamType = param->GetClass()->GetName();
	for ( TDynArray< CEntityTemplateParam* >::iterator it = m_appearanceParams.Begin();
		it != m_appearanceParams.End(); ++it )
	{
		if ( *it == param )
		{
			// we don't need duplicates either
			return false;
		}

		if ( !*it || !(*it)->GetClass() )
		{
			continue;
		}
		const CName& type = (*it)->GetClass()->GetName();
		if ( type == newParamType )
		{
			// types match - we can't add this here
			return false;
		}
	}

	m_appearanceParams.PushBack( param );
	return true;
}

Bool CEntityAppearance::RemoveParameter( CEntityTemplateParam* param )
{
	if ( !param )
	{
		return false;
	}

	// Delete effect
	m_appearanceParams.Remove( param );
	return true;
}

void CEntityAppearance::IncludeTemplate( CEntityTemplate* appearanceTemplate )
{
	m_includedTemplates.PushBack( appearanceTemplate );
}

void CEntityAppearance::RemoveTemplate( CEntityTemplate* appearanceTemplate )
{
	m_includedTemplates.Remove( appearanceTemplate );
	DecollapseTemplate( appearanceTemplate );
}

bool CEntityAppearance::ValidateIncludedTemplates( const CResource* resource )
{
	Bool errorsFound = false;
	// Check to see if m_includedTemplates references valid CEntityTemplates.
	// As null templates can be added and this will cause a hard crash.
	// as a result we remove the bad links, and throw out a data assert.
	for( Int32 i = m_includedTemplates.SizeInt() - 1; i >= 0; --i )
	{
		if ( !m_includedTemplates[i].IsValid() )
		{
			DATA_HALT( DES_Major, resource, TXT("Entity Template"), TXT("Entity Appearance for asset {%s} contains a NULL included template *THIS HAS BEEN REMOVED*, please re-save and resubmit the asset to perforce"), resource->GetDepotPath().AsChar() );
			m_includedTemplates.RemoveAt( i );
			errorsFound = true;
		}
	}

	return !errorsFound;
}

void CEntityAppearance::CollapseTemplate( CEntityTemplate* appearanceTemplate )
{
	CEntity* tplEntity = appearanceTemplate->GetEntityObject();
	const TDynArray<CComponent*>& tplEntityComponents = tplEntity->GetComponents();
	for ( auto it=tplEntityComponents.Begin(); it != tplEntityComponents.End(); ++it )
	{
		m_collapsedComponents.PushBackUnique( CName( (*it)->GetName() ) );
	}
}

void CEntityAppearance::DecollapseTemplate( CEntityTemplate* appearanceTemplate )
{
	CEntity* tplEntity = appearanceTemplate->GetEntityObject();
	const TDynArray<CComponent*>& tplEntityComponents = tplEntity->GetComponents();
	for ( auto it=tplEntityComponents.Begin(); it != tplEntityComponents.End(); ++it )
	{
		// TODO: update this when name becomes a CName
		m_collapsedComponents.Remove( CName( (*it)->GetName() ) );
	}
}

Bool CEntityAppearance::IsCollapsed( CComponent* component ) const
{
	// TODO: update this when name becomes a CName
	return m_collapsedComponents.Exist( CName( component->GetName() ) );
}

void CEntityAppearance::SetWasIncluded()
{
	// Mark the appearance as included
	m_wasIncluded = true;

	// Mark all paramters as included to
	for ( Uint32 i=0; i<m_appearanceParams.Size(); ++i )
	{
		CEntityTemplateParam* param = m_appearanceParams[i];
		if ( param )
		{
			param->SetWasIncluded();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CMBodyPartStates::CMBodyPartStates()
	: m_bodyPartStates( NULL )
{

}

CMBodyPartStates::~CMBodyPartStates()
{
	if ( m_bodyPartStates )
	{
		delete m_bodyPartStates;
		m_bodyPartStates = NULL;
	}
}

CName CMBodyPartStates::Get( const CName& bodyPartName ) const
{
	// Search for defined body part state for given body part
	if ( m_bodyPartStates )
	{
		for ( Uint32 i=0; i<m_bodyPartStates->Size(); ++i )
		{
			const BodyPart& bodyPart = (*m_bodyPartStates)[i];
			if ( bodyPart.m_part == bodyPartName )
			{
				return bodyPart.m_state;
			}
		}
	}

	// Not defined
	return CName::NONE;
}

void CMBodyPartStates::Set( const CName& bodyPartName, const CName& bodyPartStateName )
{
	if ( bodyPartStateName == CName::NONE )
	{
		// Remove body part definition
		if ( m_bodyPartStates )
		{
			for ( Uint32 i=0; i<m_bodyPartStates->Size(); ++i )
			{
				const BodyPart& bodyPart = (*m_bodyPartStates)[i];
				if ( bodyPart.m_part == bodyPartName )
				{
					m_bodyPartStates->Erase( m_bodyPartStates->Begin() + i );
					return;
				}
			}
		}
	}
	else
	{
		// Create map
		if ( !m_bodyPartStates )
		{
			m_bodyPartStates = new TDynArray< BodyPart >();
		}

		// Change existing value if it's defined
		for ( Uint32 i=0; i<m_bodyPartStates->Size(); ++i )
		{
			BodyPart& bodyPart = (*m_bodyPartStates)[i];
			if ( bodyPart.m_part == bodyPartName )
			{
				bodyPart.m_state = bodyPartStateName;
				return;
			}
		}

		// Add new value
		BodyPart* def = new ( *m_bodyPartStates ) BodyPart;
		def->m_part = bodyPartName;
		def->m_state = bodyPartStateName;
	}
}

void CMBodyPartStates::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(bodyPartStates) );

	// Save count
	const Uint32 count = m_bodyPartStates ? m_bodyPartStates->Size() : 0 ;
	saver->WriteValue( CNAME(count), count );

	// Save parts
	if ( m_bodyPartStates )
	{
		for ( Uint32 i=0; i<m_bodyPartStates->Size(); ++i )
		{
			CGameSaverBlock block( saver, CNAME(bodyPartState) );

			// Save value
			const BodyPart& part = (*m_bodyPartStates)[i];
			saver->WriteValue( CNAME(part), part.m_part );
			saver->WriteValue( CNAME(state), part.m_state );
		}
	}
}

void CMBodyPartStates::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(bodyPartStates) );

	// Destroy current data
	if ( m_bodyPartStates )
	{
		delete m_bodyPartStates;
		m_bodyPartStates = NULL;
	}

	// Load the count
	const Uint32 count = loader->ReadValue< Uint32 >( CNAME(count) );
	if ( count > 0 )
	{
		// Create the table
		m_bodyPartStates = new TDynArray< BodyPart >();
		m_bodyPartStates->Resize( count );

		// Load data
		for ( Uint32 i=0; i<count; ++i )
		{
			CGameSaverBlock block( loader, CNAME(bodyPartState) );

			// Save value
			BodyPart& part = (*m_bodyPartStates)[i];
			loader->ReadValue( CNAME(part), part.m_part );
			loader->ReadValue( CNAME(state), part.m_state );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
