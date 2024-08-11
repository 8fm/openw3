/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_LOG

	#define BT_LOG( instance, node, logText )		AI_LOG( TXT( "BT actor %s node %s: %s" ), (instance).GetActorDebugName(), node->GetNodeName().AsString().AsChar(), logText );
	#define BT_LOG_RAW( format, ... )				AI_LOG( format, ## __VA_ARGS__ );
	#define BT_ERROR( instance, node, errorText )	AI_LOG( TXT( "BT actor %s node %s ERROR: %s" ), (instance).GetActorDebugName(), node->GetNodeName().AsString().AsChar(), errorText );
	#define BT_ERROR_RAW( format, ...  )			AI_LOG( format, ## __VA_ARGS__ );
	#define BT_LOG_NODE_RESULT( instance, node )	node->LogResult( instance );

#else

	#define BT_LOG( instance, node, logText )		
	#define BT_LOG_RAW( format, ... )				
	#define BT_ERROR( instance, node, errorText )	
	#define BT_ERROR_RAW( format, ...  )			
	#define BT_LOG_NODE_RESULT( instance, node )	

#endif