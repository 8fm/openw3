/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_STANDARD_RAND_H_
#define _RED_STANDARD_RAND_H_

#include "../../redSystem/types.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			class StandardRand
			{
			public:
				StandardRand();
				~StandardRand();

				void Seed();
				void Seed( System::Random::SeedValue seed );

				template< typename TReturnType >
				TReturnType Get();

				template< typename TReturnType >
				TReturnType Max() const;
			};
		}
	}
}

#endif // _RED_STANDARD_RAND_H_
