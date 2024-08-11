#include "build.h"
#include "movableRepresentationsStack.h"

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( CMRStack );

///////////////////////////////////////////////////////////////////////////////

CMRStack::CMRStack( CMovingAgentComponent& host )
: m_host( host )
, m_active( false )
, m_initialRepresentation( CName::NONE )
, m_activeRepresentationIdx( -1 )
, m_activeRepresentation( nullptr )
{
}

CName CMRStack::GetName() const 
{ 
	return CNAME( CMRStack );
}

IMovableRepresentation* CMRStack::OnAttached( IMovableRepresentation* initialRepresentation, Bool isActive, Vector& outStartPos, EulerAngles& outStartOrientation )
{
	// get a representation to activate
	IMovableRepresentation* representationToActivate = nullptr;
	ASSERT( !m_representations.Empty() && "There are no movable representations set up for this agent" );
	if ( !m_representations.Empty() )
	{
		// there are some representations - activate the desired one
		if ( m_initialRepresentation.Empty() == false )
		{
			// we have a representation type in mind - try finding it
			for ( TDynArray< IMovableRepresentation* >::iterator it = m_representations.Begin(); it != m_representations.End(); ++it )
			{
				if ( (*it)->GetName() == m_initialRepresentation )
				{
					representationToActivate = *it;
					break;
				}
			}
			m_initialRepresentation = CName::NONE;
		}

		if ( representationToActivate == nullptr )
		{
			representationToActivate = initialRepresentation;
		}
		
		if ( initialRepresentation == nullptr )
		{
			// No movable representation to activate initially - this can happen in the editor,
			// but should never happen in a game
			representationToActivate = nullptr;
		}
	}

	// activate the representation, providing we want to have it active
	m_active = isActive;
	if ( !isActive )
	{
		OnDeactivate();
	}
	if ( representationToActivate )
	{
		Activate( representationToActivate );
	}

	// get the start position
	if ( representationToActivate && m_active )
	{
		outStartPos = representationToActivate->GetRepresentationPosition();
		outStartOrientation = representationToActivate->GetRepresentationOrientation();
	}
	else
	{
		outStartPos = m_host.GetWorldPosition();
		outStartOrientation = m_host.GetWorldRotation();
	}

	return representationToActivate;
}

void CMRStack::Activate( IMovableRepresentation* representationToActivate )
{
	PC_SCOPE( CMR_Activate );

	const Uint32 count = m_representations.Size();

	// check for double activation
	if ( m_activeRepresentation != nullptr && m_activeRepresentation == representationToActivate )
	{
		WARN_GAME( TXT( "trying to active representation which one is activated!!" ) );
		return;
	}

	// get index
	Uint32 idx = 0;
	for ( idx = 0; idx < count; ++idx )
	{
		if ( m_representations[ idx ] == representationToActivate )
		{
			break;
		}
	}
	ASSERT( idx < count );
	if ( idx >= count )
	{
		ERR_GAME( TXT( "trying to active representation out of stack!!" ) );
		return;
	}
	m_activeRepresentationIdx = idx;

	// select representation
	IMovableRepresentation* prevActive = m_activeRepresentation;
	m_activeRepresentation = representationToActivate;

	if ( m_active )
	{
		// deactivate previous one
		if ( prevActive != nullptr )
		{
			prevActive->OnDeactivate();
		}

		// activate new one
		if ( prevActive != nullptr )
		{
			m_activeRepresentation->OnActivate( prevActive->GetRepresentationPosition(), prevActive->GetRepresentationOrientation() );
		}
		else
		{
			// probably isn't needed
			m_activeRepresentation->OnActivate( m_host.GetWorldPosition(), m_host.GetWorldRotation() );
		}
	}
}

void CMRStack::Add( IMovableRepresentation* representation, Bool activeInBackground )
{
	ASSERT( representation );
	if ( representation == nullptr )
	{
		ERR_GAME( TXT( "trying to add NULL representation!!" ) );
		return;
	}

	// add new representation if not exists
	m_representations.PushBackUnique( representation );

	// init
	if ( m_activeRepresentation )
	{
		representation->OnInit( m_activeRepresentation->GetRepresentationPosition(), m_activeRepresentation->GetRepresentationOrientation() );
	}
	else
	{
		representation->OnInit( m_host.GetWorldPosition(), m_host.GetWorldRotation() );
	}
}

void CMRStack::Remove( IMovableRepresentation* representation )
{
	ASSERT( representation );
	if ( representation == nullptr )
	{
		ERR_GAME( TXT( "trying to remove NULL representation!!" ) );
		return;
	}

	// get index of representation on stack
	Int32 idx = (Int32)m_representations.GetIndex( representation );
	RED_FATAL_ASSERT( idx < (Int32)m_representations.Size(), "bad id" );

	// check index
	if ( idx < m_activeRepresentationIdx )
	{
		m_activeRepresentationIdx--;
	}
	if ( idx == m_activeRepresentationIdx )
	{
		m_representations[ idx ]->OnDeactivate();
		m_activeRepresentationIdx = -1;
		m_activeRepresentation = nullptr;
	}

	// remove
	m_representations.RemoveAt( idx );
}

