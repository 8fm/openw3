
#include "build.h"
#include "compressedTranslation.h"

IMPLEMENT_RTTI_ENUM( ECompressTranslationType );
IMPLEMENT_RTTI_ENUM( ECompressedRotationType );

CompressedTranslation* CompressedTranslation::CreateCompressTranslation( ECompressTranslationType type )
{
	switch ( type )
	{
	case CT_8:
		return new CompressTranslation8();
	case CT_16:
		return new CompressTranslation16();
	case  CT_None:
		return NULL;
	default:
		ASSERT( 0 );
		return NULL;
	}
}
