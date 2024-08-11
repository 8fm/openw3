/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _XOR_SHIFT_H_
#define _XOR_SHIFT_H_

#include "../../redSystem/types.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			class XorShift
			{
			public:
				XorShift();
				~XorShift();

				void Seed();
				void Seed( System::Random::SeedValue seed );

				template< typename TReturnType >
				TReturnType Get();

				template< typename TReturnType >
				TReturnType Max() const;

			private:
				// state variables
				System::Random::SeedValue m_x;
				System::Random::SeedValue m_y;
				System::Random::SeedValue m_z;
				System::Random::SeedValue m_w;
			};
		}
	}
}

#endif // _XOR_SHIFT_H_
