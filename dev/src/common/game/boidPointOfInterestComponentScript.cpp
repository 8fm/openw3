#include "build.h"
#include "boidPointOfInterestComponentscript.h"

IMPLEMENT_ENGINE_CLASS( CBoidPointOfInterestComponentScript );

CBoidPointOfInterestComponentScript::CBoidPointOfInterestComponentScript()
	: CBoidPointOfInterestComponent	()
{

}

void CBoidPointOfInterestComponentScript::OnUsed( Uint32 count, Float deltaTime )
{
	CallFunction( this, CNAME( OnUsed ), count, deltaTime );
}