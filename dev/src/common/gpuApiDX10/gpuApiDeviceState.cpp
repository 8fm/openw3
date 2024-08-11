/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

namespace GpuApi
{
	// ----------------------------------------------------------------------
	// DeviceState structure

	DeviceState::DeviceState ()
	 : m_categories( 0 )
	{}

	DeviceState::DeviceState ( const DeviceState &other )
	 : m_categories( 0 )
	{
		*this = other;
	}

	DeviceState& DeviceState::operator= ( const DeviceState &other )
	{
		// This shit is quite hacky. I need non const object to easily manipulate reference counts,
		// so all addRef/Release are done on this (no 'other', given in parameter).
		// It works, because DeviceState always handles ref counting (there is no risk that
		// any of referenced objects will be destroyed).
		if ( this != &other )
		{
			// Release all refs
			ChangeAllRefCounts( false );

			// Import
			m_categories = other.m_categories;
			if ( DEVSTATECAT_RenderTargetSetup & other.m_categories )
			{
				m_renderTargetSetup = other.m_renderTargetSetup;
			}

			// Add references
			ChangeAllRefCounts( true );
		}

		return *this;
	}

	DeviceState::~DeviceState ()
	{
		ChangeAllRefCounts( false );
	}

	void DeviceState::ChangeAllRefCounts( bool inc )
	{
		if ( DEVSTATECAT_RenderTargetSetup & m_categories )
		{
			m_renderTargetSetup.ChangeAllRefCounts( inc );
		}		
	}

	void DeviceState::Clear()
	{
		*this = DeviceState ();
	}

	// ----------------------------------------------------------------------
	// GpuApi side implementation

	DeviceState GrabDeviceState( Uint32 categories )
	{
		DeviceState state;
		SDeviceData &dd = GetDeviceData();
		
		// Copy states

		if ( DEVSTATECAT_RenderTargetSetup & categories )
		{
			state.m_renderTargetSetup = dd.m_StateRenderTargetSetup;
		}

		state.m_categories = categories;

		// Add ref to all resources
		state.ChangeAllRefCounts( true );

		// Finish :)
		return state;
	}

	void GrabDeviceState( DeviceState &outState, Uint32 categories )
	{
		outState = GrabDeviceState( categories );
	}

	void RestoreDeviceState( const DeviceState &state, Uint32 categoriesMask )
	{
		Uint32 mergedCategories = (state.GetCategories() & categoriesMask);
		if ( mergedCategories )
		{
			if ( DEVSTATECAT_RenderTargetSetup & mergedCategories )
			{
				SetupRenderTargets( state.GetRenderTargetSetup() );
			}
		}
	}

	void RestoreDeviceState( const DeviceState &state )
	{
		if ( state.GetCategories() )
		{
			RestoreDeviceState( state, state.GetCategories() );
		}
	}

	// ----------------------------------------------------------------------
}
