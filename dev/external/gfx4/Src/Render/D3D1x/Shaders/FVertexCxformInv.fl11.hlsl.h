#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xyzw        1     NONE   float      w
// TEXCOORD                 1   xyzw        2     NONE   float      w
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[30]
// Embedded Data:
//  0x0000001e - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000019 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_input_ps linear v0.w
dcl_input_ps linear v1.w
dcl_input_ps linear v2.w
dcl_output o0.xyzw
mad o0.xyzw, v0.wwww, v2.wwww, v1.wwww
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[30], bundle is:[121] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FVertexCxformInv.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  8, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  8, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 52;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 0;
extUserElementCount  = 0;
NumVgprs             = 4;
NumSgprs             = 4;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000000
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b32     m0, s2                                  // 000000000000: BEFC0302
  v_interp_p1_f32  v2, v0, attr0.w                      // 000000000004: C8080300
  v_interp_p1_f32  v3, v0, attr1.w                      // 000000000008: C80C0700
  v_interp_p1_f32  v0, v0, attr2.w                      // 00000000000C: C8000B00
  v_interp_p2_f32  v2, v1, attr0.w                      // 000000000010: C8090301
  v_interp_p2_f32  v3, v1, attr1.w                      // 000000000014: C80D0701
  v_interp_p2_f32  v0, v1, attr2.w                      // 000000000018: C8010B01
  v_mac_legacy_f32  v3, v2, v0                          // 00000000001C: 0C060102
  v_cvt_pkrtz_f16_f32  v0, v3, v3                       // 000000000020: 5E000703
  s_nop         0x0000                                  // 000000000024: BF800000
  exp           mrt0, v0, v0, v0, v0 compr vm           // 000000000028: F800140F 00000000
  s_endpgm                                              // 000000000030: BF810000
end


// Approximately 2 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FVertexCxformInv[] =
{
     68,  88,  66,  67,  45, 175, 
    142, 168, 125,  69,  15, 171, 
     77, 239, 188, 248, 189, 234, 
     90, 173,   1,   0,   0,   0, 
     88,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    180,   0,   0,   0,  28,   1, 
      0,   0,  80,   1,   0,   0, 
    188,   3,   0,   0,  82,  68, 
     69,  70, 120,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     60,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  70, 
    111, 114,  32,  68, 117, 114, 
     97, 110, 103, 111,  32,  57, 
     46,  51,  48,  46,  49,  50, 
     48,  57,  56,  46,  48,   0, 
     73,  83,  71,  78,  96,   0, 
      0,   0,   3,   0,   0,   0, 
      8,   0,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   8,   0,   0,  86,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   8,   0,   0,  86,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
     15,   8,   0,   0,  67,  79, 
     76,  79,  82,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 100,   2, 
      0,   0,  80,   0,   0,   0, 
    153,   0,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     30,   0,   0,   0,  80,   0, 
      0,   0,  25,   0,   0,   0, 
    106,   8,   0,   1,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,  32,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   2,   0, 
      0,   0, 246,  31,  16,   0, 
      1,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    123,   0,   0,   0,  21,   0, 
      1,   0,  41, 203,   9,   0, 
    120,   1, 237,  86, 191,  75, 
    195,  64,  24, 189,  52, 218, 
    218,  90, 109,   7,   7,   5, 
      7,   5, 135, 186,  20, 170, 
    157, 156,  10, 214, 130, 171, 
    130, 131,  28,  98, 106,  83, 
     21, 211,  20, 210,  82, 162, 
    131, 232, 226, 236,  95, 224, 
    230,  32, 250,  23,  56, 168, 
    131,  67, 160, 171, 174,  14, 
    206, 142,  14,  29,   4, 253, 
    222, 229,  14, 106,  90, 117, 
     41,  34, 216,   7, 239,  94, 
    222,  93, 238, 203, 253, 226, 
    203,  53, 163,  76, 160, 245, 
    152, 189, 129, 238,  63, 111, 
    207, 150,  72,   7, 137, 154, 
     84, 192,  29,  66, 201, 216, 
    149, 212,  20,  10, 188,  16, 
     98, 108, 128,  68,  39, 102, 
    137,  57,  34, 218,  20, 125, 
    228, 238,  38, 168,  84, 241, 
    108,  98, 129,  24,  35, 162, 
     14,  33,  41, 140, 120,  14, 
     66, 213, 117, 107,  67,  31, 
    133,  48,  10, 137, 142, 119, 
    103,  80, 116, 199,  48,  17, 
    227, 255,  10, 114, 186,   2, 
    136, 171, 190, 249,  93, 159, 
    223,  66, 199,  60,   9, 106, 
    159,   0, 236,  71,  59,  70, 
     81, 252,   0, 204,  11, 188, 
     35, 126, 133, 119,   2, 180, 
     27, 112,  14, 194, 114, 213, 
    194, 218,  56, 155, 130, 134, 
     82, 226,  92, 252,  23,  96, 
    253,   0, 172,   5, 206,  75, 
    146, 152,  32,  42, 160, 221, 
     95, 161, 163,  91, 240, 116, 
    156, 181, 224, 148, 167, 101, 
    107, 161, 175, 242,  58, 188, 
    168, 248, 236, 245,   8, 219, 
    128,  79,  76, 250, 253, 201, 
    243, 128,  95,  15, 248, 205, 
    128,  47, 182, 123,  60,  11, 
     47, 226, 235, 186, 220, 231, 
    195, 191, 204, 254,  24, 123, 
    195, 254,  24, 123, 195, 254, 
     24, 123,  70, 129, 144, 254, 
    118, 195, 244,  33, 143,  69, 
    226,  30, 253, 172,  61,  77, 
    143, 122,  90, 100, 196, 211, 
    134,  53,  47, 164, 133, 227, 
    148, 195, 252,  28,  56, 166, 
    114, 216, 241, 237, 131, 159, 
     92, 217, 189,  84, 228,  95, 
    228, 220, 209, 192,  31, 243, 
     85, 182, 191,  72, 133, 224, 
     62,  96,  52,  46, 150, 206, 
    231, 158, 174, 147, 242, 110, 
     20, 151, 138,  24, 184, 179, 
     76,  95, 198, 154,  39, 178, 
     46,  37, 149,   6,  33, 250, 
    151,  23, 120, 209, 176, 140, 
    131,  90, 186,  94, 117, 170, 
    123, 233,  90, 221,  49, 141, 
    202,  28,  47, 153,  13, 110, 
    186, 117, 211, 177,  13, 139, 
    111, 151, 221,  44,  95, 117, 
    182, 248, 138, 105, 151,  76, 
    135, 231, 231, 243,  25, 151, 
    175, 238,  24, 100, 106, 188, 
    176, 102,  58, 117, 211,  93, 
    116, 203,  85, 167, 178, 108, 
     55, 210, 101,  43, 147,  73, 
    239,  88,  53, 139, 226, 251, 
     56, 147,  95, 197, 109,  42, 
     66, 172,  24, 187,  54, 251, 
      0, 242, 131,  18, 126,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
