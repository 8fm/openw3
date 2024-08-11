/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestEntityMotionBlock : public CQuestGraphBlock
{
	friend class CQuestEntityMotion;

	DECLARE_ENGINE_CLASS( CQuestEntityMotionBlock, CQuestGraphBlock, 0 )

private:
	CName										m_entityTag;
	Float										m_duration;
	EngineTransform								m_targetTransform;
	Vector										m_positionDelta;
	EulerAngles									m_rotationDelta;
	Vector										m_scaleDelta;
	SSimpleCurve								m_animationCurve;

private:
	TInstanceVar< TGenericPtr >					i_motion;		//!< Instance pointer to the motion

public:
	CQuestEntityMotionBlock() : m_duration( 1 )
	{
		m_name = TXT("Entity Motion");
		m_animationCurve.Clear();
		m_animationCurve.SetLoop( false );
		m_animationCurve.AddPoint( 0.0f, 0.0f );
		m_animationCurve.AddPoint( 1.0f, 1.0f );
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 232, 182, 64 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestEntityMotionBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_entityTag, TXT( "Entity tag" )  );
	PROPERTY_EDIT( m_duration, TXT( "Duration of the motion in seconds" ) );
	PROPERTY_EDIT( m_targetTransform, TXT("Target transformation") );
	PROPERTY_EDIT( m_positionDelta, TXT("Additional position delta") );
	PROPERTY_EDIT( m_rotationDelta, TXT("Additional rotation delta") );
	PROPERTY_EDIT( m_scaleDelta, TXT("Additional scale delta") );
	PROPERTY_EDIT_RANGE( m_animationCurve, TXT("Animation curve"), 0.0f, 1.0f );
END_CLASS_RTTI()
