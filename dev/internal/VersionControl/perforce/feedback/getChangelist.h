/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_GET_CHANGELIST_H__
#define __VERCON_PERFORCE_FEEDBACK_GET_CHANGELIST_H__

#include "base.h"

namespace VersionControl
{
	namespace Feedback
	{
		class GetChangelist : public Base
		{
		public:
			GetChangelist();
			virtual ~GetChangelist();

			inline const StrBuf& Get() { return m_changelistNumber; }

			// 
		private:
			virtual void OutputStat( StrDict* varList ) final;

		private:
			StrBuf m_changelistNumber;
		};
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_GET_CHANGELIST_H__
