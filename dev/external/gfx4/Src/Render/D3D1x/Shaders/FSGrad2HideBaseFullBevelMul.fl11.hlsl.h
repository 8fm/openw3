#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 srctexscale;                // Offset:   32 Size:    16
//   float4 texscale;                   // Offset:   48 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_gradtex                   sampler      NA          NA    0        1
// sampler_srctex                    sampler      NA          NA    1        1
// sampler_tex                       sampler      NA          NA    2        1
// gradtex                           texture  float4          2d    0        1
// srctex                            texture  float4          2d    1        1
// tex                               texture  float4          2d    2        1
// Constants                         cbuffer      NA          NA    0        1
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
// First Precompiled Shader at offset:[454]
// Embedded Data:
//  0x000001c6 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000001c1 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[4], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 4
mov r0.xy, l(0,0,0,0)
mov r0.z, -cb0[0].x
loop 
  lt r0.w, cb0[0].x, r0.z
  breakc_nz r0.w
  add r1.x, r0.z, cb0[1].x
  mov r2.xy, r0.xyxx
  mov r2.z, -cb0[0].y
  loop 
    lt r0.w, cb0[0].y, r2.z
    breakc_nz r0.w
    add r1.y, r2.z, cb0[1].y
    mad r1.zw, r1.xxxy, cb0[3].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.zwzz, t2.xyzw, s2, l(0.000000)
    add r2.y, r0.w, r2.y
    mad r1.yz, -r1.xxyx, cb0[3].xxyx, v2.xxyx
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.yzyy, t2.xyzw, s2, l(0.000000)
    add r2.x, r0.w, r2.x
    add r2.z, r2.z, l(1.000000)
  endloop 
  mov r0.xy, r2.xyxx
  add r0.z, r0.z, l(1.000000)
endloop 
mad r0.xy, -r0.xyxx, cb0[0].wwww, l(1.000000, 1.000000, 0.000000, 0.000000)
add r0.xy, -r0.yxyy, r0.xyxx
mul r0.y, r0.y, cb0[0].z
mad r0.x, -r0.x, cb0[0].z, l(1.000000)
mul r0.x, r0.x, l(0.500000)
max r0.x, r0.x, l(0.000000)
min r1.x, r0.x, l(0.500000)
mov r1.yw, l(0,0,0,0)
sample_indexable(texture2d)(float,float,float,float) r2.xyzw, r1.xyxx, t0.xyzw, s0
mad r0.x, r0.y, l(0.500000), l(0.500000)
max r0.x, r0.x, l(0.500000)
min r1.z, r0.x, l(1.000000)
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, r1.zwzz, t0.xyzw, s0
mul r1.xy, v2.xyxx, cb0[2].xyxx
sample_l_indexable(texture2d)(float,float,float,float) r1.x, r1.xyxx, t1.wxyz, s1, l(0.000000)
mul r1.w, r1.x, l(0.000100)
mov_sat r3.x, r2.w
mov_sat r3.y, r0.w
mov r2.w, l(1.000000)
mov r0.w, l(1.000000)
mul r0.xyzw, r3.yyyy, r0.xyzw
mad r0.xyzw, r2.xyzw, r3.xxxx, r0.xyzw
add r2.x, -r3.x, l(1.000000)
add r2.x, -r3.y, r2.x
mov r1.xyz, l(0,0,0,0)
mad r0.xyzw, r1.xyzw, r2.xxxx, r0.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[454], bundle is:[245] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSGrad2HideBaseFullBevelMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 560;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 7;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 0, offset 24:27 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 1, offset 28:31 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 2, offset 32:35 dwords
;  extUserElements 1[6] = IMM_CONST_BUFFER, 0, offset 36:39 dwords
NumVgprs             = 16;
NumSgprs             = 34;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000007
; constBufUsage           = 0x00000001

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
  s_mov_b64     s[32:33], exec                          // 000000000000: BEA0047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x20                 // 00000000000C: C0C20120
  s_waitcnt     lgkmcnt(0)                              // 000000000010: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 000000000014: C2860900
  v_mov_b32     v2, 0                                   // 000000000018: 7E040280
  v_mov_b32     v3, 0                                   // 00000000001C: 7E060280
  s_load_dwordx8  s[16:23], s[0:1], 0x10                // 000000000020: C0C80110
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  v_max_f32     v4, -s12, -s12                          // 000000000028: D2200004 6000180C
label_000C:
  v_mov_b32     v5, s12                                 // 000000000030: 7E0A020C
  v_cmp_gt_f32  vcc, v4, v5                             // 000000000034: 7C080B04
  s_cbranch_vccnz  label_0032                           // 000000000038: BF870023
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 00000000003C: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000040: BF8C007F
  v_add_f32     v5, s2, v4                              // 000000000044: 060A0802
  v_max_f32     v6, -s13, -s13                          // 000000000048: D2200006 60001A0D
