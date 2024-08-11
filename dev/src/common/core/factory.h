/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"

/// Base resource factory interface
class IFactory : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFactory, CObject );

public:
	/// Resource creation options
	class FactoryOptions
	{
	public:
		CObject*		m_parentObject;			// NULL for normal resources, sometimes used for embedded resources

	public:
		RED_INLINE FactoryOptions()
			: m_parentObject( NULL )
		{};
	};

protected:
	CClass*						m_resourceClass;		// Resource class this creator supports

public:
	// Create resource
	virtual CResource* DoCreate( const FactoryOptions& options )=0;

	virtual String GetFriendlyName() const;

	// Check support
	Bool SupportsResource( CClass* resourceClass ) const;

	RED_INLINE CClass* GetResourceClass() const { return m_resourceClass; }


public:
	static IFactory* FindFactory( CClass* resourceClass );
	static void EnumFactoryClasses( TDynArray< CClass* >& factoryClasses );
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IFactory, CObject );