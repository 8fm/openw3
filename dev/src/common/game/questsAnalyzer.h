/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


struct SQuestBlockExecContext
{
	CQuestThread* thread;
	const CQuestGraphBlock* block;

	SQuestBlockExecContext() : thread( NULL ), block( NULL ) {}
	SQuestBlockExecContext( CQuestThread* _thread,
		const CQuestGraphBlock* _block ) 
		: thread( _thread )
		, block( _block ) 
	{}
};

typedef THashMap< String, SQuestBlockExecContext > ActiveQuestBlocksMap;

class CQuestsAnalyzer
{
private:
	CQuestThread* m_thread;
	ActiveQuestBlocksMap m_blocks;

public:
	CQuestsAnalyzer( CQuestThread& thread );

	void Update();

	RED_INLINE const ActiveQuestBlocksMap& GetBlocks() const { return m_blocks; }

	SQuestBlockExecContext* FindBlock( const String& blockName, Bool matchPart = false );

private:
	struct Node
	{
		String path;
		CQuestThread* thread;

		Node() : thread( NULL ) {}
		Node( const String& _path, CQuestThread* _thread )
			: path( _path )
			, thread( _thread )
		{}
	};
};
