/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CBgRootComponent : public CAnimatedComponent
{
	DECLARE_ENGINE_CLASS( CBgRootComponent, CAnimatedComponent, 0 );

public:
	CBgRootComponent();

	virtual void OnPostLoad();

	void AsyncUpdate( Float dt );

	void FireEvents();

protected:
	void FilterEvents();

	virtual Bool ShouldAddToTickGroups() const;

	RED_INLINE Bool IsEventForExJob( const CName& evtName ) const { return evtName == CNAME( take_item ) || evtName == CNAME( leave_item ); }
};

BEGIN_CLASS_RTTI( CBgRootComponent )
	PARENT_CLASS( CAnimatedComponent );
END_CLASS_RTTI();
