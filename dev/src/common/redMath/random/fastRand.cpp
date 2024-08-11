/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "fastRand.h"

namespace Red { namespace Math {

	namespace Random {

		using namespace System;

		FastRand::FastRand()
		{
			Seed();
		}

		FastRand::~FastRand()
		{

		}

		void FastRand::Seed()
		{
			Seed( 0 );
		}

		void FastRand::Seed( System::Random::SeedValue seed )
		{
			m_seed = seed;
		}

		template<>
		Float FastRand::Max() const
		{
			return 1.0f;
		}

		template<>
		Uint32 FastRand::Max() const
		{
			return 0x7FFF;
		}

		template<>
		Uint32 FastRand::Get()
		{
			m_seed = (214013 * m_seed + 2531011);
			return (m_seed >> 16) & 0x7FFF;
		}

		template<>
		Float FastRand::Get()
		{
			return Get< Uint32 >() * ( 1.0f / Max< Uint32 >() );
		}
	} // namespace Red { namespace Math {

} } // namespace Random {
