/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemMaterialOverride.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemMaterialOverride );

RED_DEFINE_STATIC_NAME( MaterialOverride )

/// Runtime player for material override
class CFXTrackItemMaterialOverridePlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemMaterialOverride*		m_track;
	Bool									m_isActive;

public:
	CFXTrackItemMaterialOverridePlayData( const CFXTrackItemMaterialOverride* trackItem, IMaterial* material, CEntity* entity, const CName exclusionTag, Bool drawOriginal )
		: IFXTrackItemPlayData( entity, trackItem )
		, m_track( trackItem )
	{
		m_isActive = entity->SetMaterialReplacement( material, drawOriginal, CName::NONE, exclusionTag, &m_track->GetIncludeList(), &m_track->GetExcludeList(), m_track->AreMeshAlternativesForced() );
	};

	~CFXTrackItemMaterialOverridePlayData()
	{
		CNode* node = GetNode();
		ASSERT( node );
		if ( node && m_isActive )
		{
			CEntity* entity = (CEntity*)( node );
			if ( entity )
			{
				entity->DisableMaterialReplacement();
			}
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		const Vector v = m_track->GetVectorFromCurve( fxState.GetCurrentTime() );

		CNode* node = GetNode();
		if ( node && m_isActive )
		{
			CEntity* entity = (CEntity*)( node );
			if ( entity )
			{
				entity->SendParametersMaterialReplacement( v );
			}
		}
		else
		{
			ASSERT( node );
		}
	}
};


CFXTrackItemMaterialOverride::CFXTrackItemMaterialOverride()
	: CFXTrackItemCurveBase( 4, CNAME( MaterialOverride ) )
	, m_drawOriginal( false )
	, m_forceMeshAlternatives( false )
{
}

IFXTrackItemPlayData* CFXTrackItemMaterialOverride::OnStart( CFXState& fxState ) const
{
	if ( !m_material )
	{
		ERR_ENGINE( TXT( "No material defined for material override track" ) );
		return NULL;
	}

	CEntity* entity = fxState.GetEntity();

	CFXTrackItemMaterialOverridePlayData *ppd = new CFXTrackItemMaterialOverridePlayData( this, m_material.Get(), entity, m_exclusionTag, m_drawOriginal );

	return ppd;

}

String CFXTrackItemMaterialOverride::GetCurveName( Uint32 i /* = 0 */ ) const
{
	switch( i )
	{
	case 0:
		return TXT("Param1");
	case 1:
		return TXT("Param2");
	case 2:
		return TXT("Param3");
	case 3:
		return TXT("Param4");
	default:
		return String::EMPTY;
	}
}
