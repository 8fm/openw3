/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EAIActionStatus
{
	ACTION_NotStarted = 0,
	ACTION_InProgress,
	ACTION_Successful,
	ACTION_Failed
};

BEGIN_ENUM_RTTI( EAIActionStatus )
	ENUM_OPTION( ACTION_NotStarted )
	ENUM_OPTION( ACTION_InProgress )
	ENUM_OPTION( ACTION_Successful )
	ENUM_OPTION( ACTION_Failed )
END_ENUM_RTTI()

#define DECLARE_AI_ACTION_CLASS( klass, baseClass )	 	\
	DECLARE_RTTI_SIMPLE_CLASS( klass )					\
	public: typedef baseClass TBaseClass;

class CAIAction abstract : public ISerializable
{
	DECLARE_AI_ACTION_CLASS( CAIAction, ISerializable )

	friend class CR6AISystem;
	friend class CR6BehTreeInstance;
	friend class CBehTreeNodeAIActionInstance;

protected:
	EAIActionStatus					m_status;
	CBehTreeNodeAIActionInstance*	m_nodeInstance;
#ifndef FINAL
	String							m_errorMsg;
#endif

public:
	CAIAction();

	RED_INLINE EAIActionStatus GetStatus() const { return m_status; }
	RED_INLINE CAIAction* Bind( CBehTreeNodeAIActionInstance* inst ) { m_nodeInstance = inst; return this; }
	CNode* FindActionTarget() const;

#ifndef FINAL
	RED_INLINE const String& GetErrorMsg() const { return m_errorMsg; }
	void SetErrorState( const Char* format, ... );
#else
	RED_INLINE const String& GetErrorMsg() const { return String::EMPTY; }
	RED_INLINE void SetErrorState( const Char* format, ... ) {}
#endif

	// methods to implement in derived classes
protected:
	virtual Bool CanBeStartedOn( CComponent* component ) const = 0;
	virtual EAIActionStatus StartOn( CComponent* component );
	virtual EAIActionStatus Tick( Float timeDelta );
	virtual EAIActionStatus Cancel( const Char* reason );
	virtual EAIActionStatus RequestInterruption();
	virtual EAIActionStatus Stop( EAIActionStatus newStatus );
	virtual EAIActionStatus Reset();
	virtual Bool ShouldBeTicked() const = 0;
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) {}
};

BEGIN_ABSTRACT_CLASS_RTTI( CAIAction )
	PARENT_CLASS( ISerializable )
	PROPERTY( m_status )
#ifndef FINAL
	PROPERTY_NOT_COOKED( m_errorMsg )
#endif
END_CLASS_RTTI()
