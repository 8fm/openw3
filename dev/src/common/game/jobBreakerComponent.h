/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CJobBreakerComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CJobBreakerComponent, CComponent, 0 );

	static const Float	UPDATE_INTERVAL;
	static const Float	BREAK_DISTANCE;
	static const Float	BREAK_WIDTH;
	static const Int32	MAX_PUSHED_ACTORS;

private:
	Float								m_distance;
	Float								m_radius;
	Float								m_timeToTick;
	THandle< CMovingAgentComponent >	m_movingAgentComponent;

protected:
	void OnAttachFinished( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;

	void TickImpl();

public:
	void OnTick( Float timeDelta ) override;

	virtual bool UsesAutoUpdateTransform() override { return false; }
};

BEGIN_CLASS_RTTI( CJobBreakerComponent );
	PARENT_CLASS( CComponent );	
	PROPERTY_EDIT( m_distance	, TXT( "Breaking distance" ) );
	PROPERTY_EDIT( m_radius		, TXT( "Breaking radius" )	 );
END_CLASS_RTTI();