/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/
#pragma once

#ifdef RED_ASSERTS_ENABLED

// Asserts should be thought of as crashes, if you hit one, something has gone wrong that needs to be fixed as soon
// as possible. Asserts should verify that code is working correctly, and should not be used to validate data.
// Faulty data should be reported through the editor
// Continue Always is deprecated, to stop people ignoring asserts rather than dealing with them
// Currently, passing a message through as the second parameter is optional, this is to provide backwards
// All asserts should be defined with a message describing the problem
// and, if possible, a cause and potential fix

// ...and this time... it's serious. This kind of assert can't be just turned off from command line.
#define R6_ASSERT( expression, ... )																																			\
{																																												\
	static Red::System::Error::EAssertAction lastAction = Red::System::Error::AA_Break;																							\
	if ( lastAction != Red::System::Error::AA_ContinueAlways )																													\
	{																																											\
		if ( !( expression ) )																																					\
		{																																										\
			lastAction = Red::System::Error::HandleAssertion( MACRO_TXT( __FILE__ ), __LINE__, TXT( #expression ), 0, ##__VA_ARGS__ );							\
			if ( lastAction == Red::System::Error::AA_Break )																													\
			{																																									\
				RED_BREAKPOINT();																																				\
			}																																									\
		}																																										\
	}																																											\
}

#else // RED_ASSERTS_ENABLED

#define R6_ASSERT( ... )

#endif // RED_ASSERTS_ENABLED