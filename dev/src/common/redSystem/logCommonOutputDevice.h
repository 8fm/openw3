/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_LOG_COMMON_OUTPUT_DEVICE_H_
#define _RED_LOG_COMMON_OUTPUT_DEVICE_H_

#include "log.h"

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class CommonOutputDevice : public OutputDevice
			{
			public:
				CommonOutputDevice();
				virtual ~CommonOutputDevice();

				virtual void Write( const Message& message );

			protected:

				virtual void WriteFormatted( const Char* message ) = 0;
			};
		}
	}
}

#endif //_RED_LOG_COMMON_OUTPUT_DEVICE_H_
