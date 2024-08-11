/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_GET_MULTIPLE_FILE_STATUS_H__
#define __VERCON_PERFORCE_FEEDBACK_GET_MULTIPLE_FILE_STATUS_H__

#include "base.h"
#include "getFileStatus.h"

#include "../../interface/fileList.h"

namespace VersionControl
{
	namespace Feedback
	{
		class GetDeletedFiles : public Base
		{
		public:
			GetDeletedFiles( Filelist* list );
			virtual ~GetDeletedFiles();

			// 
		private:
			virtual void OutputStat( StrDict* varList ) final;

		private:
			Filelist* m_list;
			GetFileStatus m_status;
		};
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_GET_MULTIPLE_FILE_STATUS_H__
