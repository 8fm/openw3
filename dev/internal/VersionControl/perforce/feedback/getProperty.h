/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_PERFORCE_FEEDBACK_GET_FILE_PROPERTY_H__
#define __VERCON_PERFORCE_FEEDBACK_GET_FILE_PROPERTY_H__

#include "base.h"

namespace VersionControl
{
	namespace Feedback
	{
		class GetProperty : public Base
		{
		public:
			GetProperty( const char* key );
			virtual ~GetProperty();

			inline const StrPtr& GetKey() const { return m_key; }
			inline const StrPtr& GetValue() const { return m_value; }

			// 
		private:
			virtual void OutputStat( StrDict* varList ) final;

		private:
			StrBuf m_key;
			StrBuf m_value;
		};
	}
}

#endif // __VERCON_PERFORCE_FEEDBACK_GET_FILE_PROPERTY_H__
