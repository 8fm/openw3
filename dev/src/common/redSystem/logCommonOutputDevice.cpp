/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "logCommonOutputDevice.h"

#include "utility.h"
#include "typetraits.h"
#include "crt.h"

#define RED_LOG_TIMESTAMPS
#define RED_LOG_CHANNELNAMES

namespace Red
{
namespace System
{
namespace Log
{

const Char * GPriorityName[ P_Count ] = 
{
	TXT( "Error" ),
	TXT( "Warning" ),
	TXT( "Info" ),
	TXT( "Spam" ) 
};


CommonOutputDevice::CommonOutputDevice()
:	Log::OutputDevice()
{

}

CommonOutputDevice::~CommonOutputDevice()
{

}

void CommonOutputDevice::Write( const Message& message )
{
	if( message.priority <= GetPriorityVisible() )
	{
		Char finalStringBuffer[ MAX_LINE_LENGTH ];
		Uint32 finalStringBufferUsed = 0;

	#ifdef RED_LOG_TIMESTAMPS
		finalStringBufferUsed += SNPrintF
			(
			finalStringBuffer,
			ARRAY_COUNT( finalStringBuffer ),
			TXT( "[%u.%02u.%02u %02u:%02u:%02u]" ),
			message.dateTime.GetYear(),
			message.dateTime.GetMonth() + 1,
			message.dateTime.GetDay() + 1,
			message.dateTime.GetHour(),
			message.dateTime.GetMinute(),
			message.dateTime.GetSecond()
			);
	#endif

	#ifdef RED_LOG_PRIORITY
	
		finalStringBufferUsed += SNPrintF( 
			&finalStringBuffer[ finalStringBufferUsed ], 
			MAX_LINE_LENGTH - finalStringBufferUsed,
			TXT( "[%" ) MACRO_TXT( RED_PRIs ) TXT( "]" ),
			GPriorityName[ message.priority ]
		);

	#endif

	#ifdef RED_LOG_CHANNELNAMES

		finalStringBufferUsed += SNPrintF( 
			&finalStringBuffer[ finalStringBufferUsed ], 
			MAX_LINE_LENGTH - finalStringBufferUsed,
			TXT( "[%" ) MACRO_TXT( RED_PRIs ) TXT( "] " ),
			message.channelText
		);

	#endif

		StringConcatenate( finalStringBuffer, message.text, ARRAY_COUNT( finalStringBuffer ) );
		StringConcatenate( finalStringBuffer, TXT( "\n" ), ARRAY_COUNT( finalStringBuffer ) );

		// And finally, write to the log file
		WriteFormatted( finalStringBuffer );
	}	
}

}
}
}
