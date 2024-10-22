#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex                       sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[86]
// Embedded Data:
//  0x00000056 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000051 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 2
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v2.xyxx, t0.xyzw, s0
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[86], bundle is:[166] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGCxformAcMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 196;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 10;
NumSgprs             = 14;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000001
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
  s_mov_b64     s[12:13], exec                          // 000000000000: BE8C047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx4  s[0:3], s[0:1], 0x08                  // 000000000010: C0800108
  v_interp_p1_f32  v2, v0, attr2.x                      // 000000000014: C8080800
  v_interp_p1_f32  v3, v0, attr2.y                      // 000000000018: C80C0900
  v_interp_p2_f32  v2, v1, attr2.x                      // 00000000001C: C8090801
  v_interp_p2_f32  v3, v1, attr2.y                      // 000000000020: C80D0901
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3] dmask:0xf // 000000000028: F0800F00 00010202
  v_interp_p1_f32  v6, v0, attr1.x                      // 000000000030: C8180400
  v_interp_p1_f32  v7, v0, attr1.y                      // 000000000034: C81C0500
  v_interp_p1_f32  v8, v0, attr1.z                      // 000000000038: C8200600
  v_interp_p2_f32  v6, v1, attr1.x                      // 00000000003C: C8190401
  v_interp_p2_f32  v7, v1, attr1.y                      // 000000000040: C81D0501
  v_interp_p2_f32  v8, v1, attr1.z                      // 000000000044: C8210601
  v_interp_p1_f32  v9, v0, attr1.w                      // 000000000048: C8240700
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000004C: BF8C0F70
  v_mul_legacy_f32  v2, v2, v6                          // 000000000050: 0E040D02
  v_mul_legacy_f32  v3, v3, v7                          // 000000000054: 0E060F03
  v_mul_legacy_f32  v4, v4, v8                          // 000000000058: 0E081104
  v_interp_p1_f32  v6, v0, attr0.x                      // 00000000005C: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 000000000060: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 000000000064: C8200200
  v_interp_p2_f32  v9, v1, attr1.w                      // 000000000068: C8250701
  v_interp_p1_f32  v0, v0, attr0.w                      // 00000000006C: C8000300
  v_interp_p2_f32  v6, v1, attr0.x                      // 000000000070: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 000000000074: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 000000000078: C8210201
  v_mul_legacy_f32  v2, v2, v9                          // 00000000007C: 0E041302
  v_mul_legacy_f32  v3, v3, v9                          // 000000000080: 0E061303
  v_mul_legacy_f32  v4, v4, v9                          // 000000000084: 0E081304
  v_mul_legacy_f32  v5, v5, v9                          // 000000000088: 0E0A1305
  v_interp_p2_f32  v0, v1, attr0.w                      // 00000000008C: C8010301
  v_mac_legacy_f32  v2, v6, v5                          // 000000000090: 0C040B06
  v_mac_legacy_f32  v3, v7, v5                          // 000000000094: 0C060B07
  v_mac_legacy_f32  v4, v8, v5                          // 000000000098: 0C080B08
  v_mac_legacy_f32  v5, v0, v5                          // 00000000009C: 0C0A0B00
  v_mul_legacy_f32  v0, v2, v5                          // 0000000000A0: 0E000B02
  v_mul_legacy_f32  v1, v3, v5                          // 0000000000A4: 0E020B03
  v_mul_legacy_f32  v2, v4, v5                          // 0000000000A8: 0E040B04
  s_mov_b64     exec, s[12:13]                          // 0000000000AC: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 0000000000B0: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v5                       // 0000000000B4: 5E020B02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 0000000000B8: F800140F 00000100
  s_endpgm                                              // 0000000000C0: BF810000
end


