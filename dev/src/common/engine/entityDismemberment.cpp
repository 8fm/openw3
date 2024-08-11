/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityDismemberment.h"
#include "entityTemplate.h"
#include "entity.h"
#include "appearanceComponent.h"
#include "../core/hashset.h"

IMPLEMENT_ENGINE_CLASS( CEntityDismemberment );
IMPLEMENT_ENGINE_CLASS( SDismembermentEffect );
IMPLEMENT_ENGINE_CLASS( SDismembermentWoundDecal );
IMPLEMENT_ENGINE_CLASS( SDismembermentWoundSingleSpawn );
IMPLEMENT_ENGINE_CLASS( CDismembermentWound );
IMPLEMENT_ENGINE_CLASS( SDismembermentWoundFilter );
IMPLEMENT_RTTI_ENUM( EWoundTypeFlags );
IMPLEMENT_RTTI_BITFIELD( EDismembermentEffectTypeFlag );

//////////////////////////////////////////////////////////////////////////

void SDismembermentWoundSingleSpawn::CollectEffects( Uint32 effectsMask, TDynArray< CName > & effectsNames ) const
{
	if ( effectsMask & DETF_Base )
	{
		effectsNames.PushBack( m_effectsNames );
	}
	for ( const SDismembermentEffect& effect : m_additionalEffects )
	{
		if ( effectsMask & effect.m_typeMask )
		{
			effectsNames.PushBackUnique( effect.m_name );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CDismembermentWound::CDismembermentWound()
	: m_name( CName::NONE )
	, m_fillMesh( nullptr )
{}


CDismembermentWound::CDismembermentWound( const CName& name )
	: m_name( name )
	, m_fillMesh( nullptr )
	, m_isExplosionWound ( false )
	, m_isFrostWound ( false )
{
}

#ifndef NO_EDITOR
Bool CDismembermentWound::SetTransform( const Vector* pos, const EulerAngles* rot, const Vector* scale )
{
	if ( !pos && !rot && !scale )
	{
		// Nothing to do
		return false;
	}

	if ( pos )
	{
		m_transform.SetPosition( *pos );
	}
	if ( rot )
	{
		m_transform.SetRotation( *rot );
	}
	if ( scale )
	{
		m_transform.SetScale( *scale );
	}

	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////


CEntityDismemberment::CEntityDismemberment()
{
}

CEntityDismemberment::~CEntityDismemberment()
{
	m_wounds.ClearPtrFast();
}


Bool CEntityDismemberment::HasWound( const CName& name ) const
{
	return FindWoundByName( name ) != nullptr;
}

#ifndef NO_EDITOR
CDismembermentWound* CEntityDismemberment::FindWoundByName( const CName& name )
{
	for ( CDismembermentWound* wound : m_wounds )
	{
		if ( wound->GetName() == name )
		{
			return wound;
		}
	}
	return nullptr;
}
#endif

const CDismembermentWound* CEntityDismemberment::FindWoundByName( const CName& name ) const
{
	for ( const CDismembermentWound* wound : m_wounds )
	{
		if ( wound->GetName() == name )
		{
			return wound;
		}
	}
	return nullptr;
}


#ifndef NO_EDITOR

Bool CEntityDismemberment::AddWound( const CName& name )
{
	if ( HasWound( name ) )
	{
		return false;
	}

	m_wounds.PushBack( new CDismembermentWound( name ) );

	return true;
}

Bool CEntityDismemberment::AddWound( CDismembermentWound* wound )
{
	if ( wound == nullptr || HasWound( wound->GetName() ) )
	{
		return false;
	}

	m_wounds.PushBack( wound );

	return true;
}


Bool CEntityDismemberment::RemoveWound( const CName& name )
{
	for ( Uint32 i = 0; i < m_wounds.Size(); ++i )
	{
		CDismembermentWound* wound = m_wounds[ i ];
		if ( wound->m_name == name )
		{
			m_wounds.RemoveAt( i );
			delete wound;
			return true;
		}
	}
	return false;
}

Bool CEntityDismemberment::SetWoundTransform( const CName& name, const Vector* pos, const EulerAngles* rot, const Vector* scale )
{
	CDismembermentWound* wound = FindWoundByName( name );
	if ( wound == nullptr )
	{
		return false;
	}

	return wound->SetTransform( pos, rot, scale );
}



void CEntityDismemberment::SetWoundDisabledForAppearance( const CName& woundName, const CName& appearanceName, Bool disabled )
{
	if ( disabled )
	{
		m_disabledWounds.PushBackUnique( SDismembermentWoundFilter( woundName, appearanceName ) );
	}
	else
	{
		m_disabledWounds.RemoveFast( SDismembermentWoundFilter( woundName, appearanceName ) );
	}
}

#endif // !NO_EDITOR

Bool CEntityDismemberment::IsWoundDisabledForAppearance( const CName& woundName, const CName& appearanceName ) const
{
	return m_disabledWounds.Exist( SDismembermentWoundFilter( woundName, appearanceName ) );
}


//////////////////////////////////////////////////////////////////////////
// Static helpers

// NOTE : These are not necessarily the most optimal implementations. For convenience they reference each other, which often means
// doing repeated searches through multiple CEntityDismemberments. In practice, I don't think this should matter too much, because
// they're only likely to be called when something is dismembered, which can happen just once for an entity in-game. If it becomes
// a bottleneck, then we can probably do these better.

Bool CEntityDismemberment::IsWoundDisabledRecursive( CEntity* entity, const CName& woundName )
{
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( entityTemplate == nullptr )
	{
		return false;
	}

	CAppearanceComponent* appearanceComponent = entity->FindComponent< CAppearanceComponent >();
	if ( appearanceComponent == nullptr )
	{
		return false;
	}

	const CName& currentAppearance = appearanceComponent->GetAppearance();

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		if ( dismember->IsWoundDisabledForAppearance( woundName, currentAppearance ) )
		{
			return true;
		}
		// If the wound is defined here, then we don't need to check further. If it's been disabled elsewhere, this one overrides it.
		if ( dismember->FindWoundByName( woundName ) )
		{
			return false;
		}
	}
	return false;
}

Bool CEntityDismemberment::IsWoundDisabledByIncludeRecursive( CEntity* entity, const CName& woundName )
{
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( entityTemplate == nullptr )
	{
		return false;
	}

	CAppearanceComponent* appearanceComponent = entity->FindComponent< CAppearanceComponent >();
	if ( appearanceComponent == nullptr )
	{
		return false;
	}

	const CName& currentAppearance = appearanceComponent->GetAppearance();

	const CEntityDismemberment* baseDismember = entityTemplate->FindParameter< CEntityDismemberment >( false, false );

	// If base dismember has a wound by this name, it overrides any includes, and therefore won't be disabled.
	if ( baseDismember != nullptr && baseDismember->FindWoundByName( woundName ) != nullptr )
	{
		return false;
	}

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		// Skip the base dismember, only consider those that have been included.
		if ( dismember == baseDismember )
		{
			continue;
		}
		
		if ( dismember->IsWoundDisabledForAppearance( woundName, currentAppearance ) )
		{
			return true;
		}
		// If the wound is defined here, then we don't need to check further. If it's been disabled elsewhere, this one overrides it.
		if ( dismember->FindWoundByName( woundName ) )
		{
			return false;
		}
	}
	return false;
}


void CEntityDismemberment::GetEnabledWoundNamesRecursive( CEntity* entity, TDynArray< CName >& outNames, EWoundTypeFlags woundTypeFlags /* = WTF_All */ )
{
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( entityTemplate == nullptr )
	{
		return;
	}

	CAppearanceComponent* appearanceComponent = entity->FindComponent< CAppearanceComponent >();

	CName currentAppearance = CName::NONE;
	if ( appearanceComponent != nullptr )
	{
		currentAppearance = appearanceComponent->GetAppearance();
	}

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		const TDynArray< CDismembermentWound* >& wounds = dismember->GetWounds();
		for ( const CDismembermentWound* wound : wounds )
		{
			if ( wound->GetTypeFlags() & woundTypeFlags )
			{
				if ( currentAppearance == CName::NONE || !IsWoundDisabledRecursive( entity, wound->GetName() ) )
				{
					outNames.PushBackUnique( wound->GetName() );
				}
			}
		}
	}
}

void CEntityDismemberment::GetEnabledWoundsRecursive( CEntity* entity, TDynArray< const CDismembermentWound* >& outWounds, EWoundTypeFlags woundTypeFlags /*= WTF_All*/ )
{
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( entityTemplate == nullptr )
	{
		return;
	}

	CAppearanceComponent* appearanceComponent = entity->FindComponent< CAppearanceComponent >();

	CName currentAppearance = CName::NONE;
	if ( appearanceComponent != nullptr )
	{
		currentAppearance = appearanceComponent->GetAppearance();
	}

	THashSet< CName > checkedWoundNames;

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		const TDynArray< CDismembermentWound* >& wounds = dismember->GetWounds();
		for ( const CDismembermentWound* wound : wounds )
		{
			if ( wound->GetTypeFlags() & woundTypeFlags )
			{
				if ( checkedWoundNames.Insert( wound->GetName() ) )
				{
					if ( currentAppearance == CName::NONE || !IsWoundDisabledRecursive( entity, wound->GetName() ) )
					{
						outWounds.PushBack( wound );
					}
				}
			}
		}
	}
}

