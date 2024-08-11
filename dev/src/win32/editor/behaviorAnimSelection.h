/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animationSelectionEditor.h"

class CBehaviorAnimSelection : public IAnimationSelectionEditor

{
public:
	CBehaviorAnimSelection( CPropertyItem* propertyItem );

protected:
	virtual CAnimatedComponent* RetrieveAnimationComponent() const;

};
