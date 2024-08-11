#pragma once

/// Input key definition
enum EInputKey : Int32
{
	IK_None=0,
	IK_LeftMouse=1,
	IK_RightMouse=2,
	IK_MiddleMouse=3,
	IK_Unknown04=4,
	IK_Unknown05=5,
	IK_Unknown06=6,
	IK_Unknown07=7,
	IK_Backspace=8,
	IK_Tab=9,
	IK_Unknown0A=10,
	IK_Unknown0B=11,
	IK_Unknown0C=12,
	IK_Enter=13,
	IK_Unknown0E=14,
	IK_Unknown0F=15,
	IK_Shift=16,
	IK_Ctrl=17,
	IK_Alt=18,
	IK_Pause=19,
	IK_CapsLock=20,
	IK_Unknown15=21,
	IK_Unknown16=22,
	IK_Unknown17=23,
	IK_Unknown18=24,
	IK_Unknown19=25,
	IK_Unknown1A=26,
	IK_Escape=27,
	IK_Unknown1C=28,
	IK_Unknown1D=29,
	IK_Unknown1E=30,
	IK_Unknown1F=31,
	IK_Space=32,
	IK_PageUp=33,
	IK_PageDown=34,
	IK_End=35,
	IK_Home=36,
	IK_Left=37,
	IK_Up=38,
	IK_Right=39,
	IK_Down=40,
	IK_Select=41,
	IK_Print=42,
	IK_Execute=43,
	IK_PrintScrn=44,
	IK_Insert=45,
	IK_Delete=46,
	IK_Help=47,
	IK_0=48,
	IK_1=49,
	IK_2=50,
	IK_3=51,
	IK_4=52,
	IK_5=53,
	IK_6=54,
	IK_7=55,
	IK_8=56,
	IK_9=57,
	IK_Unknown3A=58,
	IK_Unknown3B=59,
	IK_Unknown3C=60,
	IK_Unknown3D=61,
	IK_Unknown3E=62,
	IK_Unknown3F=63,
	IK_Unknown40=64,
	IK_A=65,
	IK_B=66,
	IK_C=67,
	IK_D=68,
	IK_E=69,
	IK_F=70,
	IK_G=71,
	IK_H=72,
	IK_I=73,
	IK_J=74,
	IK_K=75,
	IK_L=76,
	IK_M=77,
	IK_N=78,
	IK_O=79,
	IK_P=80,
	IK_Q=81,
	IK_R=82,
	IK_S=83,
	IK_T=84,
	IK_U=85,
	IK_V=86,
	IK_W=87,
	IK_X=88,
	IK_Y=89,
	IK_Z=90,
	IK_Unknown5B=91,
	IK_Unknown5C=92,
	IK_Unknown5D=93,
	IK_Unknown5E=94,
	IK_Unknown5F=95,
	IK_NumPad0=96,
	IK_NumPad1=97,
	IK_NumPad2=98,
	IK_NumPad3=99,
	IK_NumPad4=100,
	IK_NumPad5=101,
	IK_NumPad6=102,
	IK_NumPad7=103,
	IK_NumPad8=104,
	IK_NumPad9=105,
	IK_NumStar=106,
	IK_NumPlus=107,
	IK_Separator=108,
	IK_NumMinus=109,
	IK_NumPeriod=110,
	IK_NumSlash=111,
	IK_F1=112,
	IK_F2=113,
	IK_F3=114,
	IK_F4=115,
	IK_F5=116,
	IK_F6=117,
	IK_F7=118,
	IK_F8=119,
	IK_F9=120,
	IK_F10=121,
	IK_F11=122,
	IK_F12=123,
	IK_F13=124,
	IK_F14=125,
	IK_F15=126,
	IK_F16=127,
	IK_F17=128,
	IK_F18=129,
	IK_F19=130,
	IK_F20=131,
	IK_F21=132,
	IK_F22=133,
	IK_F23=134,
	IK_F24=135,
	IK_Pad_A_CROSS=136,
	IK_Pad_B_CIRCLE=137,
	IK_Pad_X_SQUARE=138,
	IK_Pad_Y_TRIANGLE=139,
	IK_Pad_Start=140,
	IK_Pad_Back_Select=141,
	IK_Pad_DigitUp=142,
	IK_Pad_DigitDown=143,
	IK_Pad_DigitLeft=144,
	IK_Pad_DigitRight=145,
	IK_Pad_LeftThumb=146,		// L3	
	IK_Pad_RightThumb=147,		// R3
	IK_Pad_LeftShoulder=148,	// L1
	IK_Pad_RightShoulder=149,	// R1
	IK_Pad_LeftTrigger=150,		// Axis	L2
	IK_Pad_RightTrigger=151,	// Axis	R2
	IK_Pad_LeftAxisX=152,		// Axis	ANALOG LEFT
	IK_Pad_LeftAxisY=153,		// Axis
	IK_Pad_RightAxisX=154,		// Axis	ANALOG RIGHT
	IK_Pad_RightAxisY=155,		// Axis
	IK_NumLock=156,
	IK_ScrollLock=157,
	IK_Unknown9E=158,
	IK_Unknown9F=159,
	IK_LShift=160,
	IK_RShift=161,
	IK_LControl=162,
	IK_RControl=163,
	IK_UnknownA4=164,
	IK_UnknownA5=165,
	IK_UnknownA6=166,
	IK_UnknownA7=167,
	IK_UnknownA8=168,
	IK_UnknownA9=169,
	IK_UnknownAA=170,
	IK_UnknownAB=171,
	IK_UnknownAC=172,
	IK_UnknownAD=173,
	IK_UnknownAE=174,
	IK_UnknownAF=175,
	IK_UnknownB0=176,
	IK_UnknownB1=177,
	IK_UnknownB2=178,
	IK_UnknownB3=179,
	IK_UnknownB4=180,
	IK_UnknownB5=181,
	IK_UnknownB6=182,
	IK_UnknownB7=183,
	IK_UnknownB8=184,
	IK_Unicode=185,
	IK_Semicolon=186,
	IK_Equals=187,
	IK_Comma=188,
	IK_Minus=189,
	IK_Period=190,
	IK_Slash=191,
	IK_Tilde=192,
	IK_Mouse4=193,
	IK_Mouse5=194,
	IK_Mouse6=195,
	IK_Mouse7=196,
	IK_Mouse8=197,
	IK_UnknownC6=198,
	IK_UnknownC7=199,
	IK_Joy1=200,
	IK_Joy2=201,
	IK_Joy3=202,
	IK_Joy4=203,
	IK_Joy5=204,
	IK_Joy6=205,
	IK_Joy7=206,
	IK_Joy8=207,
	IK_Joy9=208,
	IK_Joy10=209,
	IK_Joy11=210,
	IK_Joy12=211,
	IK_Joy13=212,
	IK_Joy14=213,
	IK_Joy15=214,
	IK_Joy16=215,
	IK_UnknownD8=216,
	IK_UnknownD9=217,
	IK_UnknownDA=218,
	IK_LeftBracket=219,
	IK_Backslash=220,
	IK_RightBracket=221,
	IK_SingleQuote=222,
	IK_UnknownDF=223,
	IK_UnknownE0=224,
	IK_UnknownE1=225,
	IK_UnknownE2=226,
	IK_UnknownE3=227,
	IK_MouseX=228,
	IK_MouseY=229,
	IK_MouseZ=230,
	IK_MouseW=231,
	IK_JoyU=232,
	IK_JoyV=233,
	IK_JoySlider1=234,
	IK_JoySlider2=235,
	IK_MouseWheelUp=236,
	IK_MouseWheelDown=237,
	IK_UnknownEE=238,
	IK_UnknownEF=239,
	IK_JoyX=240,
	IK_JoyY=241,
	IK_JoyZ=242,
	IK_JoyR=243,
	IK_UnknownF4=244,
	IK_UnknownF5=245,
	IK_Attn=246,
	IK_ClearSel=247,
	IK_ExSel=248,
	IK_ErEof=249,
	IK_Play=250,
	IK_Zoom=251,
	IK_NoName=252,
	IK_UnknownFD=253,
	IK_UnknownFE=254,
	IK_PS4_OPTIONS=255,
	IK_PS4_TOUCH_PRESS=256,
	IK_Last=257,
	IK_Count=258,