const CDismembermentWound* CEntityDismemberment::FindWoundByNameRecursive( CEntityTemplate* entityTemplate, const CName& woundName )
{
	if ( entityTemplate == nullptr )
	{
		return nullptr;
	}

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		const CDismembermentWound* wound = dismember->FindWoundByName( woundName );
		if ( wound != nullptr )
		{
			return wound;
		}
	}
	return nullptr;
}

#ifndef NO_EDITOR
CDismembermentWound* CEntityDismemberment::FindNonConstWoundByNameRecursive( CEntityTemplate* entityTemplate, const CName& woundName )
{
	return const_cast< CDismembermentWound* >( FindWoundByNameRecursive( entityTemplate, woundName ) );
}
#endif

void CEntityDismemberment::GetAllWoundNamesRecursive( CEntityTemplate* entityTemplate, TDynArray< CName >& outNames )
{
	if ( entityTemplate == nullptr )
	{
		return;
	}

	TDynArray< const CEntityDismemberment* > dismembers;
	entityTemplate->GetAllParameters( dismembers );
	for ( const CEntityDismemberment* dismember : dismembers )
	{
		const TDynArray< CDismembermentWound* >& wounds = dismember->GetWounds();
		for ( const CDismembermentWound* wound : wounds )
		{
			outNames.PushBackUnique( wound->GetName() );
		}
	}
}
