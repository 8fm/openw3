/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "log.h"
#include "../../redSystem/log.h"

namespace red
{
namespace memory
{
	void FormatAndHandleLogRequest( const char * message, ... )
	{
		char buffer[ 256 ];
		wchar_t ouputBuffer[ 256 ];

		va_list arglist;
		va_start( arglist, message );
		Red::System::VSNPrintF( buffer, 255, message, arglist );
		va_end( arglist );

		auto messageLength = Strlen( buffer, sizeof( buffer ) );

		u64 retval = 0;
		mbstowcs_s( &retval, ouputBuffer, messageLength + 1, buffer, messageLength );
		RED_LOG( RED_LOG_CHANNEL( Memory ), ouputBuffer );
	}
}
}
