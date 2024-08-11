/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------

#include "integerVector.h"

//---------------------------------------------------------------------------
#ifdef RED_FINAL_BUILD
	#define RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
#endif

//---------------------------------------------------------------------------

/// Masked trigger channels (keep in sync with trigger channels)
enum ETriggerChannel
{
	TC_Default			= FLAG( 0 ),		//!< Default group
	TC_Player			= FLAG( 1 ),		//!< Player
	TC_Camera			= FLAG( 2 ),		//!< Camera object
	TC_NPC				= FLAG( 3 ),		//!< General NPC
	TC_SoundReverbArea	= FLAG( 4 ),		//!< Sound source
	TC_SoundAmbientArea	= FLAG( 5 ),		//!< Sound ambient area external shell
	TC_Quest			= FLAG( 6 ),		//!< Used in quest conditions
	TC_Projectiles		= FLAG( 7 ),		//!< Projectiles' collisions
	TC_Horse			= FLAG( 8 ),		//!< Horse
	TC_Custom0			= FLAG( 16 ),		//!< Custom group
	TC_Custom1			= FLAG( 17 ),		//!< Custom group
	TC_Custom2			= FLAG( 18 ),		//!< Custom group
	TC_Custom3			= FLAG( 19 ),		//!< Custom group
	TC_Custom4			= FLAG( 20 ),		//!< Custom group
	TC_Custom5			= FLAG( 21 ),		//!< Custom group
	TC_Custom6			= FLAG( 22 ),		//!< Custom group
	TC_Custom7			= FLAG( 23 ),		//!< Custom group
	TC_Custom8			= FLAG( 24 ),		//!< Custom group
	TC_Custom9			= FLAG( 25 ),		//!< Custom group
	TC_Custom10			= FLAG( 26 ),		//!< Custom group
	TC_Custom11			= FLAG( 27 ),		//!< Custom group
	TC_Custom12			= FLAG( 28 ),		//!< Custom group
	TC_Custom13			= FLAG( 29 ),		//!< Custom group
	TC_Custom14			= FLAG( 30 ),		//!< Custom group
};

// Maks is defined as bitfield so we can have more than one at the same time
BEGIN_BITFIELD_RTTI( ETriggerChannel, 4 );
	BITFIELD_OPTION( TC_Default );
	BITFIELD_OPTION( TC_Player );
	BITFIELD_OPTION( TC_Camera );
	BITFIELD_OPTION( TC_NPC );
	BITFIELD_OPTION( TC_SoundReverbArea );
	BITFIELD_OPTION( TC_SoundAmbientArea );
	BITFIELD_OPTION( TC_Quest );
	BITFIELD_OPTION( TC_Projectiles );
	BITFIELD_OPTION( TC_Horse );
	BITFIELD_OPTION( TC_Custom1 );
	BITFIELD_OPTION( TC_Custom2 );
	BITFIELD_OPTION( TC_Custom3 );
	BITFIELD_OPTION( TC_Custom4 );
	BITFIELD_OPTION( TC_Custom5 );
	BITFIELD_OPTION( TC_Custom6 );
	BITFIELD_OPTION( TC_Custom7 );
	BITFIELD_OPTION( TC_Custom8 );
	BITFIELD_OPTION( TC_Custom9 );
	BITFIELD_OPTION( TC_Custom10 );
	BITFIELD_OPTION( TC_Custom11 );
	BITFIELD_OPTION( TC_Custom12 );
	BITFIELD_OPTION( TC_Custom13 );
	BITFIELD_OPTION( TC_Custom14 );
END_BITFIELD_RTTI();

//---------------------------------------------------------------------------

/// Callback interface for trigger system
class ITriggerCallback
{
public:
	virtual ~ITriggerCallback() {};

	//! Activator has entered trigger area
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator ) = 0;

	//! Activator has exited trigger area
	virtual void OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator ) = 0;
};

//---------------------------------------------------------------------------

/// Trigger system and terrain systme intergration interface
class ITriggerSystemTerrainQuery
{
public:
	virtual ~ITriggerSystemTerrainQuery() {};

	//! Returns true if given position in world space is above terrain
	virtual bool IsPositionAboveTerrain( const Vector& position ) const = 0;
};

//---------------------------------------------------------------------------

/// Trigger object construction info
struct CTriggerObjectInfo
{
	// Component (owner)
	class CAreaComponent* m_component;

	// Trigger geometry to use
	class CAreaShape* m_shape;

	// Initial placement
	Matrix m_localToWorld;

	// Include collision channels
	Uint32 m_includeChannels;

	// Excluded collision channels
	Uint32 m_excludeChannels;

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	// Debug name
	String m_debugName;
#endif

	// Optional trigger beveling radius (XY only)
	Float m_bevelRadius;

	// Optional trigger beveling radius (Z only)
	Float m_bevelRadiusVertical;

	// Is CCD allowed on this trigger
	Bool m_useCCD;

	// This trigger work above the terrain
	Bool m_allowAboveTerrain;

	// This trigger work below the terrain
	Bool m_allowBelowTerrain;

	// Callback interface
	class ITriggerCallback* m_callback;

public:
	CTriggerObjectInfo()
		: m_shape(NULL)
		, m_localToWorld(Matrix::IDENTITY)
		, m_component(NULL)
		, m_includeChannels(0)
		, m_excludeChannels(0)
		, m_bevelRadius(0.0f)
		, m_bevelRadiusVertical(0.0f)
		, m_callback(NULL)
		, m_useCCD( false )
		, m_allowAboveTerrain( true )
		, m_allowBelowTerrain( true )
	{};
};

