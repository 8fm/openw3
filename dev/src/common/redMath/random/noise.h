/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NOISE_H_
#define _RED_NOISE_H_

#include "../../redSystem/types.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			class Noise
			{
			public:
				Noise();
				~Noise();

				void Seed();
				void Seed( System::Random::SeedValue seed );

				template< typename TReturnType >
				TReturnType Get() const;

				template< typename TReturnType >
				TReturnType Max() const;

			private:
				System::Random::SeedValue m_seed;
			};
		}
	}
}

#endif // _RED_NOISE_H_
