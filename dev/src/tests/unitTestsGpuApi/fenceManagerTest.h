/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _FENCE_MANAGER_TEST_H_
#define _FENCE_MANAGER_TEST_H_

#include "testEngine.h"

class CFenceManagerTest : public CBaseTest
{
public:
	CFenceManagerTest( String& name ) : CBaseTest( name ) {}
	virtual ~CFenceManagerTest() {}

	virtual Bool Initialize() override { return true; }

	virtual Bool Execute( CTestUtils& context ) override;
};

#endif //_FENCE_MANAGER_TEST_H_
