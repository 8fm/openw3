/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_LOG_FILE_H_
#define _RED_LOG_FILE_H_

#include "logCommonOutputDevice.h"

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class File : public CommonOutputDevice
			{
			public:
				File( const Char* filename, Bool isPrimaryLogFile = false );
				virtual ~File();
				
				void Close();

			private:
				virtual void WriteFormatted( const Char* message );
				virtual void Flush();

			private:
				FILE* m_file;
			};
		}
	}
}

#endif //_RED_LOG_FILE_H_
