#include "build.h"
#include "beehive.h"

IMPLEMENT_ENGINE_CLASS( CBeehiveEntity );


void CBeehiveEntity::OnStreamIn()
{
	TBaseClass::OnStreamIn();
	CallEvent( CNAME( OnStreamIn ) );
}

void CBeehiveEntity::OnStreamOut()
{
	TBaseClass::OnStreamOut();
	CallEvent( CNAME( OnStreamOut ) );
}
