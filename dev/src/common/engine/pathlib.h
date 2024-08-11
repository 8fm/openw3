#pragma once

///////////////////////////////////////////////////////////////////////////////
// This file contains all external interface of pathlib library.
///////////////////////////////////////////////////////////////////////////////

#include "pathlibConst.h"

RED_DISABLE_WARNING_MSC( 4481 )

// #define NO_EDITOR_PATHLIB_SUPPORT
#define PATHLIB_LOG( format, ... )			PathLib::CLog::GetInstance()->Log( PathLib::CLog::F_DEFAULT, format, ## __VA_ARGS__ ); RED_LOG( Path, format, ## __VA_ARGS__ );
#define PATHLIB_ERROR( format, ... )		PathLib::CLog::GetInstance()->Log( PathLib::CLog::F_WARNING, format, ## __VA_ARGS__ ); RED_LOG( Path, format, ## __VA_ARGS__ );

//#define PATHLIB_ASSERTS_ENABLED


#ifdef PATHLIB_ASSERTS_ENABLED
#define PATHLIB_ASSERT( ... ) RED_ASSERT ( ## __VA_ARGS__ )
#else
#define PATHLIB_ASSERT( ... )
#endif

#if defined( NO_EDITOR_PATHLIB_SUPPORT ) || defined( NO_EDITOR )
#define NO_NAVMESH_GENERATION
#endif
//#define DEBUG_NAVMESH_COLORS


/// Type of path engine collision for static meshes
enum EPathLibCollision
{
	PLC_Disabled,					//!< No collision at all
	PLC_Static,						//!< Mesh collision is cut from navigation mesh
	PLC_StaticWalkable,				//!< Mesh collision is cut from navigation mesh but we can walk on this mesh also
	PLC_StaticMetaobstacle,			//!< Mesh collision is cut from navigation but can be passed if creatures are ignoring them
	PLC_Dynamic,					//!< Path engine collision is precomputed but can be enabled/disbled dynamically during the game
	PLC_Walkable,					//!< Mesh used in generation of navmesh but don't create obstacles
	PLC_Immediate,					//!< Path engine collision is only spawned in-game
};

BEGIN_ENUM_RTTI( EPathLibCollision );
	ENUM_OPTION( PLC_Disabled );
	ENUM_OPTION( PLC_Static );
	ENUM_OPTION( PLC_StaticWalkable );
	ENUM_OPTION( PLC_StaticMetaobstacle );
	ENUM_OPTION( PLC_Dynamic );
	ENUM_OPTION( PLC_Walkable );
	ENUM_OPTION( PLC_Immediate );
END_ENUM_RTTI();



namespace PathLib
{

struct SCustomCollisionTester : public Red::System::NonCopyable
{
	virtual ~SCustomCollisionTester(){}
	virtual Bool IntersectLine( const Vector2& point1, const Vector2& point2 ) = 0;
	virtual Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) = 0;
	virtual void ComputeBBox( Box& outBBox ) = 0;
};

class CLog
{
public:
	enum ELineFlag
	{
		F_DEFAULT							= 0,
		F_WARNING							= FLAG( 1 ),
		F_RECEIVED							= FLAG( 2 )
	};
	typedef Red::Threads::CScopedLock< Red::Threads::CMutex > Lock;
protected:
	static const Int32 s_maxLines = 64;
	Red::Threads::CMutex			m_lock;
	Int8							m_lineFlags[ s_maxLines ];
	String							m_lines[ s_maxLines ];
	Int32							m_linesCount;
	Int32							m_currentLine;
	Red::Threads::CAtomic< Uint32 >	m_version;

	RED_INLINE Uint32 LineIndex( Int32 line ) const									{ return (s_maxLines+m_currentLine-line)%s_maxLines; }

	void Write( ELineFlag flag, const Char* text );

public:
	CLog();

	Red::Threads::CMutex& GetMutex()													{ return m_lock; }

	void Log( ELineFlag flag, const Char* format, ... );

	Int32 GetLinesCount()	 const 														{ return m_linesCount; }
	const String& GetLine( Int32 line ) const											{ return m_lines[ LineIndex( line ) ]; }
	ELineFlag GetLineFlag( Int32 line ) const											{ return ELineFlag(m_lineFlags[ LineIndex( line ) ]); }
	void MarkLineAsReceived( Int32 line )												{ m_lineFlags[ LineIndex( line ) ] |= F_RECEIVED; }

	RED_INLINE Uint32 GetCurrentVersion() const										{ return m_version.GetValue(); }

	static CLog* GetInstance();
};

class CVersionTracking : public Red::System::NonCopyable
{
protected:
	Uint16				m_initialVersion;
	Uint16				m_version;	

	~CVersionTracking(){}

public:
	CVersionTracking()
		: m_initialVersion( 0 )
		, m_version( 0 )													{}

	Uint32						GetVersion() const							{ return m_version; }
	Bool						IsInitialVersion() const					{ return m_version == m_initialVersion; }
	void						ResetVersion()								{ m_initialVersion = m_version; }
	void						SetVersion( Uint32 version )				{ m_version = Uint16(version); }
	void						MarkVersionDirty()							{ ++m_version; }
};

};		// namespace PathLib

