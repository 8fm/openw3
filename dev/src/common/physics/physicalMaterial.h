#pragma once

// Conversion for simple numeric types
template<> RED_INLINE Bool FromString( const String& text, Color& value )
{ 
	if( text == TXT( "BLACK" ) ) { value = Color::BLACK; return true; }
	else if( text == TXT( "WHITE" ) ) { value = Color::WHITE; return true; }
	else if( text == TXT( "RED" ) )	{ value = Color::RED; return true; }
	else if( text == TXT( "BLACK" ) ) { value = Color::BLACK; return true; }
	else if( text == TXT( "GREEN" ) ) { value = Color::GREEN; return true; }
	else if( text == TXT( "BLUE" ) ) { value = Color::BLUE; return true; }
	else if( text == TXT( "CYAN" ) ) { value = Color::CYAN; return true; }
	else if( text == TXT( "MAGENTA" ) ) { value = Color::MAGENTA; return true; }
	else if( text == TXT( "LIGHT_RED" ) ) { value = Color::LIGHT_RED; return true; }
	else if( text == TXT( "LIGHT_GREEN" ) ) { value = Color::LIGHT_GREEN; return true; }
	else if( text == TXT( "LIGHT_BLUE" ) ) { value = Color::LIGHT_BLUE; return true; }
	else if( text == TXT( "LIGHT_YELLOW" ) ) { value = Color::LIGHT_YELLOW; return true; }
	else if( text == TXT( "LIGHT_CYAN" ) ) { value = Color::LIGHT_CYAN; return true; }
	else if( text == TXT( "LIGHT_MAGENTA" ) ) { value = Color::LIGHT_MAGENTA; return true; }
	else if( text == TXT( "BROWN" ) ) { value = Color::BROWN; return true; }
	else if( text == TXT( "GRAY" ) ) { value = Color::GRAY; return true; }
	return false;
}

struct SPhysicalMaterial
{
	CName m_name;
	char* m_ansiName;
	float m_density;
	float m_soundDirectFilter;
	float m_soundReverbFilter;
	CName m_particleFootsteps;
	CName m_particleBodyroll;
	THandle< CObject > m_decalFootstepsDiff;
	THandle< CObject > m_decalFootstepsNormal;
	
	void* m_middlewareInstance;

#ifndef NO_EDITOR
	Color m_debugColor;
#endif

	// If > 0.5f the object with this material will float on the surface (the higher, the better swimmer it is)
	// If < 0.5f the object will drown (closer to 0 -> faster drowning)
	float m_floatingRatio;

	SPhysicalMaterial() : m_ansiName( nullptr ), m_middlewareInstance( 0 ), m_density( 0.0f ), m_soundDirectFilter( 0.0f ), m_soundReverbFilter( 0.0f ), m_floatingRatio(1), m_decalFootstepsDiff( nullptr ), m_decalFootstepsNormal( nullptr )  {};
	~SPhysicalMaterial();

	void SetName( const CName& name )
	{
		m_name = name;
		StringAnsi string = UNICODE_TO_ANSI( m_name.AsChar() );
		Uint32 size = string.Size();

		if( m_ansiName ) RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsMaterial, m_ansiName ); 
		m_ansiName = 0;

		m_ansiName = ( char* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsMaterial, size );
		Red::System::MemoryCopy( m_ansiName, string.AsChar(), size );
	}
};
