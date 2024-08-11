/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SHARED_PTR_H_
#define _RED_MEMORY_SHARED_PTR_H_

#include "operators.h"
#include "uniquePtr.h"

namespace red
{
	template< typename T >  
	class DefaultOwnership;

	template< typename, template< typename > class Ownership >
	class TWeakPtr;

	template< typename T, template< typename > class Ownership = DefaultOwnership >
	class TSharedPtr : public Ownership< T >
	{
	public:

		typedef Ownership< T > ParentType;
		typedef T * PtrType;
		typedef T & RefType;
	
		TSharedPtr();
		TSharedPtr( const TSharedPtr & copyFrom );
		TSharedPtr( TSharedPtr && rvalue );

		template< typename U >
		explicit TSharedPtr( U * pointer );	
		
		template< typename U >
		TSharedPtr( const TSharedPtr< U, Ownership > & copyFrom );

		template< typename U >
		TSharedPtr( TSharedPtr< U, Ownership > && rvalue );

		template< typename U >
		TSharedPtr( const TWeakPtr< U, Ownership > & copyFrom );

		template< typename U >
		TSharedPtr( TUniquePtr< U > && rvalue );

		~TSharedPtr();

		PtrType Get() const;

		PtrType operator->() const;
		RefType operator*() const;

		void Reset();
		void Reset( PtrType pointer );

		template< typename U >
		void Reset( U * pointer );

		void Swap( TSharedPtr & swapWith );

		TSharedPtr & operator=( const TSharedPtr & copyFrom );
		TSharedPtr & operator=( TSharedPtr && rvalue );
		
		template< typename U >
		TSharedPtr & operator=( const TSharedPtr< U, Ownership >  & copyFrom );

		template< typename U >
		TSharedPtr & operator=( TSharedPtr< U, Ownership > && rvalue );

		template< typename U >
		TSharedPtr & operator=( TUniquePtr< U > && rvalue );
			
		struct BoolConversion{ int valid; };
		typedef int BoolConversion::*bool_operator;
		operator bool_operator() const;
		bool operator!() const;
	};  

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	bool operator==( const TSharedPtr< LeftType, LeftOwnership> & leftPtr, const TSharedPtr< RightType, RightOwnership> & rightPtr );

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	bool operator!=( const TSharedPtr< LeftType, LeftOwnership > & leftPtr, const TSharedPtr< RightType, RightOwnership > & rightPtr );

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	bool operator<( const TSharedPtr< LeftType, LeftOwnership > & leftPtr, const TSharedPtr< RightType, RightOwnership > & rightPtr );

	struct DefaultOwnershipRefCount
	{
		typedef Int32 RefCountType;

		DefaultOwnershipRefCount( RefCountType s, RefCountType w )
			:	strong(s),
				weak(w)
		{}

		RefCountType strong;
		RefCountType weak;
	};

	template< typename T >  
	class DefaultOwnership
	{
	public: 
		
		typedef T * PtrType;
		typedef DefaultOwnershipRefCount::RefCountType RefCountType;

		RefCountType GetRefCount() const;
		RefCountType GetWeakRefCount() const;

	protected:

		DefaultOwnership();
		DefaultOwnership( PtrType pointer );

		DefaultOwnership( const DefaultOwnership & copyFrom );
		DefaultOwnership( DefaultOwnership && rvalue );

		template< typename U >
		DefaultOwnership( const DefaultOwnership< U > & copyFrom );

		template< typename U >
		DefaultOwnership( DefaultOwnership< U > && rvalue );

		void Release();
		void ReleaseWeak();
		void AddRef();
		void AddRefWeak();

		void UpgradeFromWeakToStrong( DefaultOwnership & upgradeFrom );

		void Swap( DefaultOwnership & swapWith );

		PtrType Get() const;

	private:

		void AssignRValue( DefaultOwnership && rvalue );

		DefaultOwnershipRefCount * m_refCount;
		PtrType m_pointee;
	};
}

#include "sharedPtr.hpp"

#endif
