/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "xorshift.h"

#define STATE_SEED_Y 72095012
#define STATE_SEED_Z 26857737
#define STATE_SEED_W 11816981

namespace Red { namespace Math {

	namespace Random {

		using namespace System;

		XorShift::XorShift()
		{
			Seed();
		}

		XorShift::~XorShift()
		{

		}

		void XorShift::Seed()
		{
			Seed( 0 );
		}

		void XorShift::Seed( System::Random::SeedValue seed )
		{
			m_x = seed;
			m_y = STATE_SEED_Y;
			m_z = STATE_SEED_Z;
			m_w = STATE_SEED_W;
		}

		template<>
		Float XorShift::Max() const
		{
			return 1.0f;
		}

		template<>
		Uint32 XorShift::Max() const
		{
			return 0x7fffffff;
		}

		template<>
		Uint32 XorShift::Get()
		{
			Uint32 t = m_x ^ (m_x << 11);
			m_x = m_y; m_y = m_z; m_z = m_w;
			return m_w = m_w ^ (m_w >> 19) ^ t ^ (t >> 8);
		}

		template<>
		Float XorShift::Get()
		{
			return Get< Uint32 >() * ( 1.0f / Max< Uint32 >() );
		}
	} // namespace Red { namespace Math {

} } // namespace Random {