label_0014:
  v_mov_b32     v7, s13                                 // 000000000050: 7E0E020D
  v_cmp_gt_f32  vcc, v6, v7                             // 000000000054: 7C080F06
  s_cbranch_vccnz  label_0030                           // 000000000058: BF870019
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x0c        // 00000000005C: C24C090C
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000060: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 000000000064: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 000000000068: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 00000000006C: C8210901
  v_add_f32     v9, s3, v6                              // 000000000070: 06120C03
  s_waitcnt     lgkmcnt(0)                              // 000000000074: BF8C007F
  v_mad_legacy_f32  v10, v5, s24, v7                    // 000000000078: D280000A 041C3105
  v_mad_legacy_f32  v11, v9, s25, v8                    // 000000000080: D280000B 04203309
  v_mad_legacy_f32  v7, -v5, s24, v7                    // 000000000088: D2800007 241C3105
  v_mad_legacy_f32  v8, -v9, s25, v8                    // 000000000090: D2800008 24203309
  image_sample_lz  v[9:12], v[10:11], s[16:23], s[4:7] dmask:0x8 // 000000000098: F09C0800 0024090A
  image_sample_lz  v[7:10], v[7:8], s[16:23], s[4:7] dmask:0x8 // 0000000000A0: F09C0800 00240707
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000A8: BF8C0F71
  v_add_f32     v3, v3, v9                              // 0000000000AC: 06061303
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000B0: BF8C0F70
  v_add_f32     v2, v2, v7                              // 0000000000B4: 06040F02
  v_add_f32     v6, 1.0, v6                             // 0000000000B8: 060C0CF2
  s_branch      label_0014                              // 0000000000BC: BF82FFE4
label_0030:
  v_add_f32     v4, 1.0, v4                             // 0000000000C0: 060808F2
  s_branch      label_000C                              // 0000000000C4: BF82FFDA