// Approximately 9 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGCxformAcMul[] =
{
     68,  88,  66,  67, 145,   9, 
    116, 165,  52,  42, 102,  82, 
    246, 216, 137,  97, 207,  23, 
    201,  95,   1,   0,   0,   0, 
     56,   6,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0, 104,   1, 
      0,   0, 156,   1,   0,   0, 
    156,   5,   0,   0,  82,  68, 
     69,  70, 200,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    140,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    124,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 136,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,   0, 116, 101, 
    120,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  70, 111, 114,  32,  68, 
    117, 114,  97, 110, 103, 111, 
     32,  57,  46,  51,  48,  46, 
     49,  50,  48,  57,  56,  46, 
     48,   0,  73,  83,  71,  78, 
     92,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88, 248,   3,   0,   0, 
     80,   0,   0,   0, 254,   0, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  86,   0, 
      0,   0,  80,   0,   0,   0, 
     81,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   2,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   2,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,  18, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    168,   0,   0,   0,  21,   0, 
      1,   0,  41, 140,  10,   0, 
    120,   1, 237,  85,  65, 107, 
     19,  65,  20, 126,  51, 187, 
    217, 108, 178, 219,  93,  37, 
     61, 180,  80, 177, 165,  45, 
    120, 208,  72, 212,  83, 241, 
     80,  49,  88, 148, 122, 177, 
    245,  82, 151, 182, 219, 118, 
     99, 165, 155,  68, 146,  40, 
    193,  67, 140, 224,  49,   7, 
     65, 241,  55,  40, 136, 120, 
    240,  40,  73,  16,  15,   3, 
    122, 240,  15, 120, 247, 232, 
     69,  40,  34, 213, 247, 118, 
    103, 234,  82, 218,  91,  64, 
    193, 124, 240, 189,  55, 223, 
    206, 155, 183, 179,  51, 179, 
    243,  58,  89, 136, 240, 227, 
    221,  75, 155, 252, 238,  23, 
    247,  69, 145,   1, 164, 176, 
    141,  46, 242, 132, 166,  73, 
     22,  32, 204, 144,   5,  56, 
     69, 134,   2,  56,   0, 165, 
    112, 144,  31, 144, 243,  72, 
    234,  83, 140,  49, 223, 127, 
    138, 246,  24, 146, 226, 230, 
    112, 204,  53, 244, 103, 144, 
    216, 132,  25,  36, 165, 165, 
    116,   4, 229,   9,  26,  25, 
    132,  78, 230,   0,  40, 159, 
    194,  40, 146, 114,  17, 200, 
     39, 115, 192,  44, 153, 195, 
    113,  22,  41, 151, 224,  80, 
    208, 220,  84,  46, 242, 242, 
    243, 163, 239, 248, 219, 136, 
    231, 149, 180, 127, 246, 137, 
     64, 251, 145,   4, 205,  89, 
    197,  93, 148,  31, 162, 214, 
    108,  81, 106, 181, 151, 125, 
    228,  81, 248, 133, 136,  14, 
     75,   2,  74, 211, 126,  25, 
    224, 162,  69, 207,  78, 194, 
     36, 121,  62,  29, 157, 139, 
    255,   5, 234, 172, 210,  90, 
    208, 250, 210,  57, 141,  87, 
     36,   6, 245, 159,  70,   2, 
    180, 123, 196,  39,  99, 176, 
     75,  74, 105, 118,  64, 107, 
    164, 163, 141,  75, 104, 250, 
     49,  53,  88, 229,  22,  95, 
    117,  39, 100, 191,   6,  30, 
    106,  47, 161,  87,  80, 175, 
     36, 244,  58, 234, 245, 132, 
    222,  64, 189, 177, 175, 101, 
    126,  55, 122,  31,  79, 209, 
     62, 227, 195, 214, 191, 204, 
    225,  28,   7, 195, 225,  28, 
      7, 195, 225,  28,   7, 198, 
      8,  45, 189, 211, 109, 101, 
    247, 186,  92, 251, 217,   5, 
    246, 190, 111, 178, 118,  31, 
     76,  83,  64, 198,  22, 204, 
    204,   8, 150,  25,  17,  15, 
    161, 211,   3, 183, 253, 141, 
    115, 188, 196, 244,  49,   1, 
    169,   9,   1, 198, 164,  96, 
    250, 184,  96, 169,  19, 130, 
     25,  83,   2, 210,  51, 226, 
    174, 219, 233, 241,  17, 221, 
    209,  92, 195, 209, 143, 155, 
     88, 230,  48, 150,  97,  44, 
    199, 216, 244, 172, 192, 251, 
     80,  48, 192,  49,  12, 199, 
    240,  41, 193, 115,  24, 155, 
    195, 216, 156, 233, 164, 114, 
     89, 135, 105,  76,  24, 150, 
    110, 167,  45, 195,  54,  45, 
    211,   6,  43, 107, 115,  11, 
     28, 205, 226, 142, 110, 233, 
    142, 173, 239, 117,  49,  71, 
    124,  39, 143, 170,  59, 245, 
     81, 111,  28, 109, 212,  68, 
    168, 122, 171, 128, 229, 154, 
    169,  62,  42, 221,  88,  27, 
    216,  51,  89, 139,  31,  75, 
     79, 245, 131, 106, 134, 179, 
    159,  37, 198,  91, 217, 255, 
     74, 122,  26, 111,  33, 111, 
    238,  60, 255, 120, 235, 251, 
    244, 155, 207, 242, 249,  39, 
    233,  41,   7, 213, 247, 240, 
    245, 213, 197,   5, 106,  32, 
    190, 202, 190,  53,  36, 141, 
     47, 205, 121,  27, 126, 232, 
     63, 168, 231,  27, 213,  90, 
    117,  39,  95, 111, 212,   2, 
    191, 124, 206, 219,  10, 238, 
    123,  65, 179,  17, 212,  42, 
    126, 232, 221,  46,  53,  47, 
    120,  75, 181,  77, 239,  70, 
     80, 217,  10, 106,  94, 241, 
    124, 177, 208, 244, 150, 182, 
    125,  20, 117, 239, 202, 114, 
    208,  92,  94, 184, 220,  44, 
     85, 107, 229,  75, 155, 215, 
    239, 133, 249,  82,  88,  40, 
    228, 183, 195, 122, 136,  47, 
    136, 177,  38,  95,  79,  85, 
     44, 141,  44, 251, 119,  42, 
    240,  27, 243, 147,  45, 230, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   9,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
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