//---------------------------------------------------------------------------

/// Activator construction info
struct CTriggerActivatorInfo
{
	// Component (owner)
	class CComponent* m_component;

	// Initial placement
	Matrix m_localToWorld;

	// Initial size (extents)
	Vector m_extents;

	// Include collision channels
	Uint32 m_channels;

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	// Debug name
	String m_debugName;
#endif

	// Maximum distance an activator can travel in one frame before we consider its movement discontinous
	Float m_maxContinousDistance;

	// Enable CCD for this activator
	Bool m_enableCCD;

public:
	CTriggerActivatorInfo()
		: m_localToWorld(Matrix::IDENTITY)
		, m_channels(0)
		, m_extents(0.3f, 0.3f, 0.9f)
		, m_component(NULL)
		, m_maxContinousDistance( 10.0f )
		, m_enableCCD( false )
	{};
};

//---------------------------------------------------------------------------

/// Base interface for trigger object
class ITriggerObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_TriggerSystem );

protected:
	virtual ~ITriggerObject() {};

public:
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	//! Get object debug name
	virtual const String& GetDebugName() const = 0;
#endif

	//! Get user component
	virtual class CComponent* GetComponent() const = 0;

	//! Get object's CCD flag
	virtual const Bool IsCCDEnabled() const = 0;

	//! Set trigger base position in trigger system (will move all shapes)
	virtual void SetPosition(const Matrix& matrix) = 0;

	//! Set trigger mask
	virtual void SetMask(const Uint32 inclusionMask, const Uint32 exclusionMask) = 0;

	//! Change the terrain flags
	virtual void SetTerrainMask(const Bool allowAboveTerrain, const Bool allowBelowTerrain) = 0;

	//! Enable/Disable CCD
	virtual void EnableCCD(const Bool isCCDEnabled) = 0;

	//! Render debug geometry
	virtual void Render(class CRenderFrame* frame) = 0;

	//! General point overlap test, returns true if world point is inside any of the convex pieces forming this shape
	virtual Bool TestPoint( const Vector& worldPoint ) const = 0;

	//! General box overlap test, returns true if specified box is inside any of the convex pieces forming this shape
	virtual Bool TestBox( const Vector& center, const Vector& extents ) const = 0;

	//! Trace AABB through the BSP
	virtual Bool TraceBox( const Vector& start, const Vector& end, const Vector& extents, Float& inTime, Float& outTime ) const = 0;

	//! Remove trigger from trigger system
	virtual void Remove() = 0;

	//! Release memory refernece
	virtual void Release() = 0;
};

//--------------------------------------------------------------------------

class ITriggerActivator
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	virtual ~ITriggerActivator() {};

public:
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	//! Get activator debug name
	virtual const String& GetDebugName() const = 0;
#endif

	//! Get user component
	virtual class CComponent* GetComponent() const = 0;

	//! Get activator trigger mask
	virtual const Uint32 GetMask() const = 0;

	//! Get activator CCD flag
	virtual const Bool IsCCDEnabled() const = 0;

	//! Change activator size (only cylinder is supported now)
	virtual void SetExtents(const Vector& extents) = 0;

	//! Set activator mask
	virtual void SetMask(const Uint32 mask) = 0;

	//! Enable/Disable CCD
	virtual void EnableCCD(const Bool isCCDEnabled) = 0;

	//! Move trigger to new location
	virtual void Move(const IntegerVector4& position, const Bool teleported = false) = 0;

	//! Render debug geometry
	virtual void Render(class CRenderFrame* frame) = 0;

	//! Remove activator from scene
	virtual void Remove() = 0;

	//! Release memory refernece
	virtual void Release() = 0;

	//! Get number of trigger objects this activator interacts with right now
	virtual const Uint32 GetNumOccupiedTrigger() const = 0;

	//! Get trigger object
	virtual const ITriggerObject* GetOccupiedTrigger(const Uint32 index) const = 0;

	//! Check if is inside trigger
	virtual const Bool IsInsideTrigger( const ITriggerObject* trigger ) const = 0;
};

//--------------------------------------------------------------------------

/// Trigger manager
class ITriggerManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_TriggerSystem );

public:
	typedef TStaticArray< const ITriggerObject*, 1024 > TResultTriggers;

public:
	virtual ~ITriggerManager() {};

	//! Set/remove terrain integration interface
	virtual void SetTerrainIntegration( ITriggerSystemTerrainQuery* queryInterface ) = 0;

	//! Create trigger object
	virtual ITriggerObject* CreateTrigger(const CTriggerObjectInfo& initInfo) = 0;

	//! Create trigger activator
	virtual ITriggerActivator* CreateActivator(const CTriggerActivatorInfo& initInfo) = 0;

	//! Process activator movements, generate entry/exit events and call the callback methods
	virtual void Update() = 0;

	//! Render debug geometry
	virtual void Render(class CRenderFrame* frame) = 0;

	//! Query triggers at given position
	//! NOTE: this requires preallocated buffer for the output (preferably on the stack)
	//! NOTE: triggers can be returned in any order
	virtual void GetTriggersAtPoint( const Vector& worldPosition, const Uint32 triggerChannelMask, TResultTriggers& outTriggers ) = 0;
	
public:
	//! Create trigger manager instance
	static ITriggerManager* CreateInstance();
};

//--------------------------------------------------------------------------

