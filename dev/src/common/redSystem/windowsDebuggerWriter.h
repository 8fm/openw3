/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_LOG_WINDOWS_DEBUGGER_H_
#define _RED_LOG_WINDOWS_DEBUGGER_H_

#include "logCommonOutputDevice.h"

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class WindowsDebuggerWriter : public CommonOutputDevice
			{
			public:
				WindowsDebuggerWriter();
				virtual ~WindowsDebuggerWriter();

			private:
				virtual void WriteFormatted( const Char* message );
			};
		}
	}
}

#endif //_RED_LOG_WINDOWS_DEBUGGER_H_
