/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/cooker.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/game/storySceneComponent.h"

class CEntityTemplateCooker : public ICooker
{
	DECLARE_RTTI_SIMPLE_CLASS( CEntityTemplateCooker );

public:

	// The cooker
	CEntityTemplateCooker()
	{
		m_resourceClass = ClassID< CEntityTemplate >();
		m_platforms.PushBack( PLATFORM_PC );
#ifndef WCC_LITE
		m_platforms.PushBack( PLATFORM_PS4 );
		m_platforms.PushBack( PLATFORM_XboxOne );
#endif
	}

	// Create resource
	virtual Bool DoCook( class ICookerFramework& cooker, const CookingOptions& options ) override;
};

DEFINE_SIMPLE_RTTI_CLASS( CEntityTemplateCooker, ICooker );
IMPLEMENT_ENGINE_CLASS( CEntityTemplateCooker );

Bool CEntityTemplateCooker::DoCook( class ICookerFramework& cooker, const CookingOptions& options )
{
	// Trying to cook resource that was already cooked
	if ( options.m_resource->HasFlag( OF_WasCooked ) )
	{
		WARN_WCC( TXT( "Resource '%s' was already cooked. Not cooking." ), options.m_resource->GetFriendlyName( ).AsChar( ) );
		return true;
	}

	CEntityTemplate* et = SafeCast< CEntityTemplate >( options.m_resource );
	if ( et )
	{
		// Compile full data buffer using our normal non-byteswapped buffer, but turn on proper byte swapping for full buffer
		et->CookDataBuffer( cooker, options.m_platform );

		// Cooked
		return true;
	}

	// Not cooked
	return false;
}

//////////////////////////////////////////////////////////////////////////