label_0032:
  v_mad_legacy_f32  v2, -v2, s15, 1.0                   // 0000000000C8: D2800002 23C81F02
  v_mad_legacy_f32  v3, -v3, s15, 1.0                   // 0000000000D0: D2800003 23C81F03
  v_sub_f32     v4, v3, v2                              // 0000000000D8: 08080503
  v_sub_f32     v2, v2, v3                              // 0000000000DC: 08040702
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000E0: C0C80100
  s_load_dwordx8  s[24:31], s[0:1], 0x18                // 0000000000E4: C0CC0118
  v_mul_legacy_f32  v3, s14, v4                         // 0000000000E8: 0E06080E
  v_mad_legacy_f32  v2, -v2, s14, 1.0 div:2             // 0000000000EC: D2800002 3BC81D02
  v_mad_legacy_f32  v3, v3, 0.5, 0.5                    // 0000000000F4: D2800003 03C1E103
  v_med3_f32    v6, v2, 0, 0.5                          // 0000000000FC: D2AE0006 03C10102
  v_med3_f32    v2, v3, 0.5, 1.0                        // 000000000104: D2AE0002 03C9E103
  v_mov_b32     v3, 0                                   // 00000000010C: 7E060280
  s_waitcnt     lgkmcnt(0)                              // 000000000110: BF8C007F
  image_sample  v[10:13], v[2:3], s[16:23], s[24:27] dmask:0xf // 000000000114: F0800F00 00C40A02
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x08          // 00000000011C: C2410908
  v_interp_p1_f32  v4, v0, attr2.x                      // 000000000120: C8100800
  v_interp_p1_f32  v5, v0, attr2.y                      // 000000000124: C8140900
  s_load_dwordx8  s[4:11], s[0:1], 0x08                 // 000000000128: C0C20108
  v_interp_p2_f32  v4, v1, attr2.x                      // 00000000012C: C8110801
  v_interp_p2_f32  v5, v1, attr2.y                      // 000000000130: C8150901
  v_mov_b32     v7, 0                                   // 000000000134: 7E0E0280
  s_waitcnt     lgkmcnt(0)                              // 000000000138: BF8C007F
  v_mul_legacy_f32  v2, s2, v4                          // 00000000013C: 0E040802
  v_mul_legacy_f32  v3, s3, v5                          // 000000000140: 0E060A03
  image_sample  v[6:9], v[6:7], s[16:23], s[24:27] dmask:0xf // 000000000144: F0800F00 00C40606
  image_sample_lz  v[2:5], v[2:3], s[4:11], s[28:31] dmask:0x8 // 00000000014C: F09C0800 00E10202
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000154: BF8C0F72
  v_max_f32     v4, v13, v13 clamp                      // 000000000158: D2200804 00021B0D
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 000000000160: BF8C0F71
  v_max_f32     v3, v9, v9 clamp                        // 000000000164: D2200803 00021309
  v_mul_legacy_f32  v5, v10, v4                         // 00000000016C: 0E0A090A
  v_mul_legacy_f32  v9, v11, v4                         // 000000000170: 0E12090B
  v_mul_legacy_f32  v10, v12, v4                        // 000000000174: 0E14090C
  v_sub_f32     v11, 1.0, v3                            // 000000000178: 081606F2
  v_interp_p1_f32  v12, v0, attr1.x                     // 00000000017C: C8300400
  v_add_f32     v15, v3, v4                             // 000000000180: 061E0903
  v_mac_legacy_f32  v5, v6, v3                          // 000000000184: 0C0A0706
  v_mac_legacy_f32  v9, v7, v3                          // 000000000188: 0C120707
  v_mac_legacy_f32  v10, v8, v3                         // 00000000018C: 0C140708
  v_interp_p1_f32  v13, v0, attr1.y                     // 000000000190: C8340500
  v_interp_p1_f32  v14, v0, attr1.z                     // 000000000194: C8380600
  v_subrev_f32  v3, v4, v11                             // 000000000198: 0A061704
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000019C: BF8C0F70
  v_mul_legacy_f32  v2, 0x38d1b717, v2                  // 0000000001A0: 0E0404FF 38D1B717
  v_interp_p2_f32  v12, v1, attr1.x                     // 0000000001A8: C8310401
  v_interp_p1_f32  v4, v0, attr1.w                      // 0000000001AC: C8100700
  v_interp_p2_f32  v13, v1, attr1.y                     // 0000000001B0: C8350501
  v_interp_p2_f32  v14, v1, attr1.z                     // 0000000001B4: C8390601
  v_mac_legacy_f32  v15, v2, v3                         // 0000000001B8: 0C1E0702
  v_mul_legacy_f32  v2, v5, v12                         // 0000000001BC: 0E041905
  v_interp_p2_f32  v4, v1, attr1.w                      // 0000000001C0: C8110701
  v_interp_p1_f32  v6, v0, attr0.x                      // 0000000001C4: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 0000000001C8: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 0000000001CC: C8200200
  v_mul_legacy_f32  v3, v9, v13                         // 0000000001D0: 0E061B09
  v_mul_legacy_f32  v5, v10, v14                        // 0000000001D4: 0E0A1D0A
  v_interp_p1_f32  v0, v0, attr0.w                      // 0000000001D8: C8000300
  v_mul_legacy_f32  v2, v2, v4                          // 0000000001DC: 0E040902
  v_mul_legacy_f32  v3, v3, v4                          // 0000000001E0: 0E060903
  v_mul_legacy_f32  v5, v5, v4                          // 0000000001E4: 0E0A0905
  v_interp_p2_f32  v6, v1, attr0.x                      // 0000000001E8: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 0000000001EC: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 0000000001F0: C8210201
  v_mul_legacy_f32  v4, v15, v4                         // 0000000001F4: 0E08090F
  v_interp_p2_f32  v0, v1, attr0.w                      // 0000000001F8: C8010301
  v_mac_legacy_f32  v2, v6, v4                          // 0000000001FC: 0C040906
  v_mac_legacy_f32  v3, v7, v4                          // 000000000200: 0C060907
  v_mac_legacy_f32  v5, v8, v4                          // 000000000204: 0C0A0908
  v_mac_legacy_f32  v4, v0, v4                          // 000000000208: 0C080900
  v_mul_legacy_f32  v0, v2, v4                          // 00000000020C: 0E000902
  v_mul_legacy_f32  v1, v3, v4                          // 000000000210: 0E020903
  v_mul_legacy_f32  v2, v5, v4                          // 000000000214: 0E040905
  s_mov_b64     exec, s[32:33]                          // 000000000218: BEFE0420
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 00000000021C: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v4                       // 000000000220: 5E020902
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000224: F800140F 00000100
  s_endpgm                                              // 00000000022C: BF810000