void CMRStack::Reset()
{
	m_representations.Clear();
	m_activeRepresentationIdx = -1;
	m_activeRepresentation = nullptr;

	// by default the stack is inactive so make it so
	m_active = false;
}

void CMRStack::OnActivate( const Vector& position, const EulerAngles& orientation )
{
	if ( !m_active )
	{
		if ( m_activeRepresentation )
		{
			m_activeRepresentation->OnActivate( position, orientation );
		}

		m_active = true;
	}
}

void CMRStack::OnDeactivate()
{
	if ( m_active )
	{
		// this method gets called when the entire stack gets deactivated - we need to deactivate the active representation
		if ( m_activeRepresentation )
		{
			m_activeRepresentation->OnDeactivate();
		}

		m_active = false;
	}
}

void CMRStack::OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation )
{
	ASSERT( m_activeRepresentation );
	ASSERT( m_active );

	PC_SCOPE( CMR_OnMove );

	if ( m_representations.Empty() || !m_activeRepresentation || !m_active )
	{
		return;
	}

	m_activeRepresentation->OnMove( deltaPosition, deltaOrientation );
}

void CMRStack::OnSeparate( const Vector& deltaPosition )
{
	ASSERT( m_activeRepresentation );
	ASSERT( m_active );

	PC_SCOPE( CMR_OnSeparate );

	if ( m_representations.Empty() || !m_activeRepresentation || !m_active )
	{
		return;
	}

	m_activeRepresentation->OnSeparate( deltaPosition );
}

void CMRStack::OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ )
{
	ASSERT( m_activeRepresentation );

	PC_SCOPE( CMR_OnSetPlacement );

	if ( m_representations.Empty() || !m_activeRepresentation )
	{
		return;
	}

	m_activeRepresentation->OnSetPlacement( timeDelta, newPosition, newOrientation, correctZ );
}

void CMRStack::OnFollowPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation )
{
	ASSERT( m_activeRepresentation );

	PC_SCOPE( CMR_OnFollowPlacement );

	if ( m_representations.Empty() )
	{
		return;
	}

	const Uint32 count = m_representations.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_representations[i]->OnFollowPlacement( timeDelta, newPosition, newOrientation );
	}
}

//////////////////////////////////////////////////////////////////////////

void CMRStack::Update1_PreSeparation( Float timeDelta )
{
	ASSERT( m_activeRepresentation );

	PC_SCOPE( CMR_Update1_PreSeparation );

	if ( m_representations.Empty() || !m_active || !m_activeRepresentation )
	{
		return;
	}

    m_activeRepresentation->Update1_PreSeparation( timeDelta );
}

//////////////////////////////////////////////////////////////////////////

void CMRStack::Update2_PostSeparation( Float timeDelta, Bool continousMovement )
{
	ASSERT( m_activeRepresentation );

	PC_SCOPE( Update2_PostSeparation );

	if ( m_representations.Empty() || !m_active || !m_activeRepresentation )
	{
		return;
	}

	Bool postMoved = m_activeRepresentation->Update2_PostSeparation( timeDelta, continousMovement );

	if ( m_host.HasMoved() || postMoved )
	{
		PC_SCOPE( Update2_PostSeparation_FollowPlacement );

		const Vector& actualNewPosition = m_activeRepresentation->GetRepresentationPosition( true );
		const EulerAngles& actualNewRotation = m_activeRepresentation->GetRepresentationOrientation();

		const Uint32 count = m_representations.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( i != m_activeRepresentationIdx )
			{
				m_representations[i]->OnFollowPlacement( timeDelta, actualNewPosition, actualNewRotation );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Vector CMRStack::GetStackPosition() const
{
	ASSERT( m_activeRepresentation );

	if ( !m_representations.Empty() && m_activeRepresentation && m_active )
	{
		return m_activeRepresentation->GetRepresentationPosition();
	}
	else
	{
		return m_host.GetWorldPosition();
	}
}

EulerAngles CMRStack::GetStackOrientation() const
{
	ASSERT( m_activeRepresentation );

	if ( !m_representations.Empty() &&m_activeRepresentation && m_active )
	{
		return m_activeRepresentation->GetRepresentationOrientation();
	}
	else
	{
		return m_host.GetWorldRotation();
	}
}

// ------------------------------------------------------------------------
// Saves representation
// ------------------------------------------------------------------------

void CMRStack::SaveState( IGameSaver* saver )
{
	/*CGameSaverBlock block0( saver, CNAME( CMRStack ) );

	Bool hasRepresentations = m_representations.Empty() == false;
	saver->WriteValue( CNAME( hasRepresentation ), hasRepresentations );

	if ( hasRepresentations )
	{	
		saver->WriteValue( CNAME( activeRepresentation ), m_representations[0]->GetName() );
	}*/
}

void CMRStack::RestoreState( IGameLoader* loader )
{
	/*CGameSaverBlock block0( loader, CNAME( CMRStack ) );

	Bool hasRepresentations = loader->ReadValue( CNAME( hasRepresentation ), false );
	if ( hasRepresentations )
	{	
		m_initialRepresentation = loader->ReadValue( CNAME( activeRepresentation ), CName::NONE );
	}
	else
	{
		m_initialRepresentation = CName::NONE;
	}*/
}

///////////////////////////////////////////////////////////////////////////////
