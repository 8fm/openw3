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
// sampler_tex[3]                    sampler      NA          NA    3        1
// tex[3]                            texture  float4          2d    3        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xy          1     NONE   float   xy  
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
// First Precompiled Shader at offset:[45]
// Embedded Data:
//  0x0000002d - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000028 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v1.xyxx, t3.xyzw, s3
mul o0.xyzw, r0.xxxx, v0.wwww
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[45], bundle is:[137] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVAEAlphaInv.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  3, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 88;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 2;
;  extUserElements 1[0] = IMM_RESOURCE, 3, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 3, offset 8:11 dwords
NumVgprs             = 6;
NumSgprs             = 14;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000008
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
  v_interp_p1_f32  v2, v0, attr1.x                      // 000000000014: C8080400
  v_interp_p1_f32  v3, v0, attr1.y                      // 000000000018: C80C0500
  v_interp_p2_f32  v2, v1, attr1.x                      // 00000000001C: C8090401
  v_interp_p2_f32  v3, v1, attr1.y                      // 000000000020: C80D0501
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3]         // 000000000028: F0800100 00010202
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000030: C8000300
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000034: C8010301
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000038: BF8C0F70
  v_mul_legacy_f32  v0, v2, v0                          // 00000000003C: 0E000102
  s_mov_b64     exec, s[12:13]                          // 000000000040: BEFE040C
  v_cvt_pkrtz_f16_f32  v0, v0, v0                       // 000000000044: 5E000100
  s_nop         0x0000                                  // 000000000048: BF800000
  exp           mrt0, v0, v0, v0, v0 compr vm           // 00000000004C: F800140F 00000000
  s_endpgm                                              // 000000000054: BF810000
end


// Approximately 3 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVAEAlphaInv[] =
{
     68,  88,  66,  67,  39, 124, 
    159,  50, 104,  79, 148,  98, 
    105, 126,  62,  77, 250,  67, 
    185, 155,   1,   0,   0,   0, 
     20,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     12,   1,   0,   0,  92,   1, 
      0,   0, 144,   1,   0,   0, 
    120,   4,   0,   0,  82,  68, 
     69,  70, 208,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    146,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    124,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 139,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,  91,  51,  93, 
      0, 116, 101, 120,  91,  51, 
     93,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  70, 111, 114,  32,  68, 
    117, 114,  97, 110, 103, 111, 
     32,  57,  46,  51,  48,  46, 
     49,  50,  48,  57,  56,  46, 
     48,   0, 171, 171,  73,  83, 
     71,  78,  72,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   8, 
      0,   0,  62,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   3,   3, 
      0,   0,  67,  79,  76,  79, 
     82,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88, 224,   2,   0,   0, 
     80,   0,   0,   0, 184,   0, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  45,   0, 
      0,   0,  80,   0,   0,   0, 
     40,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   3,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   3,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   3,   0, 
      0,   0,   0,  96,  16,   0, 
      3,   0,   0,   0,  56,   0, 
      0,   7, 242,  32,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 139,   0, 
      0,   0,  21,   0,   1,   0, 
     41,  29,  10,   0, 120,   1, 
    237,  86, 207, 107,  19,  65, 
     20, 126, 147, 221, 164,  73, 
    141,  73,  75,  61,  84,  80, 
    240,  34, 244,  20, 136, 122, 
    144,  94, 154,  66, 235,  15, 
    188,  89,  42, 173,  12, 210, 
    109, 179,  49, 213, 109,  90, 
     54, 105,   9, 165, 196, 120, 
    207, 161, 127, 130, 119, 255, 
      1, 241, 144, 132, 158, 246, 
     80, 168,  71, 143, 253,  19, 
    122, 180,  32, 213, 247, 109, 
    222, 132, 116,  49, 183, 128, 
    130, 249, 224, 123, 111, 191, 
    157, 121, 111, 223, 206,  12, 
     51, 115, 119, 146,  66, 156, 
    189,  59, 238, 194, 127, 126, 
    124, 217, 248, 196,  62, 206, 
     84, 226, 129, 122,  18, 150, 
    145, 130,  33, 154, 131,  65, 
    135,  24,  81, 130,  93, 134, 
    185, 198,  44,  48, 209, 102, 
    216,  67, 161, 235, 177, 157, 
     98, 218, 204, 175,  28, 247, 
    148, 253,  52, 147, 195,   9, 
    169, 145,  22, 233,   0, 227, 
      1,  11, 134, 129, 184,  40, 
     16, 107, 128,  26, 140, 134, 
     31, 204,  65, 247,  97, 254, 
    140,  25,  38,  98, 135,   1, 
    181, 153,  92, 240, 242, 251, 
    225, 255, 254, 109, 160,  30, 
     75,  70,   6, 117,   2, 253, 
    121,  98,  96,  62,   6, 129, 
    154,  17,   3, 124, 147, 126, 
    102, 204, 206,  69,  99,  44, 
    208,  47,  92,  12,  67, 240, 
    139, 145, 198, 195,   0, 140, 
     70, 190, 132,  84, 147,  80, 
     51, 116,  15,  15, 255,  25, 
    204,  90, 197,  88, 128,  88, 
    247,  89, 166,   1, 218, 177, 
    246, 137, 154,  29, 240, 120, 
    150, 126,  64,  25, 173,  34, 
    218,  26, 166,  21, 189, 129, 
    206, 222, 233, 107,  29, 209, 
    175,  35, 122,  35, 162,  55, 
    175, 105, 126,  14, 181, 228, 
    199,  60, 179, 107, 252, 203, 
     28, 215,  56,  26, 142, 107, 
     28,  13, 199,  53, 142, 140, 
     33,  26, 118, 171, 221, 152, 
    188, 106, 199, 172, 159, 109, 
     82,  39, 221, 164, 106, 118, 
    201,  78,   6,  20,  79,   7, 
    202,  78,   5,  42, 126,  51, 
    248,  64, 173,  14, 169, 230, 
     69,  44, 198, 167, 155,  69, 
    129, 178,  84, 176, 151, 109, 
    117,  88, 102, 210, 246,  21, 
    199, 201,  62, 121, 203, 236, 
    115,  31,  59, 183, 217, 154, 
    179,  16, 123,  52,  96, 238, 
     26, 124, 122,  41, 180,  65, 
    227,  36, 227, 253,  90, 205, 
    201, 193,  63,  43, 222, 220, 
    101,  50, 253,  44,  61,  60, 
    147, 246, 130, 120, 196, 223, 
     96, 238, 111, 159, 238, 127, 
     73, 182,  94,  20, 229, 253, 
    134, 120, 228, 192,  21, 172, 
    230, 164,  22, 190, 203, 187, 
     35, 241, 188, 153, 135, 241, 
    165, 121, 189, 233, 120, 206, 
     97,  53,  87, 219, 245, 119, 
    223, 231, 170,  53, 223, 117, 
    118,  30, 232, 162, 123, 160, 
    221, 122, 205, 245,  43, 142, 
    167, 223, 150, 234, 143, 244, 
    138, 191, 165,  95, 186, 149, 
    162, 235, 235, 165, 135,  75, 
    249, 186,  94,  41,  59,  44, 
    170, 250, 201, 250, 234, 171, 
    197, 229,  69, 111, 175, 236, 
     60, 175,  28, 228,  74,  94, 
     62, 159,  43, 123,  85, 220, 
    202, 122, 184, 144, 111, 226, 
    190,  55, 193, 220, 113, 182, 
     43, 244,  27, 252,  97,  30, 
     84,   0,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
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
      0,   0,   0,   0
};
