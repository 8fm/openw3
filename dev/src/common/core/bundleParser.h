/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef IBUNDLE_PARSER_H_
#define IBUNDLE_PARSER_H_

class CAsyncLoadToken;
namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			class IBundleParser
			{
			public:
				virtual void Parse() = 0;
				virtual CAsyncLoadToken* CreateLoadToken( const String& absoluteBundlePath, void* dstBuffer ) = 0;
			};
		}
	}
}
#endif