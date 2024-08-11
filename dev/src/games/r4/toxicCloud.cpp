#include "build.h"
#include "toxicCloud.h"


IMPLEMENT_ENGINE_CLASS( W3ToxicCloud );

void W3ToxicCloud::OnStreamIn()
{
	TBaseClass::OnStreamIn();
	CallEvent( CNAME( OnStreamIn ) );
}

void W3ToxicCloud::OnStreamOut()
{
	TBaseClass::OnStreamOut();
	CallEvent( CNAME( OnStreamOut ) );
}

