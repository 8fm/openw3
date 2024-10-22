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
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float      w
// TEXCOORD                 0   xyzw        2     NONE   float   xyzw
// TEXCOORD                 1   xyzw        3     NONE   float   xyzw
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
// First Precompiled Shader at offset:[54]
// Embedded Data:
//  0x00000036 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000031 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.w
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xyzw
dcl_output o0.xyzw
dcl_temps 1
mad r0.xyzw, v0.xyzw, v3.xyzw, v2.xyzw
mul r0.w, r0.w, v1.w
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[54], bundle is:[150] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FVertexCxformEAlphaMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  8, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask 15, param2, paramSlot 2, DefaultVal={0,0,0,0};   [3] generic, usageIdx  3, channelMask 15, param3, paramSlot 3, DefaultVal={0,0,0,0}

codeLenInByte        = 160;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 0;
extUserElementCount  = 0;
NumVgprs             = 14;
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
; SPI_PS_IN_CONTROL       = 0x00000004
SPIC:NUM_INTERP             = 4
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
  v_interp_p1_f32  v2, v0, attr0.x                      // 000000000004: C8080000
  v_interp_p1_f32  v3, v0, attr0.y                      // 000000000008: C80C0100
  v_interp_p1_f32  v4, v0, attr0.z                      // 00000000000C: C8100200
  v_interp_p1_f32  v5, v0, attr0.w                      // 000000000010: C8140300
  v_interp_p1_f32  v6, v0, attr2.x                      // 000000000014: C8180800
  v_interp_p1_f32  v7, v0, attr2.y                      // 000000000018: C81C0900
  v_interp_p1_f32  v8, v0, attr2.z                      // 00000000001C: C8200A00
  v_interp_p1_f32  v9, v0, attr2.w                      // 000000000020: C8240B00
  v_interp_p1_f32  v10, v0, attr3.x                     // 000000000024: C8280C00
  v_interp_p1_f32  v11, v0, attr3.y                     // 000000000028: C82C0D00
  v_interp_p1_f32  v12, v0, attr3.z                     // 00000000002C: C8300E00
  v_interp_p1_f32  v13, v0, attr3.w                     // 000000000030: C8340F00
  v_interp_p1_f32  v0, v0, attr1.w                      // 000000000034: C8000700
  v_interp_p2_f32  v5, v1, attr0.w                      // 000000000038: C8150301
  v_interp_p2_f32  v9, v1, attr2.w                      // 00000000003C: C8250B01
  v_interp_p2_f32  v13, v1, attr3.w                     // 000000000040: C8350F01
  v_interp_p2_f32  v2, v1, attr0.x                      // 000000000044: C8090001
  v_interp_p2_f32  v3, v1, attr0.y                      // 000000000048: C80D0101
  v_interp_p2_f32  v4, v1, attr0.z                      // 00000000004C: C8110201
  v_interp_p2_f32  v6, v1, attr2.x                      // 000000000050: C8190801
  v_interp_p2_f32  v7, v1, attr2.y                      // 000000000054: C81D0901
  v_interp_p2_f32  v8, v1, attr2.z                      // 000000000058: C8210A01
  v_interp_p2_f32  v10, v1, attr3.x                     // 00000000005C: C8290C01
  v_interp_p2_f32  v11, v1, attr3.y                     // 000000000060: C82D0D01
  v_interp_p2_f32  v12, v1, attr3.z                     // 000000000064: C8310E01
  v_interp_p2_f32  v0, v1, attr1.w                      // 000000000068: C8010701
  v_mac_legacy_f32  v9, v5, v13                         // 00000000006C: 0C121B05
  v_mac_legacy_f32  v6, v2, v10                         // 000000000070: 0C0C1502
  v_mac_legacy_f32  v7, v3, v11                         // 000000000074: 0C0E1703
  v_mac_legacy_f32  v8, v4, v12                         // 000000000078: 0C101904
  v_mul_legacy_f32  v0, v9, v0                          // 00000000007C: 0E000109
  v_mul_legacy_f32  v1, v6, v0                          // 000000000080: 0E020106
  v_mul_legacy_f32  v2, v7, v0                          // 000000000084: 0E040107
  v_mul_legacy_f32  v3, v8, v0                          // 000000000088: 0E060108
  v_cvt_pkrtz_f16_f32  v1, v1, v2                       // 00000000008C: 5E020501
  v_cvt_pkrtz_f16_f32  v0, v3, v0                       // 000000000090: 5E000103
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000094: F800140F 00000001
  s_endpgm                                              // 00000000009C: BF810000
end


// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FVertexCxformEAlphaMul[] =
{
     68,  88,  66,  67, 137, 152, 
    245,  65,  96, 111,  76,  86, 
    120,  94,  26, 231,  95, 207, 
    225,  10,   1,   0,   0,   0, 
     68,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    180,   0,   0,   0,  52,   1, 
      0,   0, 104,   1,   0,   0, 
    168,   4,   0,   0,  82,  68, 
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
     73,  83,  71,  78, 120,   0, 
      0,   0,   4,   0,   0,   0, 
      8,   0,   0,   0, 104,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,  15,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   8,   0,   0, 110,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
     15,  15,   0,   0, 110,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   3,   0,   0,   0, 
     15,  15,   0,   0,  67,  79, 
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
     83,  72,  69,  88,  56,   3, 
      0,   0,  80,   0,   0,   0, 
    206,   0,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     54,   0,   0,   0,  80,   0, 
      0,   0,  49,   0,   0,   0, 
    106,   8,   0,   1,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      2,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      3,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     56,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  58,  16,  16,   0, 
      1,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    152,   0,   0,   0,  21,   0, 
      1,   0,  41,  61,  10,   0, 
    120,   1, 237,  86,  75, 107, 
     19,  81,  20,  62, 103,  38, 
    239,  76,  38,  81,  90,  76, 
    181,  66, 197, 170,  85,  52, 
     38, 218, 110, 132,  98, 196, 
    234,  66, 232,  38,   5, 145, 
     50, 132,  76, 204, 196, 136, 
    147,  68,  38,  81,   6,  17, 
     81, 127, 129, 160,  63, 192, 
    255, 161, 180,  93, 184,  56, 
     46, 244,  23, 232,  66, 220, 
    187, 206,  66, 208, 115, 230, 
     33,  67, 219, 116,  21,  80, 
     36,  31, 124, 247, 155, 239, 
     62, 207, 125, 112, 231, 174, 
    102, 192, 195, 235,  31, 181, 
    161, 232, 251, 111,  95, 110, 
    125,  96, 141,  51,  49,  80, 
    129, 155, 146,  20, 160, 152, 
    150,  20,  96,  73,  18, 169, 
    160,   0, 232,  44,  42, 243, 
     45, 179, 202, 148, 178, 144, 
     62, 170,  59,  53,  78, 165, 
    122, 158, 121, 142,  63, 214, 
     89,  79,  51,  37, 111, 145, 
    201, 221, 120, 223, 187,  17, 
    230, 237,  87,  22, 132, 238, 
    225, 168,  36,   1, 246, 212, 
    149,   1, 198, 224,  12,  83, 
    226,  31, 135, 104,  83, 233, 
     87, 226,  20, 196,  36, 249, 
    203, 216,  51,  79,  70, 184, 
     79,   2, 217, 143,  40,  14, 
    154, 103,   8, 169,  35, 115, 
    219,  97, 142, 195,  47, 134, 
    232, 126, 144, 182,   9, 111, 
    151,  89, 177,   8,  11, 162, 
    202,  69, 239,  92,  36, 212, 
    171, 208,  96, 253, 223,  33, 
    107,  32,  16, 149, 243,  82, 
     96, 250,  43, 226,  67, 242, 
     79,  50,   1, 158, 111,  11, 
     95,  21,  97,  36,  46, 244, 
    188, 108,  35, 127, 111, 125, 
    175, 138, 247,  14,  94, 196, 
    179, 195, 184,  82,  87,  17, 
    234, 249, 249,  63, 222,  96, 
    111,  68, 252,  38, 251, 205, 
    136, 111, 176, 111,  68, 124, 
    147, 125,  51, 244,  97, 255, 
    121, 111,  60,  53, 220, 231, 
    103, 255,  50, 167,  49,  78, 
    134, 211,  24,  39, 195, 105, 
    140,  19, 163,   7,  69, 253, 
    185,   5, 144,  34,  64, 141, 
     64,  41,  16, 168,  51,   4, 
    169,  34,  65, 122, 158,  32, 
    179,  64, 144,  93,  36, 208, 
    150,   8, 114, 231,   9, 244, 
     50,  65, 126, 153,  32,   9, 
    132, 234,  44,  97, 246,  20, 
     97, 126, 133,  16, 210, 132, 
    152,  35,  84,  14,  17, 166, 
    230,   8, 211, 199,   9,  51, 
     39,   8, 181, 179, 132, 185, 
     11, 132, 122, 133,  48, 137, 
     20,  63, 118,  88,  83, 102, 
     53,  77,  61, 162, 107, 177, 
    185, 130, 150,  70, 208,  19, 
    168, 232,  73, 140, 233,  41, 
     76, 232, 124,  95, 250, 247, 
    237,  76, 120,  95, 190, 216, 
    174,   6, 239, 161, 114, 160, 
    114, 215, 203, 253, 174, 239, 
    250,  59,  55, 130, 242,  59, 
    129, 202, 111,  58, 203,  92, 
    253, 254, 105, 165, 252, 185, 
    254, 238, 105, 144, 239,   6, 
     42, 125, 200, 251, 230, 227, 
    203, 175,  91, 163,  32, 239, 
     77, 160,  45, 166, 180, 111, 
     95,  49, 154, 166, 109,  62, 
     25, 148, 134, 125, 167, 255, 
    160,  52,  24,  58, 150, 217, 
    189, 100, 180, 172, 199, 134, 
    229,  14,  45, 167, 103, 218, 
    198, 189, 182, 187, 108, 108, 
     56, 119, 141, 154, 213, 107, 
     89, 142, 177, 118, 121, 173, 
    226,  26,  27,  29, 147, 205, 
    192, 184, 121, 219, 114, 134, 
    150, 123, 221, 109, 247, 157, 
    238, 141, 107, 246, 195, 142, 
    185, 254, 200,  46, 181, 237, 
     74, 165, 212, 177,   7,  54, 
    143, 226, 163,  16,  60, 182, 
    228,  61, 152, 100, 118, 205, 
    251,  61, 248,  13, 234, 156, 
     37, 190,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
      5,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
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
      0,   0,   0,   0
};
