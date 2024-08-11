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

	template< typename Param0 >
	class TRedGuiCallback1
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		virtual ~TRedGuiCallback1() = 0;
		virtual void invoke( Param0 param0 ) const = 0;
		virtual Bool operator==( const TRedGuiCallback1* const callback ) const = 0;
	};

	template< typename Param0 >
	TRedGuiCallback1< Param0 >::~TRedGuiCallback1()
	{
		/* intentionally empty */
	}

	template< typename Param0, typename T, typename Method >
	class TRedGuiMethodCallback1 : public TRedGuiCallback1< Param0 >
	{
	public:
		TRedGuiMethodCallback1( void *object, Method method )
			: m_object( object )
			, m_method( method ) 
		{ 
			/*intentionally empty*/ 
		}

		virtual ~TRedGuiMethodCallback1()
		{
			/* intentionally empty */
		}

		virtual void invoke( Param0 param0 ) const
		{
			T* obj = static_cast< T* >( m_object );
			return ( obj->*m_method )( param0 );
		}

		virtual Bool operator==( const TRedGuiCallback1< Param0 >* const callback ) const
		{
			const TRedGuiMethodCallback1* const callbackMethod = static_cast< const TRedGuiMethodCallback1* const >( callback );
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

	class TRedGuiDelegate1
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		TRedGuiDelegate1() 
		{ 
			/* intentionally empty */ 
		}

		~TRedGuiDelegate1(void) 
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
			m_callbacks.PushBack( new TRedGuiMethodCallback1< CRedGuiEventPackage&, T, Method >( object, method ) );
		}

		template< typename T, typename Method >
		void Unbind( T* object, Method method )
		{
			const TRedGuiCallback1< CRedGuiEventPackage& >* const tempCallback = new TRedGuiMethodCallback1< CRedGuiEventPackage&, T, Method >( object, method );
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

		Bool operator()( CRedGuiEventPackage package ) const
		{
			const Uint32 callbackCount = m_callbacks.Size();
			for( Uint32 i=0; i<callbackCount; ++i )
			{
				if( m_callbacks[i] != nullptr )
				{
					m_callbacks[i]->invoke( package );

					if( package.ProcessingIsStopped() == true )
					{
						break;
					}
				}
			}

			return package.IsProcessed();
		}

	private:
		TDynArray< const TRedGuiCallback1< CRedGuiEventPackage& >* > m_callbacks;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
