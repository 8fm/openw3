#include "build.h"
#include "r4BoidSpecies.h"
#include "humbleCrittersLairEntity.h"
#include "flyingCrittersParams.h"

CR4BoidSpecies::CR4BoidSpecies()
	: CBoidSpecies()
{

}

CR4BoidSpecies::~CR4BoidSpecies()
{
	for (Uint32 i = 0; i < m_groupStateArray.Size(); ++i)
	{
		delete m_groupStateArray[ i ];
	}
	for (Uint32 i = 0; i < m_flyingPoiConfigArray.Size(); ++i)
	{
		delete m_flyingPoiConfigArray[ i ];
	}
	for ( Uint32 i = 0; i < m_poiConfigByGroupArray.Size(); ++i )
	{
		delete m_poiConfigByGroupArray[ i ];
	}
}

CBoidLairParams* CR4BoidSpecies::NewBoidSpeciesParams( CName className )
{
	if( className == CNAME( HumbleCritterLair ) )
	{
		return new CHumbleCritterLairParams( true );
	}
	else if ( className == CNAME( FlyingCritterLair ) )
	{
		return new CFlyingCritterLairParams( true );
	}
	else if ( className == CNAME( BoidLair ) )
	{
		return new CBoidLairParams( );
	}
	else if ( className == CNAME( SwarmLair ) )
	{
		return new CSwarmLairParams( );
	}
	return NULL;
}