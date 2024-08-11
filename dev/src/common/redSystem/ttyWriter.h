/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_LOG_TTY_H_
#define _RED_LOG_TTY_H_

#include "logCommonOutputDevice.h"

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class TTYWriter : public CommonOutputDevice
			{
			public:
				TTYWriter();
				virtual ~TTYWriter();

			private:
				virtual void WriteFormatted( const Char* message );
			};
		}
	}
}

#endif //_RED_LOG_TTY_H_
