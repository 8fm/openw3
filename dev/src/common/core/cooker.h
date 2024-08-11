/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"

enum ECookingPlatform : Int32;

#ifndef NO_RESOURCE_COOKING

/// Cooker framework, placeholder class for now - I need to move some stuff from DependencySaver here
class ICookerFramework
{
public:
	virtual ~ICookerFramework();

	// get the cooking platform, will not be PLATFORM_None
	virtual ECookingPlatform GetPlatform() const = 0;

	// report fatal cooking error, resource path is tracked by the cooker and should not be passed
	// errors passed using this interface are tracked in the final cooker report
	virtual void CookingError( const CObject* subObject, const Char* message, ... ) = 0;

	// report cooking warning, resource path is tracked by the cooker and should not be passed
	// warnings passed using this interface are tracked in the final cooker report
	// do not abuse is for minor shit
	virtual void CookingWarning( const CObject* subObject, const Char* message, ... ) = 0;

	// track additional hard dependency of the resource being cooked
	// useful for tracking stuff that does is not reported properly via the standard dependencies
	virtual void ReportHardDependency( const String& depotPath ) = 0;

	// track additional soft dependency of the resource being cooked
	// useful for tracking stuff that does is not reported properly via the standard dependencies
	virtual void ReportSoftDependency( const String& depotPath ) = 0;

	// track cooking time
	virtual void ReportCookingTime( const CClass* objectClass, const Double timeTaken ) = 0;

	// Default cooking framework
	static ICookerFramework& Default( const ECookingPlatform platform );
};

/// Base, per resource cooker interface
class ICooker
{
	DECLARE_RTTI_SIMPLE_CLASS( ICooker );

public:
	ICooker()
		: m_isEnabled( true )
	{}

	/// Resource cooking options
	class CookingOptions
	{
	public:
		CResource*			m_resource;				//!< Resource to cook
		ECookingPlatform	m_platform;				//!< Target platform

	public:
		RED_INLINE CookingOptions( ECookingPlatform platform )
			: m_platform( platform )
		{};
	};

	// Is the cooker enabled ?
	RED_INLINE const Bool IsEnabled() const { return m_isEnabled; }

	// Create resource
	virtual Bool DoCook( class ICookerFramework& cooker, const CookingOptions& options )=0;

	// Enable/Disable the cooker, used for some legacy stuff (tempcook vs newcook)
	void SetEnabled( const Bool isEnabled );

	// Check resource class support
	Bool SupportsResource( CClass* resourceClass ) const;

	// Check platform support
	Bool SupportsPlatform( ECookingPlatform platform ) const;

	//---

	//! Find best cooker for given resource at given platform
	static ICooker* FindCooker( CClass* resourceClass, ECookingPlatform platform );

	//! List all cookers
	static void EnumCookerClasses( TDynArray< CClass* >& resourceClasses );

	//! Enable/Disable automatic cooking for given resource class
	static void EnableCookerClasses( CClass* resourceClass, const Bool isEnabled );

protected:
	CClass*							m_resourceClass;		// Resource class this cooker supports
	Bool							m_isEnabled;			// Is this cooker enabled ? Disabled cookers will not be used.
	TDynArray< ECookingPlatform >	m_platforms;			// Supported platforms
};

BEGIN_ABSTRACT_CLASS_RTTI( ICooker );
END_CLASS_RTTI();

#endif