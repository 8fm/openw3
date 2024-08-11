/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef DEFINE_OPCODE
#define UNDEF_DEFINE_OPCODE 
#define DEFINE_OPCODE(x) x,
enum {
#endif

DEFINE_OPCODE( OP_Nop );
DEFINE_OPCODE( OP_Null );
DEFINE_OPCODE( OP_IntOne );
DEFINE_OPCODE( OP_IntZero );
DEFINE_OPCODE( OP_IntConst );
DEFINE_OPCODE( OP_FloatConst );
DEFINE_OPCODE( OP_BoolTrue );
DEFINE_OPCODE( OP_BoolFalse );

#ifdef UNDEF_DEFINE_OPCODE
}
#undef UNDEF_DEFINE_OPCODE
#undef DEFINE_OPCODE
#endif