/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IAnimationConstraint
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

public:
	virtual ~IAnimationConstraint();
	IAnimationConstraint( const CName controlVariable, const CName variableToControl, Float timeout );

	void Update( CBehaviorGraphInstance& instance, Float dt );

	Bool IsUnderControlBy( const CName controlVariable ) const;

	void Deactivate( CBehaviorGraphInstance& instance ) const;

public:
	virtual Bool IsFinished() const;

	virtual Vector GetControledValue( CBehaviorGraphInstance& instance ) const;

	virtual String ToString() const = 0;

protected:
	virtual void UpdateVariable( CBehaviorGraphInstance& instance ) = 0;
	Uint32 FindVariable( const String& name ) const;

protected:
	CName		m_variableToControl;
	CName		m_controlVariable;
	Float		m_timeout;
	Float		m_timer;
};

class CAnimationConstraint : public IAnimationConstraint
{
public:
	CAnimationConstraint( const CNode* target, 
						  const CName controlVariable, const CName variableToControl, 
						  Float timeout = 0.f );

	virtual void UpdateVariable( CBehaviorGraphInstance& instance );
	virtual Bool IsFinished() const;

	virtual String ToString() const;

protected:
	THandle< CNode >	m_target;
};

class CAnimationBoneConstraint : public IAnimationConstraint
{
public:
	CAnimationBoneConstraint( const CAnimatedComponent* target, Int32 bone,
							  const CName controlVariable, const CName variableToControl,
							  Bool useOffset, const Matrix& offsetMatrix,
							  Float timeout = 0.f );

	virtual void UpdateVariable( CBehaviorGraphInstance& instance );
	virtual Bool IsFinished() const;

	virtual String ToString() const;

protected:	
	Matrix							m_offsetMatrix;
	THandle< CAnimatedComponent >	m_target;
	Int32							m_boneIndex;
	Bool							m_useOffset;
};

class CAnimationConstraintStatic : public IAnimationConstraint
{
public:
	CAnimationConstraintStatic( const Vector &target, 
								const CName controlVariable, const CName variableToControl,
								Float timeout = 0.f );

	virtual void UpdateVariable( CBehaviorGraphInstance& instance );

	virtual String ToString() const;

protected:
	Vector	m_target;
};
