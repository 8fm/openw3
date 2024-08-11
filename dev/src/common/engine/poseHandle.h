/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_ENGINE_POSE_HANDLE_H_
#define _RED_ENGINE_POSE_HANDLE_H_

struct SBehaviorGraphOutput;
class CPoseBlock;

class CPoseHandle 
{
public:

	CPoseHandle();
	explicit CPoseHandle( SBehaviorGraphOutput * pose, CPoseBlock * owner );	
	CPoseHandle( const CPoseHandle & copyFrom );
	CPoseHandle( CPoseHandle && rvalue );

	~CPoseHandle();

	SBehaviorGraphOutput * Get() const;

	SBehaviorGraphOutput * operator->() const;
	SBehaviorGraphOutput & operator*() const;

	void Reset();

	void Swap( CPoseHandle & swapWith );

	CPoseHandle & operator=( const CPoseHandle & copyFrom );
	CPoseHandle & operator=( CPoseHandle && rvalue );

	struct BoolConversion{ int valid; };
	typedef int BoolConversion::*bool_operator;
	operator bool_operator() const;
	bool operator!() const;

private:

	void SignalPoseAvailable();

	SBehaviorGraphOutput * m_pose;
	CPoseBlock * m_owner;
};

bool operator==( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr );
bool operator!=( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr );
bool operator<( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr );

#include "poseHandle.inl"

#endif
