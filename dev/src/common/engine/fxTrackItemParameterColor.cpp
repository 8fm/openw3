/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemParameterColor.h"
#include "fxTrackGroup.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemParameterColor );

CFXTrackItemParameterColor::CFXTrackItemParameterColor()
	: CFXTrackItemCurveBase( 4 )
{ }

/// Runtime player for color parameter track item
class CFXTrackItemParameterColorPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemParameterColor*	m_trackItem;		//!< Data
	CName								m_paramName;		//!< Param name to update
	Color								m_initialValue;		//!< Initial value of color
	Color								m_prevValue;		//!< Value that was last set
	Bool								m_restoreAtEnd;		//!< Restore on block end

public:
	CFXTrackItemParameterColorPlayData( CComponent* component, const CFXTrackItemParameterColor* trackItem, CName paramName, Bool restoreAtEnd )
		: IFXTrackItemPlayData( component, trackItem )
		, m_trackItem( trackItem )
		, m_paramName( paramName )
		, m_restoreAtEnd( restoreAtEnd )
	{
		EffectParameterValue effectValue;
		if ( component->GetEffectParameterValue( m_paramName, effectValue ) )
		{
			if ( effectValue.IsColor() )
			{
				m_initialValue = effectValue.GetColor();
				m_prevValue = m_initialValue;
			}
		}
	};

	~CFXTrackItemParameterColorPlayData()
	{
		if ( m_restoreAtEnd )
		{
			CComponent* component = (CComponent*)( m_node );
			if ( component )
			{
				// Create effect value
				EffectParameterValue effectValue;
				effectValue.SetColor( m_initialValue );

				// Restore initial value
				component->SetEffectParameterValue( m_paramName, effectValue );
			}
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		CComponent* component = (CComponent*)( m_node );
		if ( component )
		{
			// Evaluate curve value
			const Color val = m_trackItem->GetColorFromCurve( fxState.GetCurrentTime() );
			if ( val.R != m_prevValue.R || val.G != m_prevValue.G || val.B != m_prevValue.B || val.A != m_prevValue.A )
			{
				// Create effect value
				EffectParameterValue effectValue;
				effectValue.SetColor( val );

				// Apply parameter value
				component->SetEffectParameterValue( m_paramName, effectValue );

				// Remember
				m_prevValue = val;
			}
		}
	}

	//! Called when entity is partially unstreamed, causing certain components to be destroyed
	virtual void OnPreComponentStreamOut( CComponent* component )
	{
		ASSERT( component );
		if ( m_node && m_node->AsComponent() == component )
		{
			m_node = NULL;
		}
	}
};

IFXTrackItemPlayData* CFXTrackItemParameterColor::OnStart( CFXState& fxState ) const
{
	// Spawn runtime data
	if ( m_parameterName )
	{
		CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
		if ( component )
		{
			return new CFXTrackItemParameterColorPlayData( component, this, m_parameterName, m_restoreAtEnd );
		}
	}

	// Not applied
	return NULL;
}
