/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _UNIT_TEST_KERNEL_LOG_MOCK_H_
#define _UNIT_TEST_KERNEL_LOG_MOCK_H_

#include "../../common/redSystem/log.h"

namespace Red
{
namespace System
{
	class LogMock : public Log::Manager
	{
	public:
		MOCK_CONST_METHOD0( IsEnabled, Bool() );
		MOCK_METHOD1( Write, void(const Log::Message & message) );
	};
}
}

#endif
