/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "cooker.h"
#include "dependencyMapper.h"

#ifndef NO_RESOURCE_COOKING

IMPLEMENT_ENGINE_CLASS( ICooker );


ICookerFramework::~ICookerFramework()
{
}

ICookerFramework& ICookerFramework::Default( const ECookingPlatform platform )
{
	struct DefaultCookerFramework : public ICookerFramework 
	{
		ECookingPlatform m_platform;

		DefaultCookerFramework( ECookingPlatform platform )
			: m_platform( platform )
		{}

		virtual ECookingPlatform GetPlatform() const override { return m_platform; }
		virtual void CookingError( const CObject*, const Char*, ... ) override {}
		virtual void CookingWarning( const CObject*, const Char*, ... ) override {}
		virtual void ReportHardDependency( const String& ) override {}
		virtual void ReportSoftDependency( const String& ) override {}
		virtual void ReportCookingTime( const CClass*, const Double ) override {}

	};

	static DefaultCookerFramework defaultPlatforms[] = 
	{
		DefaultCookerFramework( PLATFORM_None ),
		DefaultCookerFramework( PLATFORM_Null ),
		DefaultCookerFramework( PLATFORM_Resave ),
		DefaultCookerFramework( PLATFORM_PC ),
		DefaultCookerFramework( PLATFORM_XboxOne ),
		DefaultCookerFramework( PLATFORM_PS4 ),
	};

	RED_FATAL_ASSERT( (Uint32)platform < ARRAY_COUNT(defaultPlatforms), "Invalid platform index" );
	return defaultPlatforms[ platform ];
}

void ICooker::SetEnabled( const Bool isEnabled )
{
	m_isEnabled = isEnabled;
}

Bool ICooker::SupportsResource( CClass* resourceClass ) const
{
	return resourceClass->IsBasedOn( m_resourceClass );
}

Bool ICooker::SupportsPlatform( ECookingPlatform platform ) const
{
	return m_platforms.Exist( platform );
}

ICooker* ICooker::FindCooker( CClass* resourceClass, ECookingPlatform platform )
{
	// Request all cooker classes
	TDynArray< CClass* > cookerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<ICooker>(), cookerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<cookerClasses.Size(); i++ )
	{
		ICooker* cooker = cookerClasses[i]->GetDefaultObject< ICooker >();
		if ( cooker->SupportsResource( resourceClass ) )
		{
			if ( cooker->SupportsPlatform( platform ) )
			{
				if ( cooker->IsEnabled() )
				{
					return cooker;
				}
			}
		}
	}

	// Resource type not supported
	return NULL;
}

void ICooker::EnumCookerClasses( TDynArray< CClass* >& resourceClasses )
{
	// Request importer classes
	TDynArray< CClass* > cookerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<ICooker>(), cookerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<cookerClasses.Size(); i++ )
	{
		ICooker* cooker = cookerClasses[i]->GetDefaultObject< ICooker >();
		if ( cooker->IsEnabled() )
		{
			resourceClasses.PushBackUnique( cooker->m_resourceClass );		
		}
	}
}

void ICooker::EnableCookerClasses( CClass* resourceClass, const Bool isEnabled )
{
	// Request importer classes
	TDynArray< CClass* > cookerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<ICooker>(), cookerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<cookerClasses.Size(); i++ )
	{
		ICooker* cooker = cookerClasses[i]->GetDefaultObject< ICooker >();
		if ( cooker->SupportsResource( resourceClass ) )
		{
			cooker->SetEnabled( isEnabled );
		}
	}
}

#endif
