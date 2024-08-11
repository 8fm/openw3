/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "movableRepresentation.h"


class CComponent;

///////////////////////////////////////////////////////////////////////////////
class CMRStack : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_AI );
private:
	CMovingAgentComponent&					m_host;
	Bool									m_active;
	CName									m_initialRepresentation;

	TDynArray< IMovableRepresentation* >	m_representations;

	Int32									m_activeRepresentationIdx;
	IMovableRepresentation*					m_activeRepresentation;

public:
	CMRStack( CMovingAgentComponent& host );

	IMovableRepresentation* OnAttached( IMovableRepresentation* initialRepresentation, Bool isActive, Vector& outStartPos, EulerAngles& outStartOrientation );

	void Activate( IMovableRepresentation* representation );

	void Add( IMovableRepresentation* representation, Bool activeInBackground );

	void Remove( IMovableRepresentation* representation );

	void Reset();

	void OnActivate( const Vector& position, const EulerAngles& orientation );

	void OnDeactivate();

	void OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation );

	void OnSeparate( const Vector& deltaPosition ); // simplified version of OnMove, to apply separation

	void OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ = false );

	void OnFollowPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation );

    void Update1_PreSeparation( Float timeDelta );
    void Update2_PostSeparation( Float timeDelta, Bool continousMovement = true );

	Vector GetStackPosition() const;
	EulerAngles GetStackOrientation() const;

	// For debug purposes
	CName GetName() const;

	RED_INLINE const Uint32 GetRepresentationsCount() { return m_representations.Size(); }

	RED_INLINE IMovableRepresentation* GetRepresentationByIndex( const Uint32 index ) const
	{
		if ( !m_representations.Empty() && index < m_representations.Size() )
		{
			return m_representations[ index ];
		}

		return nullptr;
	}

	RED_INLINE IMovableRepresentation* GetActiveRepresentation() const
	{
		return m_activeRepresentation;
	}

	// ------------------------------------------------------------------------
	// Saves representation
	// ------------------------------------------------------------------------
	void SaveState( IGameSaver* saver );
	void RestoreState( IGameLoader* loader );
};
///////////////////////////////////////////////////////////////////////////////
