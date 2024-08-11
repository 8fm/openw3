#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storySceneLine.h"

namespace CStorySceneEventGeneratorInternals
{
	/*
	struct NegativeCondition : public Condition
	{
		Condition*	m_wrappedCondition;

		NegativeCondition( Condition* condition )
			: m_wrappedCondition( condition )
		{
		}

		virtual Bool Check( const Context& context ) const
		{
			if ( m_wrappedCondition )
			{
				return !m_wrappedCondition->Check( context );
			}
			return true;
		}
	};

	struct ElementIndexCondition : public Condition
	{
		Uint32 m_index;

		ElementIndexCondition( Uint32 index ) : m_index( index ) {}

		virtual Bool Check( const Context& context ) const
		{
			return context.GetElement() == context.GetSection()->GetElement( m_index );
		}
	};

	struct FirstElementCondition : public ElementIndexCondition
	{
		FirstElementCondition() : ElementIndexCondition( 0 ) {}
	};

	struct ElementTypeCondition : public Condition
	{
		CName m_typeName;

		ElementTypeCondition( const CName& typeName ) : m_typeName( typeName ) {}

		virtual Bool Check( const Context& context ) const
		{
			return context.GetElement()->GetClass()->GetName() == m_typeName;
		}
	};

	struct SceneLineCondition : public ElementTypeCondition
	{
		SceneLineCondition() : ElementTypeCondition( CNAME( CStorySceneLine ) ) {}
		//virtual Bool Check( const Context& context ) const
		//{
		//	return context.GetElement()->IsA< CStorySceneLine >();
		//}
	};

	struct ElementLengthCondition : public Condition
	{
		Float m_length;

		ElementLengthCondition( Float length ) : m_length( length ) {}

		virtual Bool Check( const Context& context ) const
		{
			return context.GetElement()->CalculateDuration()  > m_length;
		}

	};

	struct SpeakerWasSpeakingCondition : public Condition
	{
		virtual Bool Check( const Context& context ) const
		{
			return context.m_currentSpeakingActor == context.m_lastSpeakingActor;
		}
	};
	*/
}

