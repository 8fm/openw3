/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "movableRepresentation.h"


class CMovingAgentComponent;

///////////////////////////////////////////////////////////////////////////////
class CMREntity : public IMovableRepresentation
{
private:
	CMovingAgentComponent&		m_host;
	Bool						m_wasPushable;

public:
	CMREntity( CMovingAgentComponent& host );

	virtual void OnInit( const Vector& position, const EulerAngles& orientation ) {}

	virtual void OnActivate( const Vector& position, const EulerAngles& orientation ) {}

	virtual void OnDeactivate() {}

	virtual void OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation ) override;

	virtual void OnSeparate( const Vector& deltaPosition ) override;

	virtual void OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ ) override;

	virtual Vector GetRepresentationPosition( Bool smooth = false ) const;

	virtual EulerAngles GetRepresentationOrientation() const;

	virtual CName GetName() const;

	virtual Bool IsAlwaysActive() const { return false; };
};
///////////////////////////////////////////////////////////////////////////////