end


// Approximately 57 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSGrad2HideBaseFullBevelMul[] =
{
     68,  88,  66,  67, 169,  41, 
     92, 200,  18,  58,  90, 110, 
     61, 221, 124,  94, 211,  17, 
     42, 128,   1,   0,   0,   0, 
     20,  15,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    228,   2,   0,   0,  72,   3, 
      0,   0, 124,   3,   0,   0, 
    120,  14,   0,   0,  82,  68, 
     69,  70, 168,   2,   0,   0, 
      1,   0,   0,   0, 100,   1, 
      0,   0,   7,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    108,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     28,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  44,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  59,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     71,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,  79,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0,  86,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      2,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
     90,   1,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 103, 114, 
     97, 100, 116, 101, 120,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 115, 114,  99, 116, 
    101, 120,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,   0, 103, 114,  97, 
    100, 116, 101, 120,   0, 115, 
    114,  99, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0,  90,   1,   0,   0, 
      4,   0,   0,   0, 124,   1, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  28,   2,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     80,   2,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  44,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  87,   2, 
      0,   0,  32,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  44,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  99,   2,   0,   0, 
     48,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     34,   2,   0,   0, 111, 102, 
    102, 115, 101, 116,   0, 115, 
    114,  99, 116, 101, 120, 115, 
     99,  97, 108, 101,   0, 116, 
    101, 120, 115,  99,  97, 108, 
    101,   0,  77, 105,  99, 114, 
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
     69,  88, 244,  10,   0,   0, 
     80,   0,   0,   0, 189,   2, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 198,   1, 
      0,   0,  80,   0,   0,   0, 
    193,   1,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   4,   0, 
      0,   0,  54,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   3,   0,   4,   3, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5,  50,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  48,   0, 
      0,   1,  49,   0,   0,   8, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,   3,   0, 
      4,   3,  58,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,  42,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     50,   0,   0,  10, 194,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   4,  16,   0,   1,   0, 
      0,   0,   6, 132,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   0,   0,   0,   0, 
    230,  10,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      2,   0,   0,   0,   0,  96, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   2,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,  11,  98,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   1,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      6, 129,  32,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      6,  17,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 130,   0,  16,   0, 
      0,   0,   0,   0, 150,   5, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  66,   0,  16,   0, 
      2,   0,   0,   0,  42,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     54,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  50,   0, 
      0,  14,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0, 246, 143, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     22,   5,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  42, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  11,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  42, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  51,   0,   0,   7, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     54,   0,   0,   8, 162,   0, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,   1,  64,   0,   0, 
      0,   0,   0,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,  51,   0,   0,   7, 
     66,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0, 230,  10,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     50,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  70, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   0,  16,   0,   1,   0, 
      0,   0,  54, 121,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,  23, 183, 209,  56, 
     54,  32,   0,   5,  18,   0, 
     16,   0,   3,   0,   0,   0, 
     58,   0,  16,   0,   2,   0, 
      0,   0,  54,  32,   0,   5, 
     34,   0,  16,   0,   3,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  86,   5,  16,   0, 
      3,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   2,   0, 
      0,   0,   6,   0,  16,   0, 
      3,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     10,   0,  16, 128,  65,   0, 
      0,   0,   3,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,   0,   0,   0,   8, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  26,   0,  16, 128, 
     65,   0,   0,   0,   3,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,  54,   0, 
      0,   8, 114,   0,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,   6,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,  32, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  53,  16, 
      0,   0, 247,   0,   0,   0, 
     21,   0,   1,   0,  41,  82, 
     12,   0, 120,   1, 237,  86, 
    207, 107,  92,  69,  28, 255, 
    206, 188, 217, 121,  51,  47, 
     47, 111, 211, 116, 181, 137, 
     77, 210, 173,  73, 197, 139, 
    177, 105,  19, 168,  10,  82, 
     75, 137,  30,  20, 164, 241, 
     84,  30,  37, 155, 238, 198, 
      4, 183, 105, 179, 155, 214, 
     32, 101,  93,  61, 216,  30, 
    130, 120, 241, 230, 193, 131, 
      7,  65, 168,  39,  81,  47, 
    217,  36,  84,  24, 165,  23, 
    115, 244,  22, 244,  15,  40, 
     57, 121, 144, 198, 207, 247, 
    253, 104, 151,  82, 111,   1, 
      5, 243, 129, 207, 124, 231, 
    243, 190, 223, 153, 249, 206, 
    204, 131, 249,  94,   8,  41, 
    193, 171, 223, 184, 123, 108, 
    111,  47,  87, 127,  84,  62, 
     81,   1, 125,   1, 178, 101, 
    172,  26, 110, 137, 158, 239, 
    225,  22, 150,  27,  14, 144, 
     68, 125,  48, 207, 130,  39, 
    209,  63,  11, 203, 190, 156, 
     41, 220, 230,  52,  98,  39, 
    209,  27,   1,  63, 197, 164, 
    203, 176, 151, 193,  18, 248, 
     14,  56,  12, 242, 116, 140, 
    124,  77,   6, 207, 205,  80, 
    220, 116,  65, 131, 207, 129, 
     17, 136, 101, 105,   0, 180, 
     32, 131, 125,  30, 248,  16, 
     39, 184, 121,  50, 222,   7, 
    243,  53, 158,   4, 206,  45, 
    207, 135, 243, 227,  60,  25, 
    188, 223, 127,  27, 233, 121, 
    165,  59, 197, 149,  37,  58, 
    191,  39,   6, 223,  71,  55, 
    248, 172, 210,  49,  68,  95, 
      7, 220, 166, 227,  24,  63, 
    100, 154, 207, 130, 247, 182, 
      1, 254,  19, 246, 128,  71, 
    119, 155,  34, 215, 156, 141, 
    166,  34,  90,  88, 113, 140, 
    202, 108, 229, 104, 242,  95, 
    252,  95, 144, 255, 171, 124, 
     22, 124,   5, 124, 166, 233, 
    137, 164,  96, 255,  45, 144, 
    168, 221,  97, 126,  54,  64, 
    127, 178, 202, 181, 120,  76, 
    123, 172, 147, 139, 235, 210, 
     60, 137,  71, 151, 164, 149, 
    151, 138,  67, 153, 223, 163, 
     24,  58, 238, 210,  23, 161, 
     47, 118, 233,  89, 232, 217, 
     46,  61,   7,  61, 247,  80, 
    103, 243,  23, 147, 245, 164, 
    226, 123, 198, 199, 214, 127, 
    153,   7,  57, 238,  15,  15, 
    114, 220,  31,  30, 228, 184, 
    111,  76, 208,  82,  95, 174, 
    183, 130,   7, 235, 210, 251, 
    107, 189,  44, 182,  54,  62, 
    164, 181,  14, 217,  79, 182, 
    218,  82, 181, 218,  82, 183, 
    250, 132,  75, 190,  41,  42, 
    111, 135,   3,  52,  27, 202, 
    160, 165, 122, 204, 205,  81, 
    186, 213,  81, 246, 181,  45, 
    246,  73,  19, 104,  13, 127, 
    239,  51,  52, 219,  43, 163, 
    150,  46, 154, 155, 131, 240, 
    135, 246, 205,  45,  50,  67, 
    142, 108, 217,   9,  51, 236, 
    132,  61, 238, 188, 176,  95, 
    243, 152, 128, 218, 219, 133, 
    137,  33, 213,   3, 107,  79, 
    151,  81,  18,  37, 122, 204, 
    164, 122, 140, 204,  23, 247, 
      3,  59,  70, 108, 125, 127, 
    140, 150, 139, 107,  29, 239, 
    176, 214, 215,  96, 101,  81, 
    233, 221,  48, 212, 127, 236, 
    125, 220, 217,  53,  70, 255, 
      6,  43,  49,  78,  30, 115, 
    163,  30, 172, 199, 182,  96, 
    140, 244, 149,  33, 228,  63, 
     32, 238, 109,  68,  70,  71, 
     73, 204, 176, 123,  37, 137, 
    217, 217, 244,  52, 221, 217, 
    150,  98, 211, 147, 176, 222, 
    206, 207,  30, 239, 151, 115, 
    163,  98, 251, 190,  12, 238, 
    146, 193, 254, 200, 244,  33, 
    255, 146,  51,  56,  27,  97, 
     14,  97,  15,  79, 185,  54, 
    246, 200, 113, 210, 168, 200, 
     11, 116, 196, 241,  90, 223, 
     77, 114, 149, 114, 135,  26, 
    200,  81,  25, 156, 199,  81, 
    153, 230, 141, 190,  61,  44, 
     41, 176,  65, 212,  99, 251, 
    163, 208, 150, 162,  93, 253, 
    180,  33, 117, 210, 121, 118, 
     68, 107,  63,   8, 125, 191, 
     63,  52, 126,  41, 164, 194, 
    164,  35, 125, 198, 169,  35, 
     58, 224, 189, 238,  41,  21, 
     29, 249, 254, 215,  51,  66, 
     77,  56, 242, 251, 156,  40, 
     76,  57, 161,  95, 114, 210, 
     31,   9,  11, 131,  42,  18, 
    254,  33, 135, 234, 204, 145, 
    192,  57, 203, 178, 179,  71, 
    117,  20,  12,   7,  17, 222, 
     30,  39,  45, 242, 179,  58, 
     42,  96,  93,  65, 131,  78, 
      8, 220, 129,  60, 238, 138, 
    214,  68, 194,  19,  78,  91, 
     21, 250,  86, 135, 198,   6, 
     33,  89,  19,  74,  75, 136, 
    151, 136,  87,  81,  89,  61, 
     88, 199,  28, 233, 251,  87, 
    202, 223, 175, 143,  58, 131, 
    104, 147,  46, 240,  88, 249, 
     67,  40, 141,  18,  23,  55, 
     92,  38, 229,  58, 127, 163, 
     89,  39,  31,   0, 174,  39, 
    241, 196,  38, 154,  57,   4, 
    230, 154, 227, 185, 174,  97, 
    205, 125,   6, 254, 132,  68, 
    191, 157, 213, 199, 103,  51, 
    203, 243, 226,  59, 106,  47, 
     30, 249,   8, 215,  50, 255, 
     66, 102, 177,  54, 113, 247, 
    206, 239, 191,  76, 189, 248, 
    237,  79,  47, 172, 101, 223, 
    111, 103, 150, 231, 224,  18, 
    109, 119, 234, 187, 207, 123, 
    179, 154, 253, 171, 204, 183, 
      8, 242, 248, 249, 151, 227, 
    185,  74, 189, 242,  65, 115, 
    124, 229, 106, 227, 234, 123, 
    227, 205, 149,  70, 173, 114, 
    229,  84,  92, 173, 221, 136, 
    107, 171,  43, 181, 198,  82, 
    165,  30, 191,  59, 191,  58, 
     25, 207,  52,  46, 199,  23, 
    106,  75, 213,  90,  35,  62, 
    127, 250, 252, 196, 106,  60, 
    179,  80, 129, 104, 198, 211, 
     51, 175,  55,  42, 213,  83, 
    111,  44,  86, 107, 231,  42, 
    205, 218, 244, 245, 122, 253, 
     92, 237,  70, 173, 254, 214, 
    245, 250, 248, 124, 125,  98, 
     98, 124, 161, 222, 172,  99, 
    169,  20,  39, 178,  68, 184, 
     64, 230,  66, 242,  74, 101, 
    113, 137, 254,   6, 214,  12, 
    142, 136,  83,  84,  65,  84, 
    148,   0,   0,   0,  57,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  31,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  14,   0,   0,   0, 
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
