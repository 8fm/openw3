/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_GET_FILE_STATUS_H__
#define __VERCON_PERFORCE_FEEDBACK_GET_FILE_STATUS_H__

#include "base.h"
#include "../../interface/statusFlags.h"

namespace VersionControl
{
	namespace Feedback
	{
		class GetFileStatus : public Base
		{
		public:
			GetFileStatus();
			virtual ~GetFileStatus();

			inline FileStatus Get() const { return m_status; }

			virtual void OutputStat( StrDict* varList ) final;

		private:
			FileStatus m_status;
		};
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_GET_FILE_STATUS_H__
