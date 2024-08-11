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
// TEXCOORD                 0   xy          0     NONE   float   xy  
// COLOR                    0   xyzw        1     NONE   float   xyzw
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
// First Precompiled Shader at offset:[50]
// Embedded Data:
//  0x00000032 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000002d - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xy
dcl_input_ps linear v1.xyzw
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v0.xyxx, t0.xyzw, s0
mul o0.w, r0.x, v1.w
mov o0.xyz, v1.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[50], bundle is:[143] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FText.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 112;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
NumVgprs             = 6;
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
; SPI_PS_IN_CONTROL       = 0x00000002
SPIC:NUM_INTERP             = 2
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
  v_interp_p1_f32  v2, v0, attr0.x                      // 000000000014: C8080000
  v_interp_p1_f32  v3, v0, attr0.y                      // 000000000018: C80C0100
  v_interp_p2_f32  v2, v1, attr0.x                      // 00000000001C: C8090001
  v_interp_p2_f32  v3, v1, attr0.y                      // 000000000020: C80D0101
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3]         // 000000000028: F0800100 00010202
  v_interp_p1_f32  v3, v0, attr1.w                      // 000000000030: C80C0700
  v_interp_p2_f32  v3, v1, attr1.w                      // 000000000034: C80D0701
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000038: BF8C0F70
  v_mul_legacy_f32  v2, v2, v3                          // 00000000003C: 0E040702
  v_interp_p1_f32  v3, v0, attr1.x                      // 000000000040: C80C0400
  v_interp_p1_f32  v4, v0, attr1.y                      // 000000000044: C8100500
  v_interp_p1_f32  v0, v0, attr1.z                      // 000000000048: C8000600
  v_interp_p2_f32  v3, v1, attr1.x                      // 00000000004C: C80D0401
  v_interp_p2_f32  v4, v1, attr1.y                      // 000000000050: C8110501
  v_interp_p2_f32  v0, v1, attr1.z                      // 000000000054: C8010601
  s_mov_b64     exec, s[12:13]                          // 000000000058: BEFE040C
  v_cvt_pkrtz_f16_f32  v1, v3, v4                       // 00000000005C: 5E020903
  v_cvt_pkrtz_f16_f32  v0, v0, v2                       // 000000000060: 5E000500
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000064: F800140F 00000001
  s_endpgm                                              // 00000000006C: BF810000
end


