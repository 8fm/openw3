/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "globalVisibilityId.h"
#include "umbraStructures.h"
#include "../core/configVar.h"

namespace Config
{
	TConfigVar< Bool >		cvDebugGlobalVisID( "Rendering/Debug", "DebugGlobalVisID", false );
}

namespace Helper
{
	RED_INLINE static Int32 QuantizeMatrixField( const Float x )
	{
		return (Int32)(x * 100.0f);
	}
}

IMPLEMENT_SIMPLE_RTTI_TYPE( GlobalVisID );

GlobalVisID::GlobalVisID()
	: m_id( 0 )
{
}

GlobalVisID::GlobalVisID( const class CResource* resource, const struct Matrix& localToWorld )
	: m_id( 0 )
{
	RED_FATAL_ASSERT( resource != nullptr, "Resource should be specified here" );

	// NOTE!: This should match Umbra calculations perfectly
	const String path = resource->GetDepotPath();
	if ( !path.Empty() )
	{
#ifdef USE_UMBRA
		const Uint32 transformHash = UmbraHelpers::CalculateTransformHash( localToWorld );
		m_id = UmbraHelpers::CompressToKeyType( GetHash( path ), transformHash );
#endif // USE_UMBRA
	}

#ifndef RED_FINAL_BUILD
	if ( Config::cvDebugGlobalVisID.Get() )
	{
		const Vector v = localToWorld.GetTranslation();
		const EulerAngles r = localToWorld.ToEulerAngles();
		const Vector s = localToWorld.GetScale33();
		m_debugString = String::Printf( TXT("res: '%ls', pos: [%1.3f,%1.3f,%1.3f], rot: [%1.3f,%1.3f,%1.3f], scale: [%1.3f,%1.3f,%1.3f]"),
			resource->GetDepotPath().AsChar(), 
			v.X, v.Y, v.Z, 
			r.Pitch, r.Yaw, r.Roll,
			s.X, s.Y, s.Z );
	}
#endif
}

GlobalVisID::GlobalVisID( const class CResource* resource, const class CComponent* comp )
	: m_id( 0 )
{
	RED_FATAL_ASSERT( resource != nullptr, "Resource should be specified here" );
	RED_FATAL_ASSERT( comp != nullptr, "Component should be specified here" );

	// NOTE!: This should match Umbra calculations perfectly
	const String path = resource->GetDepotPath();
	if ( !path.Empty() )
	{
#ifdef USE_UMBRA
		const Uint32 transformHash = UmbraHelpers::CalculateTransformHash( comp->GetLocalToWorld() );
		m_id = UmbraHelpers::CompressToKeyType( GetHash( path ), transformHash );
#endif // USE_UMBRA
	}

#ifndef RED_FINAL_BUILD
	if ( Config::cvDebugGlobalVisID.Get() )
	{
		const Vector v = comp->GetLocalToWorld().GetTranslation();
		const EulerAngles r = comp->GetLocalToWorld().ToEulerAngles();
		const Vector s = comp->GetLocalToWorld().GetScale33();
		m_debugString = String::Printf( TXT("res: '%ls', comp: '%ls', pos: [%1.3f,%1.3f,%1.3f], rot: [%1.3f,%1.3f,%1.3f], scale: [%1.3f,%1.3f,%1.3f]"),
			resource->GetDepotPath().AsChar(), 
			comp->GetFriendlyName().AsChar(), 
			v.X, v.Y, v.Z, 
			r.Pitch, r.Yaw, r.Roll,
			s.X, s.Y, s.Z );
	}
#endif
}


GlobalVisID::GlobalVisID( const Uint32 modelId, const struct Matrix& localToWorld )
{
#ifdef USE_UMBRA
	const Uint32 transformHash = UmbraHelpers::CalculateTransformHash( localToWorld );
	m_id = UmbraHelpers::CompressToKeyType( modelId, transformHash );
#else
	m_id = 0;
#endif // USE_UMBRA

#ifndef RED_FINAL_BUILD
	if ( Config::cvDebugGlobalVisID.Get() )
	{
		const Vector v = localToWorld.GetTranslation();
		const EulerAngles r = localToWorld.ToEulerAngles();
		const Vector s = localToWorld.GetScale33();
		m_debugString = String::Printf( TXT("modelID: '%d', pos: [%1.3f,%1.3f,%1.3f], rot: [%1.3f,%1.3f,%1.3f], scale: [%1.3f,%1.3f,%1.3f]"),
			modelId,
			v.X, v.Y, v.Z, 
			r.Pitch, r.Yaw, r.Roll,
			s.X, s.Y, s.Z );
	}
#endif
}

GlobalVisID::GlobalVisID( const GlobalVisID& other )
	: m_id( other.m_id )
#ifndef RED_FINAL_BUILD
	, m_debugString( other.m_debugString )
#endif
{
}

String GlobalVisID::ToString() const
{
#ifndef RED_FINAL_BUILD
	if ( !m_debugString.Empty() )
	{
		return String::Printf( TXT("[GlobalVisID: 0x%016llX]: %ls"), m_id, m_debugString.AsChar() );
	}
#endif

	return String::Printf( TXT("[GlobalVisID: 0x%016llX]"), m_id );
}

void GlobalVisID::SetDebugString( const String& debugString )
{
#ifndef RED_FINAL_BUILD
	m_debugString = debugString;
#endif
}
