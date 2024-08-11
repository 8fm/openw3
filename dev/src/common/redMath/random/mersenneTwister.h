/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_MERSENNE_TWISTER_H_
#define _RED_MERSENNE_TWISTER_H_

#include "../../redSystem/types.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			class MersenneTwister
			{
			public:
				MersenneTwister();
				~MersenneTwister();

				void Seed();
				void Seed( System::Random::SeedValue seed );

				template< typename TReturnType >
				TReturnType Get();

				template< typename TReturnType >
				TReturnType Max() const;

			private:

				static const unsigned SIZE = 624;
				System::Uint32 m_seed[ SIZE ];
				System::Uint32 m_index;
			};
		}
	}
}

#endif // _RED_MERSENNE_TWISTER_H_
