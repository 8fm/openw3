/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CCrowdEntryPoint : public CEntity
{
	DECLARE_ENGINE_CLASS( CCrowdEntryPoint, CEntity, 0 );

public:
	void OnAttached( CWorld* world ) override;

	Box GetBoundingBox() const;
	Box2 GetBoundingBox2() const;
	Vector2 RandomPositionInside2() const;
};

BEGIN_CLASS_RTTI( CCrowdEntryPoint )
	PARENT_CLASS( CEntity );	
END_CLASS_RTTI();