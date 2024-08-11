/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_UTILITY_H_
#define _RED_UTILITY_H_

#include "os.h"
#include "types.h"

//////////////////////////////////////////////////////////////////////////
// Utility macros
#define FLT_TO_INT(f)		(*((Uint32*)&(f)))
#define INT_TO_FLT(i)		(*((Float*)&(i)))
#define ISPOW2(x)			(0 == ((x) & ((x)-1))) 
#define POW2_ROUNDUP(x,a)	(((x)+(a)-1)&(~((a)-1)))

#define ARRAY_COUNT(arr)		( sizeof( arr ) / sizeof( ( arr )[ 0 ] ) )
#define ARRAY_COUNT_U32(arr)	static_cast< Red::System::Uint32 >( ( sizeof( arr ) / sizeof( ( arr )[ 0 ] ) ) )

#define SAFE_RELEASE( x )	{ if ( x ) { (x)->Release(); }; x = NULL; }


// Flags
#define FLAG( x )		( 1 << ( x ) )
#define FLAG64( x )		( 1LL << ( x ) )

//////////////////////////////////////////////////////////////////////////
// Utility Classes
namespace Red
{
	namespace System
	{
		//////////////////////////////////////////////////////////////////////////
		// Inherit from this class to prevent your class from being copied
		class NonCopyable
		{
		protected:
			NonCopyable() {}
			~NonCopyable() {}

		private:
			// Do not implement the copy constructor or the assignment operator
			NonCopyable( const NonCopyable& );
			const NonCopyable& operator=( const NonCopyable& );
		};

		//////////////////////////////////////////////////////////////////////////
		// 
		template< typename T >
		struct ScopedFlag : public NonCopyable
		{
		private:
			T& m_flag;
			T  m_finalValue;
			
		public:
			ScopedFlag( T& flag, T finalValue )
			:	m_flag( flag ),
				m_finalValue( finalValue )
			{}

			~ScopedFlag() { m_flag = m_finalValue; }
		};
	}
}


#endif //_RED_UTILITY_H_
