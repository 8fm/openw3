/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/stringWriter.h"

namespace Red
{

	// file output stream
	template< typename CH >
	class FileStreamWriter
	{
	public:
		FileStreamWriter( IFile* outputFile )
			: m_outputFile( outputFile )
		{}

		bool Flush( const Bool forced, const CH* data, const Red::System::Uint32 count )
		{
			if ( m_outputFile )
				m_outputFile->Serialize( (void*)data, count * sizeof(CH) );
			return true;
		}

		static FileStreamWriter& GetInstance()
		{
			static FileStreamWriter theInstance;
			return theInstance;
		}

		void Close()
		{
			delete m_outputFile;
			m_outputFile = nullptr;
		}

	private:
		IFile*		m_outputFile;
	};

	// string based file writer using some stack memory for buffer
	typedef FileStreamWriter< AnsiChar > CAnsiStringFileWriter;
	typedef Red::System::StackStringWriter< AnsiChar, 1024, FileStreamWriter< AnsiChar > > CAnsiStringFileStream;

} // Red