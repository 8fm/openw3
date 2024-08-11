/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef EDITOR_AI_DEBUG
	#define AI_EVENT( _npc, _type, _result, _name, _description )			_npc->OnAIEvent( _type, _result, _name, _description ); 
	#define AI_EVENT_START( _npc, _type, _name, _description )				_npc->OnAIEventStart( _type, _name, _description );
	#define AI_EVENT_END( _npc, _type, _result )							_npc->OnAIEventEnd( _type, _result );

	#define COMPONENT_EVENT( _comp, _type, _result, _name, _description )	\
		{ CActor* actor = Cast< CActor >( (_comp)->GetEntity() );			\
		if ( actor ) {														\
		actor->OnAIEvent( _type, _result, _name, _description );			\
		} }

	#define COMPONENT_EVENT_START( _comp, _type, _name, _description )		\
		{ CActor* actor = Cast< CActor >( (_comp)->GetEntity() );			\
		if ( actor ) {														\
		actor->OnAIEventStart( _type, _name, _description );				\
		} }

	#define COMPONENT_EVENT_END( _comp, _type, _result )					\
		{ CActor* actor = Cast< CActor >( (_comp)->GetEntity() );			\
		if ( actor ) {														\
		actor->OnAIEventEnd( _type, _result );								\
		} }

#else
	#define AI_EVENT( _npc, _type, _result, _name, _description )
	#define AI_EVENT_START( _npc, _type, _name, _description )
	#define AI_EVENT_END( _npc, _type, _result )

	#define COMPONENT_EVENT( _comp, _type, _result, _name, _description )
	#define COMPONENT_EVENT_START( _comp, _type, _name, _description )
	#define COMPONENT_EVENT_END( _comp, _type, _result )
#endif
