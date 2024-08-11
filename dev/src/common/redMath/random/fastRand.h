/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _FAST_RAND_H_
#define _FAST_RAND_H_

#include "../../redSystem/types.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			class FastRand
			{
			public:
				FastRand();
				~FastRand();

				void Seed();
				void Seed( System::Random::SeedValue seed );

				template< typename TReturnType >
				TReturnType Get();

				template< typename TReturnType >
				TReturnType Max() const;

			private:
				// state variables
				System::Random::SeedValue m_seed;
			};
		}
	}
}

#endif // _FAST_RAND_H
