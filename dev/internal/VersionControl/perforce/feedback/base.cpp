/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "base.h"

namespace VersionControl
{
	namespace Feedback
	{
		Base::Base()
		:	m_errorCount( 0 )
		{

		}

		Base::~Base()
		{

		}

		void Base::ClearErrors()
		{
			m_errors.Clear();
			m_info.Clear();
		}

		void Base::OutputError( const char* errBuf )
		{
			m_errors.Append( errBuf );
			++m_errorCount;
		}

		void Base::OutputInfo( char level, const char* data )
		{
			m_info.Append(data);
		}
	}
}