// Approximately 4 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FText[] =
{
     68,  88,  66,  67, 106, 234, 
     58, 162, 189,  88,  51,  92, 
     25,  85, 125, 194, 141, 232, 
    237,   9,   1,   0,   0,   0, 
     56,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0,  84,   1, 
      0,   0, 136,   1,   0,   0, 
    156,   4,   0,   0,  82,  68, 
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
     72,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   3,   0,   0, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  67,  79,  76, 
     79,  82,   0, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171,  83,  72,  69,  88, 
     12,   3,   0,   0,  80,   0, 
      0,   0, 195,   0,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0,  50,   0,   0,   0, 
     80,   0,   0,   0,  45,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 114,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 145,   0, 
      0,   0,  21,   0,   1,   0, 
     41,  44,  10,   0, 120,   1, 
    237,  86,  61, 139,  19,  81, 
     20, 189, 111,  50, 249,  52, 
     31,  43,   8,  70,  81,  20, 
     65, 176, 144,  96, 212, 202, 
     66,  82,  44,  98, 189, 177, 
    144, 229, 177, 100, 178,  59, 
    113, 197, 108, 118,  55,   9, 
     18,  84,  98, 236,  83,  88, 
     88, 249,  55, 180,  78, 130, 
     88,  76, 109, 105, 101,  35, 
     43,  86,  91,   6, 148, 172, 
    231, 100, 222, 211,  65, 220, 
     46, 160,  96,  14, 156, 123, 
    231, 188, 143,  59, 119, 222, 
    155, 185, 111, 174, 102, 100, 
    142, 243,  31, 110, 191, 167, 
    191, 180, 255, 249, 219,  91, 
    248,  56, 168, 140,  39, 122, 
     41,  90, 145,  98, 154,  86, 
    228,  10,  13,   7,  56,  34, 
      9, 184,  60, 184,   7,  86, 
     64, 246,  89, 134, 168,  76, 
    158, 194, 174, 128,  28, 251, 
     21, 243, 238, 194,  23,  65, 
     76, 151,   2, 200, 176,  12, 
     71,  88,  79, 196, 104,   0, 
    151, 230,  55, 216,  62,  34, 
     11,  50,  22,  65,  31, 141, 
     33, 151, 105, 254, 140, 179, 
     32, 115,  58,  14, 204, 205, 
    198, 162,  55, 143,  63, 127, 
    222, 191, 141,  48, 175, 168, 
    253, 181,  79,   4, 247,  35, 
     10, 230, 108, 199, 125,  49, 
    227, 236, 154,  77, 141, 230, 
     90, 112, 220,   4,  60,  14, 
     71,   0, 215,  59,  10, 171, 
     25,  47,  97, 118,  38, 161, 
     46, 200,  69,  94, 252, 103, 
    176, 239,  42, 215, 130, 228, 
    123, 207, 247, 200, 130, 253, 
    167,  65, 145, 193, 152, 124, 
     89, 148,  41, 149, 213,  10, 
     58,  92, 193,  80, 199, 168, 
    231,  65,  35,  26, 129,  99, 
    105, 103,   3,  31, 232,  70, 
    225, 156,  76, 185, 175, 208, 
     26,  90,  71, 244,  58, 244, 
    122,  68, 215, 160, 107,  17, 
     93, 135, 174,  91, 109, 227, 
     23, 194, 251,  57, 220, 103, 
     52, 246, 255, 101,  46, 115, 
     92,  12, 151,  57,  46, 134, 
    203,  28,  23, 198,  57, 250, 
    238, 112, 212, 207, 204,  70, 
     78, 236, 251,  72, 212, 187, 
     73,  74,  13, 112,  52, 165, 
      2,  81, 217,  64,  73,  58, 
     80,  42,  23,  60, 151, 225, 
     88, 212, 224, 208, 113,  80, 
    197, 146, 104,  79, 230, 130, 
    189, 194, 112, 236,  36, 221, 
    188, 184, 217,  64, 226,  43, 
      1,  14, 165,  64, 185, 185, 
     64, 197,  79,   6,  42, 161, 
    130, 172,  59,  27, 161, 254, 
    133, 245, 243, 148, 173, 127, 
     47, 198, 103,  96, 195, 235, 
    176, 118,  71, 129,  35,  82, 
    217,  62,  30, 151,  40, 201, 
    170,  98, 126,   8, 174,  25, 
    207,  90, 207,  82, 157, 255, 
     25,  37,  68, 205, 244, 223, 
     55, 158, 243,  79, 128,   7, 
    146,  89, 251, 244, 122, 115, 
    246, 204, 180, 247, 140, 103, 
     12, 254, 154,  29, 214, 222, 
    124,  60,  48, 109, 175, 140, 
    175, 130, 156, 223, 184, 165, 
    235,  94, 211, 123, 210,  41, 
    117, 119, 219, 187, 143,  74, 
    157, 110, 219, 247, 118, 174, 
    235,  45, 255, 177, 246, 123, 
     93, 191, 221, 242, 154, 250, 
     65, 163, 119,  83,  87, 219, 
    155, 122, 205, 111, 109, 249, 
    109, 189, 122,  99, 181, 220, 
    211, 213, 109,  15, 162, 163, 
    239, 220, 195, 192,  82, 163, 
     89,  46, 151, 182, 155, 157, 
     38, 130, 134,  56,  50, 183, 
    226, 239,  95,  18, 220, 241, 
     30, 182, 228,   7, 128,  57, 
     31,  38,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
      4,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
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
