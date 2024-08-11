/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_BASE_H__
#define __VERCON_PERFORCE_FEEDBACK_BASE_H__

#include "../api.h"

namespace VersionControl
{
	namespace Feedback
	{
		class Base : public ClientUser
		{
		public:
			Base();
			virtual ~Base();

			void ClearErrors();

			inline int GetNumErrors() const { return m_errorCount; }
			inline const char* GetError() const { return m_errors.Text(); }
			inline const char* GetInfo() const { return m_info.Text(); }


			// 
		private:
			virtual void OutputError( const char* errBuf ) final;
			virtual void OutputInfo( char level, const char* data ) final;

		private:
			// Accumulated information messages
			StrBuf m_info;

			// Accumulated error messages
			StrBuf m_errors;
			int m_errorCount;
		};

		typedef Base Error;
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_BASE_H__
