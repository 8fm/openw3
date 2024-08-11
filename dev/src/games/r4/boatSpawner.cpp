#include "build.h"
#include "boatSpawner.h"

IMPLEMENT_ENGINE_CLASS( W3BoatSpawner );

void W3BoatSpawner::OnStreamIn()
{
	TBaseClass::OnStreamIn();

	if( IsInGame() )
	{
		CallEvent( CNAME( OnStreamIn ) );
	}
}

void W3BoatSpawner::OnStreamOut()
{
	TBaseClass::OnStreamOut();

	if( IsInGame() )
	{	
		CallEvent( CNAME( OnStreamOut ) );
	}
}