	IK_Pad_First	= IK_Pad_A_CROSS,
	IK_Pad_Last		= IK_Pad_RightAxisY,
};

class EInputKeyHashFunc
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const EInputKey key )
	{
		return ( Uint32 ) key;
	}
};

// class for retrieving a map of all input key enum values
class CInputKeys
{
public:
	static void FillInputKeyMap( THashMap< String, EInputKey >& mapStringEnums );
	static void FillInputKeyMap( THashMap< EInputKey, String, EInputKeyHashFunc >& mapStringEnums );

	static String MakeNameReadable( const String &keyCodeName );
};


BEGIN_ENUM_RTTI( EInputKey );
	ENUM_OPTION( IK_None );
	ENUM_OPTION( IK_LeftMouse );
	ENUM_OPTION( IK_RightMouse );
	ENUM_OPTION( IK_MiddleMouse );
	ENUM_OPTION( IK_Unknown04 );
	ENUM_OPTION( IK_Unknown05 );
	ENUM_OPTION( IK_Unknown06 );
	ENUM_OPTION( IK_Unknown07 );
	ENUM_OPTION( IK_Backspace );
	ENUM_OPTION( IK_Tab );
	ENUM_OPTION( IK_Unknown0A );
	ENUM_OPTION( IK_Unknown0B );
	ENUM_OPTION( IK_Unknown0C );
	ENUM_OPTION( IK_Enter );
	ENUM_OPTION( IK_Unknown0E );
	ENUM_OPTION( IK_Unknown0F );
	ENUM_OPTION( IK_Shift );
	ENUM_OPTION( IK_Ctrl );
	ENUM_OPTION( IK_Alt );
	ENUM_OPTION( IK_Pause );
	ENUM_OPTION( IK_CapsLock );
	ENUM_OPTION( IK_Unknown15 );
	ENUM_OPTION( IK_Unknown16 );
	ENUM_OPTION( IK_Unknown17 );
	ENUM_OPTION( IK_Unknown18 );
	ENUM_OPTION( IK_Unknown19 );
	ENUM_OPTION( IK_Unknown1A );
	ENUM_OPTION( IK_Escape );
	ENUM_OPTION( IK_Unknown1C );
	ENUM_OPTION( IK_Unknown1D );
	ENUM_OPTION( IK_Unknown1E );
	ENUM_OPTION( IK_Unknown1F );
	ENUM_OPTION( IK_Space );
	ENUM_OPTION( IK_PageUp );
	ENUM_OPTION( IK_PageDown );
	ENUM_OPTION( IK_End );
	ENUM_OPTION( IK_Home );
	ENUM_OPTION( IK_Left );
	ENUM_OPTION( IK_Up );
	ENUM_OPTION( IK_Right );
	ENUM_OPTION( IK_Down );
	ENUM_OPTION( IK_Select );
	ENUM_OPTION( IK_Print );
	ENUM_OPTION( IK_Execute );
	ENUM_OPTION( IK_PrintScrn );
	ENUM_OPTION( IK_Insert );
	ENUM_OPTION( IK_Delete );
	ENUM_OPTION( IK_Help );
	ENUM_OPTION( IK_0 );
	ENUM_OPTION( IK_1 );
	ENUM_OPTION( IK_2 );
	ENUM_OPTION( IK_3 );
	ENUM_OPTION( IK_4 );
	ENUM_OPTION( IK_5 );
	ENUM_OPTION( IK_6 );
	ENUM_OPTION( IK_7 );
	ENUM_OPTION( IK_8 );
	ENUM_OPTION( IK_9 );
	ENUM_OPTION( IK_Unknown3A );
	ENUM_OPTION( IK_Unknown3B );
	ENUM_OPTION( IK_Unknown3C );
	ENUM_OPTION( IK_Unknown3D );
	ENUM_OPTION( IK_Unknown3E );
	ENUM_OPTION( IK_Unknown3F );
	ENUM_OPTION( IK_Unknown40 );
	ENUM_OPTION( IK_A );
	ENUM_OPTION( IK_B );
	ENUM_OPTION( IK_C );
	ENUM_OPTION( IK_D );
	ENUM_OPTION( IK_E );
	ENUM_OPTION( IK_F );
	ENUM_OPTION( IK_G );
	ENUM_OPTION( IK_H );
	ENUM_OPTION( IK_I );
	ENUM_OPTION( IK_J );
	ENUM_OPTION( IK_K );
	ENUM_OPTION( IK_L );
	ENUM_OPTION( IK_M );
	ENUM_OPTION( IK_N );
	ENUM_OPTION( IK_O );
	ENUM_OPTION( IK_P );
	ENUM_OPTION( IK_Q );
	ENUM_OPTION( IK_R );
	ENUM_OPTION( IK_S );
	ENUM_OPTION( IK_T );
	ENUM_OPTION( IK_U );
	ENUM_OPTION( IK_V );
	ENUM_OPTION( IK_W );
	ENUM_OPTION( IK_X );
	ENUM_OPTION( IK_Y );
	ENUM_OPTION( IK_Z );
	ENUM_OPTION( IK_Unknown5B );
	ENUM_OPTION( IK_Unknown5C );
	ENUM_OPTION( IK_Unknown5D );
	ENUM_OPTION( IK_Unknown5E );
	ENUM_OPTION( IK_Unknown5F );
	ENUM_OPTION( IK_NumPad0 );
	ENUM_OPTION( IK_NumPad1 );
	ENUM_OPTION( IK_NumPad2 );
	ENUM_OPTION( IK_NumPad3 );
	ENUM_OPTION( IK_NumPad4 );
	ENUM_OPTION( IK_NumPad5 );
	ENUM_OPTION( IK_NumPad6 );
	ENUM_OPTION( IK_NumPad7 );
	ENUM_OPTION( IK_NumPad8 );
	ENUM_OPTION( IK_NumPad9 );
	ENUM_OPTION( IK_NumStar );
	ENUM_OPTION( IK_NumPlus );
	ENUM_OPTION( IK_Separator );
	ENUM_OPTION( IK_NumMinus );
	ENUM_OPTION( IK_NumPeriod );
	ENUM_OPTION( IK_NumSlash );
	ENUM_OPTION( IK_F1 );
	ENUM_OPTION( IK_F2 );
	ENUM_OPTION( IK_F3 );
	ENUM_OPTION( IK_F4 );
	ENUM_OPTION( IK_F5 );
	ENUM_OPTION( IK_F6 );
	ENUM_OPTION( IK_F7 );
	ENUM_OPTION( IK_F8 );
	ENUM_OPTION( IK_F9 );
	ENUM_OPTION( IK_F10 );
	ENUM_OPTION( IK_F11 );
	ENUM_OPTION( IK_F12 );
	ENUM_OPTION( IK_F13 );
	ENUM_OPTION( IK_F14 );
	ENUM_OPTION( IK_F15 );
	ENUM_OPTION( IK_F16 );
	ENUM_OPTION( IK_F17 );
	ENUM_OPTION( IK_F18 );
	ENUM_OPTION( IK_F19 );
	ENUM_OPTION( IK_F20 );
	ENUM_OPTION( IK_F21 );
	ENUM_OPTION( IK_F22 );
	ENUM_OPTION( IK_F23 );
	ENUM_OPTION( IK_F24 );
	ENUM_OPTION( IK_Pad_A_CROSS );
	ENUM_OPTION( IK_Pad_B_CIRCLE );
	ENUM_OPTION( IK_Pad_X_SQUARE );
	ENUM_OPTION( IK_Pad_Y_TRIANGLE );
	ENUM_OPTION( IK_Pad_Start );
	ENUM_OPTION( IK_Pad_Back_Select );
	ENUM_OPTION( IK_Pad_DigitUp );
	ENUM_OPTION( IK_Pad_DigitDown );
	ENUM_OPTION( IK_Pad_DigitLeft );
	ENUM_OPTION( IK_Pad_DigitRight );
	ENUM_OPTION( IK_Pad_LeftThumb );		// L3	
	ENUM_OPTION( IK_Pad_RightThumb );		// R3
	ENUM_OPTION( IK_Pad_LeftShoulder );	// L1
	ENUM_OPTION( IK_Pad_RightShoulder );	// R1
	ENUM_OPTION( IK_Pad_LeftTrigger );		// Axis	L2
	ENUM_OPTION( IK_Pad_RightTrigger );	// Axis	R2
	ENUM_OPTION( IK_Pad_LeftAxisX );		// Axis	ANALOG LEFT
	ENUM_OPTION( IK_Pad_LeftAxisY );		// Axis
	ENUM_OPTION( IK_Pad_RightAxisX );		// Axis	ANALOG RIGHT
	ENUM_OPTION( IK_Pad_RightAxisY );		// Axis
	ENUM_OPTION( IK_NumLock );
	ENUM_OPTION( IK_ScrollLock );
	ENUM_OPTION( IK_Unknown9E );
	ENUM_OPTION( IK_Unknown9F );
	ENUM_OPTION( IK_LShift );
	ENUM_OPTION( IK_RShift );
	ENUM_OPTION( IK_LControl );
	ENUM_OPTION( IK_RControl );
	ENUM_OPTION( IK_UnknownA4 );
	ENUM_OPTION( IK_UnknownA5 );
	ENUM_OPTION( IK_UnknownA6 );
	ENUM_OPTION( IK_UnknownA7 );
	ENUM_OPTION( IK_UnknownA8 );
	ENUM_OPTION( IK_UnknownA9 );
	ENUM_OPTION( IK_UnknownAA );
	ENUM_OPTION( IK_UnknownAB );
	ENUM_OPTION( IK_UnknownAC );
	ENUM_OPTION( IK_UnknownAD );
	ENUM_OPTION( IK_UnknownAE );
	ENUM_OPTION( IK_UnknownAF );
	ENUM_OPTION( IK_UnknownB0 );
	ENUM_OPTION( IK_UnknownB1 );
	ENUM_OPTION( IK_UnknownB2 );
	ENUM_OPTION( IK_UnknownB3 );
	ENUM_OPTION( IK_UnknownB4 );
	ENUM_OPTION( IK_UnknownB5 );
	ENUM_OPTION( IK_UnknownB6 );
	ENUM_OPTION( IK_UnknownB7 );
	ENUM_OPTION( IK_UnknownB8 );
	ENUM_OPTION( IK_Unicode );
	ENUM_OPTION( IK_Semicolon );
	ENUM_OPTION( IK_Equals );
	ENUM_OPTION( IK_Comma );
	ENUM_OPTION( IK_Minus );
	ENUM_OPTION( IK_Period );
	ENUM_OPTION( IK_Slash );
	ENUM_OPTION( IK_Tilde );
	ENUM_OPTION( IK_Mouse4 );
	ENUM_OPTION( IK_Mouse5 );
	ENUM_OPTION( IK_Mouse6 );
	ENUM_OPTION( IK_Mouse7 );
	ENUM_OPTION( IK_Mouse8 );
	ENUM_OPTION( IK_UnknownC6 );
	ENUM_OPTION( IK_UnknownC7 );
	ENUM_OPTION( IK_Joy1 );
	ENUM_OPTION( IK_Joy2 );
	ENUM_OPTION( IK_Joy3 );
	ENUM_OPTION( IK_Joy4 );
	ENUM_OPTION( IK_Joy5 );
	ENUM_OPTION( IK_Joy6 );
	ENUM_OPTION( IK_Joy7 );
	ENUM_OPTION( IK_Joy8 );
	ENUM_OPTION( IK_Joy9 );
	ENUM_OPTION( IK_Joy10 );
	ENUM_OPTION( IK_Joy11 );
	ENUM_OPTION( IK_Joy12 );
	ENUM_OPTION( IK_Joy13 );
	ENUM_OPTION( IK_Joy14 );
	ENUM_OPTION( IK_Joy15 );
	ENUM_OPTION( IK_Joy16 );
	ENUM_OPTION( IK_UnknownD8 );
	ENUM_OPTION( IK_UnknownD9 );
	ENUM_OPTION( IK_UnknownDA );
	ENUM_OPTION( IK_LeftBracket );
	ENUM_OPTION( IK_Backslash );
	ENUM_OPTION( IK_RightBracket );
	ENUM_OPTION( IK_SingleQuote );
	ENUM_OPTION( IK_UnknownDF );
	ENUM_OPTION( IK_UnknownE0 );
	ENUM_OPTION( IK_UnknownE1 );
	ENUM_OPTION( IK_UnknownE2 );
	ENUM_OPTION( IK_UnknownE3 );
	ENUM_OPTION( IK_MouseX );
	ENUM_OPTION( IK_MouseY );
	ENUM_OPTION( IK_MouseZ );
	ENUM_OPTION( IK_MouseW );
	ENUM_OPTION( IK_JoyU );
	ENUM_OPTION( IK_JoyV );
	ENUM_OPTION( IK_JoySlider1 );
	ENUM_OPTION( IK_JoySlider2 );
	ENUM_OPTION( IK_MouseWheelUp );
	ENUM_OPTION( IK_MouseWheelDown );
	ENUM_OPTION( IK_UnknownEE );
	ENUM_OPTION( IK_UnknownEF );
	ENUM_OPTION( IK_JoyX );
	ENUM_OPTION( IK_JoyY );
	ENUM_OPTION( IK_JoyZ );
	ENUM_OPTION( IK_JoyR );
	ENUM_OPTION( IK_UnknownF4 );
	ENUM_OPTION( IK_UnknownF5 );
	ENUM_OPTION( IK_Attn );
	ENUM_OPTION( IK_ClearSel );
	ENUM_OPTION( IK_ExSel );
	ENUM_OPTION( IK_ErEof );
	ENUM_OPTION( IK_Play );
	ENUM_OPTION( IK_Zoom );
	ENUM_OPTION( IK_NoName );
	ENUM_OPTION( IK_UnknownFD );
	ENUM_OPTION( IK_UnknownFE );
	ENUM_OPTION( IK_PS4_OPTIONS );
	ENUM_OPTION( IK_PS4_TOUCH_PRESS );
	ENUM_OPTION( IK_Last );
	ENUM_OPTION( IK_Count );
	ENUM_OPTION( IK_Pad_First );
	ENUM_OPTION( IK_Pad_Last );
END_ENUM_RTTI();


#define ENUM_TO_STRING( _enumValue ) TXT( #_enumValue )