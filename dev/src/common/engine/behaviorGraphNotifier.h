/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class CBehaviorGraphNode;
class CBehaviorGraphInstance;

enum EBehaviorGraphNotificationType
{
	BGNT_Activation,
	BGNT_Deactivation,
	BGNT_Update,	
};

class IBehaviorGraphNotifier : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphNotifier, CObject );
	
public:
	virtual void Notify( CBehaviorGraphInstance& instance, const CBehaviorGraphNode *node, EBehaviorGraphNotificationType type ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphNotifier );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

// do tego jeszcze GraphValue - zmienna float widoczna na zewn¹trz, read only, nie do uzycia przez graf