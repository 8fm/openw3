/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiEventPackage.h"

namespace RedGui
{
	/*
	 * Is not perfect solution but it is working in all cases.
	 */

	template< typename Param0, typename Param1, typename Param2 >
	class TRedGuiCallback3
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui,MC_RedGuiInternals );
	public:
		virtual ~TRedGuiCallback3() = 0;
		virtual void invoke( Param0 param0, Param1 param1, Param2 param2 ) const = 0;
		virtual Bool operator==( const TRedGuiCallback3* const callback ) const = 0;
	};

	template< typename Param0, typename Param1, typename Param2 >
	TRedGuiCallback3< Param0, Param1, Param2 >::~TRedGuiCallback3()
	{
		/* intentionally empty */
	}
	
	template< typename Param0, typename Param1, typename Param2, typename T, typename Method >
	class TRedGuiMethodCallback3 : public TRedGuiCallback3< Param0, Param1, Param2 >
	{
	public:
		TRedGuiMethodCallback3( void* object, Method method )
			: m_object( object )
			, m_method( method ) 
		{ 
			/*intentionally empty*/ 
		}

		virtual ~TRedGuiMethodCallback3()
		{
			/* intentionally empty */
		}
	
		virtual void invoke( Param0 param0, Param1 param1, Param2 param2 ) const
		{
			T* obj = static_cast< T* >( m_object );
			return ( obj->*m_method )( param0, param1, param2 );
		}

		virtual Bool operator==( const TRedGuiCallback3< Param0, Param1, Param2 >* const callback ) const
		{
			const TRedGuiMethodCallback3* const callbackMethod = static_cast< const TRedGuiMethodCallback3* const >( callback );
			if( callbackMethod != nullptr )
			{
				if( callbackMethod->m_object == m_object )
				{
					if( callbackMethod->m_method == m_method )
					{
						return true;
					}
					return false;
				}
			}
			return false;
		}
	
	private:
		void*	m_object;
		Method	m_method;
	};
	
	template< typename Param0, typename Param1 >
	class TRedGuiDelegate3
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui,MC_RedGuiInternals );
	public:
		TRedGuiDelegate3()
		{ 
			/* intentionally empty */ 
		}
	
		~TRedGuiDelegate3() 
		{ 
			const Uint32 callbackCount = m_callbacks.Size();
			for( Uint32 i=0; i<callbackCount; ++i )
			{
				if( m_callbacks[i] != nullptr )
				{
					delete m_callbacks[i]; 
				}
			}
			m_callbacks.Clear();
		}

		template< typename T, typename Method >
		void Bind( T* object, Method method )
		{
			m_callbacks.PushBack( new TRedGuiMethodCallback3< CRedGuiEventPackage&, Param0, Param1, T, Method >( object, method ) );
		}

		template< typename T, typename Method >
		void Unbind( T* object, Method method )
		{
			const TRedGuiCallback3< CRedGuiEventPackage&, Param0, Param1 >* const tempCallback = new TRedGuiMethodCallback3< CRedGuiEventPackage&, Param0, Param1, T, Method >( object, method );
			const Uint32 callbackCount = m_callbacks.Size();
			for( Uint32 i=0; i<callbackCount; ++i )
			{
				if( m_callbacks[i]->operator==( tempCallback ) == true )
				{
					delete m_callbacks[i];
					m_callbacks.RemoveAt(i);
					break;
				}
			}
			delete tempCallback;
		}
	
		Bool operator()( CRedGuiEventPackage package, Param0 param0, Param1 param1 ) const
		{
			for( Uint32 i=0; i<m_callbacks.Size(); ++i )
			{
				if( m_callbacks[i] != nullptr )
				{
					m_callbacks[i]->invoke( package, param0, param1 );

					if( package.ProcessingIsStopped() == true )
					{
						break;
					}
				}
			}

			return package.IsProcessed();
		}
	
	private:
		TDynArray< const TRedGuiCallback3< CRedGuiEventPackage&, Param0, Param1 >* > m_callbacks;
	}; 
	
}	// namespace RedGui

#endif	// NO_RED_GUI
