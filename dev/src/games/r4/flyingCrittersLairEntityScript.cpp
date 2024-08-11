#include "build.h"
#include "flyingCrittersLairEntityScript.h"
#include "flyingSwarmGroup.h"

IMPLEMENT_ENGINE_CLASS( CFlyingCrittersLairEntityScript );

CFlyingCrittersLairEntityScript::CFlyingCrittersLairEntityScript()
	: CFlyingCrittersLairEntity()
{

}

void CFlyingCrittersLairEntityScript::OnScriptTick( CFlyingSwarmScriptInput& flyingScriptInput, Bool isActive, Float deltaTime )
{
	THandle< CFlyingSwarmScriptInput > handle( &flyingScriptInput );
	CallFunction( this, CNAME( OnTick ), handle, isActive, deltaTime );
}
void CFlyingCrittersLairEntityScript::OnBoidPointOfInterestReached( Uint32 count, CEntity *const entity, Float deltaTime )
{
	THandle< CEntity > handle( entity );
	CallFunction( this, CNAME( OnBoidPointOfInterestReached ), count, handle, deltaTime );
}

Uint32 CFlyingCrittersLairEntityScript::GetPoiCountByType( Boids::PointOfInterestType type )const
{
	TPointsMap::const_iterator itr = m_staticPointsOfInterest.Find( type );
	if ( itr != m_staticPointsOfInterest.End() )
	{
		const CPoiItem_Array& points = itr->m_second;
		return points.Size();
	}
	return 0;
}

void CFlyingCrittersLairEntityScript::funcGetPoiCountByType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, type, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_INT( GetPoiCountByType( type ) );
}

void CFlyingCrittersLairEntityScript::funcGetSpawnPointArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, spawnPointArray, TDynArray< CName >() );
	FINISH_PARAMETERS;
	if ( m_params )
	{
		spawnPointArray = m_params->m_spawnPointArray;
	}
}
