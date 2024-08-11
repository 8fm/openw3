/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_LOG_H_
#define _RED_MEMORY_LOG_H_

namespace red
{
namespace memory
{
	void FormatAndHandleLogRequest( const char * message, ... );
}
}

#ifdef RED_MEMORY_ENABLE_LOGGING

	#define RED_MEMORY_LOG( message, ... ) \
		do { red::memory::FormatAndHandleLogRequest( message, ##__VA_ARGS__ ); } while ( 0, 0 )

#else

	#define RED_MEMORY_LOG( message, ... ) do {} while ( 0, 0 )

#endif

#endif
