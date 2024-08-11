#include "build.h"
#include "aiRedefinitionParameters.h"

///////////////////////////// CAIRedefinitionParameters /////////////////////////////
IMPLEMENT_ENGINE_CLASS( CAIRedefinitionParameters );

///////////////////////////// ICustomValAIParameters /////////////////////////////
IMPLEMENT_ENGINE_CLASS( ICustomValAIParameters );

void ICustomValAIParameters::SetCNameValue( CName value )
{
	CallFunction( this, CNAME( SetCNameValue ), value );
}