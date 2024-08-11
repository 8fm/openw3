/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemParameterFloat.h"
#include "fxTrackGroup.h"
#include "component.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemParameterFloat );

CFXTrackItemParameterFloat::CFXTrackItemParameterFloat()
	: CFXTrackItemCurveBase( 1 )
{
}

CFXTrackItemParameterFloatPlayData::CFXTrackItemParameterFloatPlayData( CComponent* component, const CFXTrackItemParameterFloat* trackItem, CName paramName, Bool restoreAtEnd )
	: IFXTrackItemPlayData( component, trackItem )
	, m_trackItem( trackItem )
	, m_restoreAtEnd( restoreAtEnd )
	, m_paramName( paramName )
	, m_previousValue( -FLT_MAX )
	, m_initialValue( 0.0f )
	, m_entity( nullptr )
{
	EffectParameterValue effectValue;
	if ( component->GetEffectParameterValue( m_paramName, effectValue) )
	{
		if ( effectValue.IsFloat() )
		{
			m_initialValue = effectValue.GetFloat();
			m_previousValue = m_initialValue;
		}
	}
};

CFXTrackItemParameterFloatPlayData::CFXTrackItemParameterFloatPlayData( CEntity* entity, const TDynArray< CComponent* >& components, const CFXTrackItemParameterFloat* trackItem, CName paramName, Bool restoreAtEnd )
	: IFXTrackItemPlayData( components[0], trackItem )
	, m_trackItem( trackItem )
	, m_restoreAtEnd( restoreAtEnd )
	, m_paramName( paramName )
	, m_entity( nullptr )
{
	// Many components version, initialize arrays
	m_components.Resize( components.Size() );
	m_initialValues.Resize( components.Size() );
	m_entity = entity;

	// Get values from all components
	for ( Uint32 i = 0; i < components.Size(); ++i )
	{
		// Keep reference
		m_components[i] = components[i];

		// Extract value
		EffectParameterValue effectValue;
		if ( components[i]->GetEffectParameterValue( m_paramName, effectValue ) )
		{
			if ( effectValue.IsFloat() )
			{
				m_initialValues[i] = effectValue.GetFloat();
			}
		}
	}

	entity->SetAllComponentsFloatParameter( this );
};

CFXTrackItemParameterFloatPlayData::~CFXTrackItemParameterFloatPlayData()
{
	if ( m_restoreAtEnd )
	{
		if ( m_components.Empty() )
		{
			CComponent *component = (CComponent*)( m_node );
			if ( component )
			{
				// Construct value
				EffectParameterValue effectValue;
				effectValue.SetFloat( m_initialValue ); 

				// Restore
				component->SetEffectParameterValue( m_paramName, effectValue );
			}
		}
		else
		{
			for ( Uint32 i = 0; i < m_components.Size(); ++i )
			{
				CComponent *component = m_components[i].Get();
				if ( component )
				{
					// Construct value
					EffectParameterValue effectValue;
					if( m_initialValues[i] != -FLT_MAX )
					{
						effectValue.SetFloat( m_initialValues[i] ); 
						component->SetEffectParameterValue( m_paramName, effectValue );
					}
				}
			}
		}
	}

	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		entity->DisableAllComponentsFloatParameter( this );
	}
}

void CFXTrackItemParameterFloatPlayData::AddComponent( CComponent* component ) 
{
	ASSERT( component );

	if ( !m_components.Exist( component ) )
	{
		m_components.PushBack( component );
		m_previousValue = -FLT_MAX;
		m_initialValues.PushBack( m_previousValue );

		EffectParameterValue effectValue;
		if ( component->GetEffectParameterValue( m_paramName, effectValue ) )
		{
			if ( effectValue.IsFloat() )
			{
				m_initialValues.Back() = effectValue.GetFloat();
			}
		}
	}
}

void CFXTrackItemParameterFloatPlayData::RemoveComponent( CComponent* component ) 
{
	ASSERT( component );

	auto index = m_components.GetIndex( component );
	if (index > -1 )
	{
		m_components.RemoveAt( index );
		m_initialValues.RemoveAt( index );
	}
}

void CFXTrackItemParameterFloatPlayData::OnPreComponentStreamOut( CComponent* component )
{
	ASSERT( component );
	RemoveComponent( component );
	if ( m_node && m_node->AsComponent() == component )
	{
		m_node = NULL;
	}
}

void CFXTrackItemParameterFloatPlayData::OnTick( const CFXState& fxState, Float timeDelta )
{
	// Evaluate curve value
	const Float val = m_trackItem->GetCurveValue( fxState.GetCurrentTime() );
	if ( val != m_previousValue )
	{
		// Keep for later...
		m_previousValue = val;

		// Create effect value
		EffectParameterValue effectValue;
		effectValue.SetFloat( val );

		// Apply
		if ( m_components.Empty() )
		{
			CComponent* component = (CComponent*)( m_node );
			if ( component )
			{
				component->SetEffectParameterValue( m_paramName, effectValue );
			}
		}
		else
		{
			for ( Uint32 i = 0; i < m_components.Size(); ++i )
			{
				CComponent* component = m_components[i].Get();
				if ( component )
				{
					// Apply parameter value
					component->SetEffectParameterValue( m_paramName, effectValue );
				}
			}
		}
	}
}

IFXTrackItemPlayData* CFXTrackItemParameterFloat::OnStart( CFXState& fxState ) const
{
	// Spawn runtime data
	if ( m_parameterName )
	{
		if ( !m_allComponents )
		{
			CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
			if ( component )
			{
				return new CFXTrackItemParameterFloatPlayData( component, this, m_parameterName, m_restoreAtEnd );
			}
		}
		else
		{
			CEntity* entity = fxState.GetEntity();
			if ( entity )
			{
				// TODO: filter out components that do not override SetEffectParameterValue (e.g. SoundEmitterComponent)
				const TDynArray< CComponent* >& components = entity->GetComponents();
				if ( !components.Empty() )
				{
					return new CFXTrackItemParameterFloatPlayData( entity, components, this, m_parameterName, m_restoreAtEnd );
				}
			}
		}
	}

	// Not applied
	return NULL;
}
