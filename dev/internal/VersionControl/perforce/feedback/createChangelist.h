/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_CREATE_CHANGELIST_H__
#define __VERCON_PERFORCE_FEEDBACK_CREATE_CHANGELIST_H__

#include "base.h"

namespace VersionControl
{
	namespace Feedback
	{
		class CreateChangelist : public Base
		{
		public:
			CreateChangelist();
			virtual ~CreateChangelist();

			void AddFile( StrPtr* file );
			void AddFile( const char* file );
			void SetDescription( const char* description );

		private:
			virtual void InputData( StrBuf *strbuf, ::Error *e ) final;

		private:
			StrBuf m_files;
			StrBuf m_description;
		};
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_CREATE_CHANGELIST_H__
