/*
 * Copyright 1993-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to NVIDIA intellectual property rights under U.S. and
 * international Copyright laws.
 *
 * These Licensed Deliverables contained herein is PROPRIETARY and
 * CONFIDENTIAL to NVIDIA and is being provided under the terms and
 * conditions of a form of NVIDIA software license agreement by and
 * between NVIDIA and Licensee ("License Agreement") or electronically
 * accepted by Licensee.  Notwithstanding any terms or conditions to
 * the contrary in the License Agreement, reproduction or disclosure
 * of the Licensed Deliverables to any third party without the express
 * written consent of NVIDIA is prohibited.
 *
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
 * SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
 * PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
 * NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
 * DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
 * SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THESE LICENSED DELIVERABLES.
 *
 * U.S. Government End Users.  These Licensed Deliverables are a
 * "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
 * 1995), consisting of "commercial computer software" and "commercial
 * computer software documentation" as such terms are used in 48
 * C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
 * only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
 * 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
 * U.S. Government End Users acquire the Licensed Deliverables with
 * only those rights set forth herein.
 *
 * Any use of the Licensed Deliverables in individual and commercial
 * software must include, in the user documentation and internal
 * comments to the code, the above Disclaimer and U.S. Government End
 * Users Notice.
 */

//
// Synchronization
//

__attribute__((nv_linkonce_odr)) __inline__ int __syncthreads_count(int predicate) {
    return __nvvm_bar0_popc(predicate);
}

__attribute__((nv_linkonce_odr)) __inline__ int __syncthreads_and(int predicate) {
    return __nvvm_bar0_and(predicate);
}

__attribute__((nv_linkonce_odr)) __inline__ int __syncthreads_or(int predicate) {
    return __nvvm_bar0_or(predicate);
}

//
// Memory Fence
//

__attribute__((nv_linkonce_odr)) __inline__ void __threadfence_block() {
    __nvvm_membar_cta();
}

__attribute__((nv_linkonce_odr)) __inline__ void __threadfence() {
    __nvvm_membar_gl();
}

__attribute__((nv_linkonce_odr)) __inline__ void __threadfence_system() {
    __nvvm_membar_sys();
}

//
// Vote
//
__attribute__((nv_linkonce_odr)) __inline__ int __all(int a) {
    int result;
    asm __volatile__ ("{ \n\t"
        ".reg .pred \t%%p1; \n\t"
        ".reg .pred \t%%p2; \n\t"
        "setp.ne.u32 \t%%p1, %1, 0; \n\t"
        "vote.all.pred \t%%p2, %%p1; \n\t"
        "selp.s32 \t%0, 1, 0, %%p2; \n\t"
        "}" : "=r"(result) : "r"(a));
    return result;
}
__attribute__((nv_linkonce_odr)) __inline__ int __any(int a) {
    int result;
    asm __volatile__ ("{ \n\t"
        ".reg .pred \t%%p1; \n\t"
        ".reg .pred \t%%p2; \n\t"
        "setp.ne.u32 \t%%p1, %1, 0; \n\t"
        "vote.any.pred \t%%p2, %%p1; \n\t"
        "selp.s32 \t%0, 1, 0, %%p2; \n\t"
        "}" : "=r"(result) : "r"(a));
    return result;
}
__attribute__((nv_linkonce_odr)) __inline__ int __ballot(int a) {
    int result;
    asm __volatile__ ("{ \n\t"
        ".reg .pred \t%%p1; \n\t"
        "setp.ne.u32 \t%%p1, %1, 0; \n\t"
        "vote.ballot.b32 \t%0, %%p1; \n\t"
        "}" : "=r"(result) : "r"(a));
    return result;
}

    
//
// Misc
//

__attribute__((nv_linkonce_odr)) __inline__ void __brkpt() {
    asm __volatile__ ("brkpt;");
}

__attribute__((nv_linkonce_odr)) __inline__ int clock() {
    int r;
    asm __volatile__ ("mov.u32 \t%0, %%clock;" : "=r"(r));
    return r;
}

__attribute__((nv_linkonce_odr)) __inline__ long long clock64() {
    long long z;
    asm __volatile__ ("mov.u64 \t%0, %%clock64;" : "=l"(z));
    return z;
}
    
#define __prof_trigger(X) asm __volatile__ ("pmevent \t" #X ";")

#if __CUDA_ARCH__ >= 200
__attribute__((nv_linkonce_odr)) __inline__ int __clz(int x) {
    //int r;
    //asm("clz.b32 \t%0, %1;" : "=r"(r) :"r"(x));
    //return r;
    return __nvvm_clz_i(x);
}
__attribute__((nv_linkonce_odr)) __inline__ int __clzll(long long x) {
    //int r;
    //asm("clz.b64 \t%0, %1;" : "=r"(r) :"l"(x));
    //return r;
    return __nvvm_clz_ll(x);
}

__attribute__((nv_linkonce_odr)) __inline__ int __popc(int x) {
    //int r;
    //asm("popc.b32 \t%0, %1;" : "=r"(r) :"r"(x));
    //return r;
    return __nvvm_popc_i(x);
}
__attribute__((nv_linkonce_odr)) __inline__ int __popcll(long long x) {
    //int r;
    //asm("popc.b64 \t%0, %1;" : "=r"(r) :"l"(x));
    //return r;
    return __nvvm_popc_ll(x);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __byte_perm(unsigned int a, unsigned int b, unsigned int c) {
    //unsigned int d;
    unsigned int c2 = c & 0x7777;
    //asm("prmt.b32 \t%0, %1, %2, %3;" : "=r"(d):"r"(a),"r"(b),"r"(c2));
    //return d;
    return __nvvm_prmt(a, b, c2);
}
#endif /* __CUDA_ARCH__ >= 200 */

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __pm0(void) {
    unsigned int r;
    asm("mov.u32 \t%0, %%pm0;" : "=r"(r));
    return r;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __pm1(void) {
    unsigned int r;
    asm("mov.u32 \t%0, %%pm1;" : "=r"(r));
    return r;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __pm2(void) {
    unsigned int r;
    asm("mov.u32 \t%0, %%pm2;" : "=r"(r));
    return r;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __pm3(void) {
    unsigned int r;
    asm("mov.u32 \t%0, %%pm3;" : "=r"(r));
    return r;
}

__attribute__((nv_linkonce_odr)) __inline__ void __trap(void) {
    asm __volatile__ ("trap;");
}
    

//
// Math: Integer
//

__attribute__((nv_linkonce_odr)) __inline__ int min(int x, int y) {
    //int z;
    //asm("min.s32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_min_i(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int umin(unsigned int x, unsigned int y) {
    //unsigned int z;
    //asm("min.u32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_min_ui(x, y);
}
    
__attribute__((nv_linkonce_odr)) __inline__ long long llmin(long long x, long long y) {
    //long long z;
    //asm("min.s64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_min_ll(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long ullmin(unsigned long long x, unsigned long long y) {
    //unsigned long long z;
    //asm("min.u64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_min_ull(x, y);
}
    
__attribute__((nv_linkonce_odr)) __inline__ int max(int x, int y) {
    //int z;
    //asm("max.s32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_max_i(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int umax(unsigned int x, unsigned int y) {
    //unsigned int z;
    //asm("max.u32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_max_ui(x, y);
}
    
__attribute__((nv_linkonce_odr)) __inline__ long long llmax(long long x, long long y) {
    //long long z;
    //asm("max.s64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_max_ll(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long ullmax(unsigned long long x, unsigned long long y) {
    //unsigned long long z;
    //asm("max.u64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_max_ull(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ int __mulhi(int x, int y) {
    //int z;
    //asm("mul.hi.s32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_mulhi_i(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __umulhi(unsigned int x, unsigned int y) {
    //unsigned int z;
    //asm("mul.hi.u32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
    //return z;
    return __nvvm_mulhi_ui(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ long long __mul64hi(long long x, long long y) {
    //long long z;
    //asm("mul.hi.s64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_mulhi_ll(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __umul64hi(unsigned long long x, unsigned long long y) {
    //unsigned long long z;
    //asm("mul.hi.u64 \t%0, %1, %2;" : "=l"(z) : "l"(x),"l"(y));
    //return z;
    return __nvvm_mulhi_ull(x, y);
}

__attribute__((nv_linkonce_odr)) __inline__ int __mul24(int x, int y) {
//    int z;
//    asm("mul24.lo.s32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
//    return z;
    return __nvvm_mul24_i(x,y);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __umul24(unsigned int x, unsigned int y) {
//    unsigned int z;
//    asm("mul24.lo.u32 \t%0, %1, %2;" : "=r"(z) : "r"(x),"r"(y));
//    return z;
    return __nvvm_mul24_ui(x,y);
}

#if __CUDA_ARCH__ >= 200
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __brev(unsigned int x) {
    //unsigned int z;
    //asm("brev.b32 \t%0, %1;" : "=r"(z) : "r"(x));
    //return z;
    return __nvvm_brev32(x);
}
    
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __brevll(unsigned long long x) {
    //unsigned long long z;
    //asm("brev.b64 \t%0, %1;" : "=l"(z) : "l"(x));
    //return z;
    return __nvvm_brev64(x);
}
#endif /* __CUDA_ARCH__ >= 200 */
    
__attribute__((nv_linkonce_odr)) __inline__ int __sad(int x, int y, int z) {
    //int a;
    //asm("sad.s32 \t%0, %1, %2, %3;" : "=r"(a) : "r"(x),"r"(y),"r"(z));
    //return a;
    return __nvvm_sad_i(x, y, z);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __usad(unsigned int x, unsigned int y, unsigned int z) {
    //unsigned int a;
    //asm("sad.u32 \t%0, %1, %2, %3;" : "=r"(a) : "r"(x),"r"(y),"r"(z));
    //return a;
    return __nvvm_sad_ui(x, y, z);
}

__attribute__((nv_linkonce_odr)) __inline__ int abs(int x) {
    //int z;
    //asm("abs.s32 \t%0, %1;" : "=r"(z) : "r"(x));
    //return z;
    return __nvvm_abs_i(x);
}

__attribute__((nv_linkonce_odr)) __inline__ long labs(long x) {
#if defined(__LP64__)
    //long long z;
    //asm("abs.s64 \t%0, %1;" : "=l"(z) : "l"(x));
    //return z;
    return __nvvm_abs_ll(x);
#else
    //int z;
    //asm("abs.s32 \t%0, %1;" : "=r"(z) : "r"(x));
    //return z;
    return __nvvm_abs_i(x);
#endif
}

__attribute__((nv_linkonce_odr)) __inline__ long long llabs(long long x) {
    //long long z;
    //asm("abs.s64 \t%0, %1;" : "=l"(z) : "l"(x));
    //return z;
    return __nvvm_abs_ll(x);
}

//
// Math: FP
//

__attribute__((nv_linkonce_odr)) __inline__ float floorf(float f) {
    //float r;
    #if __CUDA_FTZ
    //asm("cvt.rmi.ftz.f32.f32 \t%0, %1;" : "=f"(r) : "f"(f));
    return __nvvm_floor_ftz_f(f);
    #else
    //asm("cvt.rmi.f32.f32 \t%0, %1;" : "=f"(r) : "f"(f));
    return __nvvm_floor_f(f);
    #endif
    //return r;
}

#if __CUDA_ARCH__ >= 130
__attribute__((nv_linkonce_odr)) __inline__ double floor(double f) {
    //double r;
    //asm("cvt.rmi.f64.f64 \t%0, %1;" : "=d"(r) : "d"(f));
    //return r;
    return __nvvm_floor_d(f);
}
#endif /* __CUDA_ARCH__ >= 130 */


__attribute__((nv_linkonce_odr)) __inline__ float fabsf(float f) {
    //float r;
    #if __CUDA_FTZ
    //asm("abs.ftz.f32 \t%0, %1;" : "=f"(r) : "f"(f));
    return __nvvm_fabs_ftz_f(f);
    #else
    //asm("abs.f32 \t%0, %1;" : "=f"(r) : "f"(f));
    return __nvvm_fabs_f(f);
    #endif
    //return r;
}

#if __CUDA_ARCH__ >= 130
__attribute__((nv_linkonce_odr)) __inline__ double fabs(double f) {
    //double r;
    //asm("abs.f64 \t%0, %1;" : "=d"(r) : "d"(f));
    //return r;
    return __nvvm_fabs_d(f);
}
#endif /* __CUDA_ARCH__ >= 130 */

__attribute__((nv_linkonce_odr)) __inline__ double __rcp64h(double d) {
    //double r;
    //asm("rcp.approx.ftz.f64 \t%0, %1;" : "=d"(r) : "d"(d));
    //return r;
    return __nvvm_rcp_approx_ftz_d(d);
}

__attribute__((nv_linkonce_odr)) __inline__ float fminf(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("min.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_fmin_ftz_f(x, y);
    #else
    //asm("min.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_fmin_f(x, y);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float fmaxf(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("max.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_fmax_ftz_f(x, y);
    #else
    //asm("max.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_fmax_f(x, y);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float rsqrtf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("rsqrt.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rsqrt_approx_ftz_f(x);
    #else
    //asm("rsqrt.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rsqrt_approx_f(x);
    #endif
    //return z;
}

#if __CUDA_ARCH__ >= 130
__attribute__((nv_linkonce_odr)) __inline__ double fmin(double x, double y) {
    //double z;
    //asm("min.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_fmin_d(x,y);
}

__attribute__((nv_linkonce_odr)) __inline__ double fmax(double x, double y) {
    //double z;
    //asm("max.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_fmax_d(x,y);
}

__attribute__((nv_linkonce_odr)) __inline__ double rsqrt(double x) {
    //double z;
    //asm("rsqrt.approx.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    return __nvvm_rsqrt_approx_d(x);
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ double ceil(double x) {
    //double z;
    //asm("cvt.rpi.f64.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_ceil_d(x);
}

__attribute__((nv_linkonce_odr)) __inline__ double trunc(double x) {
    //double z;
    //asm("cvt.rzi.f64.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_trunc_d(x);
}
#endif // __CUDA_ARCH__ >= 130

#if __CUDA_ARCH__ >= 200
__attribute__((nv_linkonce_odr)) __inline__ double __round(double x) {
    //double z;
    //asm("cvt.rni.f64.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_round_d(x);
}

__attribute__((nv_linkonce_odr)) __inline__ double __exp2(double x) {
    //double z;
    //asm("ex2.approx.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_ex2_approx_d(x);
}
    
__attribute__((nv_linkonce_odr)) __inline__ double __log2(double x) {
    //double z;
    //asm("lg2.approx.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_lg2_approx_d(x);
}
#endif /* __CUDA_ARCH__ >= 200 */

__attribute__((nv_linkonce_odr)) __inline__ float __roundf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_round_ftz_f(x);
    #else
    //asm("cvt.rni.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_round_f(x);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float __exp2f(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("ex2.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_ex2_approx_ftz_f(x);
    #else
    //asm("ex2.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_ex2_approx_f(x);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float exp2f(float x) {
    return __exp2f(x);
}

__attribute__((nv_linkonce_odr)) __inline__ float __builtin_log2f(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("lg2.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_lg2_approx_ftz_f(x);
    #else
    //asm("lg2.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_lg2_approx_f(x);
    #endif
    //return z;
}


__attribute__((nv_linkonce_odr)) __inline__ float __builtin_sinf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("sin.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sin_approx_ftz_f(x);
    #else
    //asm("sin.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sin_approx_f(x);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float __builtin_cosf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cos.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_cos_approx_ftz_f(x);
    #else
    //asm("cos.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_cos_approx_f(x);
    #endif
    //return z;
}


__attribute__((nv_linkonce_odr)) __inline__ float __builtin_roundf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_round_ftz_f(x);
    #else
    //asm("cvt.rni.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_round_f(x);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ double __builtin_round(double x) {
    //double z;
    //asm("cvt.rni.f64.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_round_d(x);
}

__attribute__((nv_linkonce_odr)) __inline__ float truncf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cvt.rzi.ftz.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_trunc_ftz_f(x);
    #else
    //asm("cvt.rzi.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_trunc_f(x);
    #endif
    //return z;
}

__attribute__((nv_linkonce_odr)) __inline__ float ceilf(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cvt.rpi.ftz.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_ceil_ftz_f(x);
    #else
    //asm("cvt.rpi.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_ceil_f(x);
    #endif
    //return z;
}


// __saturate
__attribute__((nv_linkonce_odr)) __inline__ double __saturate(double x) {
    //double z;
    //asm("cvt.sat.f64.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_saturate_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ float __saturatef(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("cvt.sat.ftz.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_saturate_ftz_f(x);
    #else
    //asm("cvt.sat.f32.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_saturate_f(x);
    #endif
    //return z;
}

#if __CUDA_ARCH__ >= 200
// fmaf
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_rn(float x, float y, float z) {
    //float a;
    #if __CUDA_FTZ
    //asm("fma.rn.ftz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rn_ftz_f(x, y, z);
    #else
    //asm("fma.rn.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rn_f(x, y, z);
    #endif
    //return a;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_rz(float x, float y, float z) {
    //float a;
    #if __CUDA_FTZ
    //asm("fma.rz.ftz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rz_ftz_f(x, y, z);
    #else
    //asm("fma.rz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rz_f(x, y, z);
    #endif
    //return a;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_rd(float x, float y, float z) {
    //float a;
    #if __CUDA_FTZ
    //asm("fma.rm.ftz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rm_ftz_f(x, y, z);
    #else
    //asm("fma.rm.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rm_f(x, y, z);
    #endif
    //return a;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_ru(float x, float y, float z) {
    //float a;
    #if __CUDA_FTZ
    //asm("fma.rp.ftz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rp_ftz_f(x, y, z);
    #else
    //asm("fma.rp.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rp_f(x, y, z);
    #endif
    //return a;
}

// fmaf_ieee
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_ieee_rn(float x, float y, float z) {
    //float a;
    //asm("fma.rn.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    //return a;
    return __nvvm_fma_rn_f(x, y, z);
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_ieee_rz(float x, float y, float z) {
    //float a;
    //asm("fma.rz.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    //return a;
    return __nvvm_fma_rz_f(x, y, z);
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_ieee_rd(float x, float y, float z) {
    //float a;
    //asm("fma.rm.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rm_f(x, y, z);
    //return a;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmaf_ieee_ru(float x, float y, float z) {
    //float a;
    //asm("fma.rp.f32 \t%0, %1, %2, %3;" : "=f"(a) : "f"(x),"f"(y),"f"(z));
    return __nvvm_fma_rp_f(x, y, z);
    //return a;
}
#endif /* __CUDA_ARCH__ >= 200 */

#if __CUDA_ARCH__ >= 130
// fma
__attribute__((nv_linkonce_odr)) __inline__ double __fma_rn(double x, double y, double z) {
    //double a;
    //asm("mad.rn.f64 \t%0, %1, %2, %3;" : "=d"(a) : "d"(x),"d"(y),"d"(z));
    //return a;
    return __nvvm_fma_rn_d(x, y, z);
}
__attribute__((nv_linkonce_odr)) __inline__ double __fma_rz(double x, double y, double z) {
    //double a;
    //asm("mad.rz.f64 \t%0, %1, %2, %3;" : "=d"(a) : "d"(x),"d"(y),"d"(z));
    //return a;
    return __nvvm_fma_rz_d(x, y, z);
}
__attribute__((nv_linkonce_odr)) __inline__ double __fma_rd(double x, double y, double z) {
    //double a;
    //asm("mad.rm.f64 \t%0, %1, %2, %3;" : "=d"(a) : "d"(x),"d"(y),"d"(z));
    //return a;
    return __nvvm_fma_rm_d(x, y, z);
}
__attribute__((nv_linkonce_odr)) __inline__ double __fma_ru(double x, double y, double z) {
    //double a;
    //asm("mad.rp.f64 \t%0, %1, %2, %3;" : "=d"(a) : "d"(x),"d"(y),"d"(z));
    //return a;
    return __nvvm_fma_rp_d(x, y, z);
}
#endif /* __CUDA_ARCH__ >= 130 */

// fdividef
__attribute__((nv_linkonce_odr)) __inline__ float __fdividef(float x, float y) {
    //float z;
#if __CUDA_ARCH__ >= 200
    #if __CUDA_FTZ
    //asm("div.approx.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_approx_ftz_f(x, y);
    #else
    //asm("div.approx.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_approx_f(x, y);
    #endif
#else
    //asm("div.approx.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_approx_f(x, y);
#endif
    //return z;
}


#if __CUDA_ARCH__ >= 200
// fdiv
__attribute__((nv_linkonce_odr)) __inline__ float __fdiv_rn(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("div.rn.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rn_ftz_f(x, y);
    #else
    //asm("div.rn.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rn_f(x, y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fdiv_rz(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("div.rz.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rz_ftz_f(x, y);
    #else
    //asm("div.rz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rz_f(x, y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fdiv_rd(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("div.rm.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rm_ftz_f(x, y);
    #else
    //asm("div.rm.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rm_f(x, y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fdiv_ru(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("div.rp.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rp_ftz_f(x, y);
    #else
    //asm("div.rp.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_div_rp_f(x, y);
    #endif
    //return z;
}

// frcp
__attribute__((nv_linkonce_odr)) __inline__ float __frcp_rn(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("rcp.rn.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rn_ftz_f(x);
    #else
    //asm("rcp.rn.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rn_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __frcp_rz(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("rcp.rz.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rz_ftz_f(x);
    #else
    //asm("rcp.rz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rz_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __frcp_rd(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("rcp.rm.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rm_ftz_f(x);
    #else
    //asm("rcp.rm.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rm_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __frcp_ru(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("rcp.rp.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rp_ftz_f(x);
    #else
    //asm("rcp.rp.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_rcp_rp_f(x);
    #endif
    //return z;
}

// fsqrt
__attribute__((nv_linkonce_odr)) __inline__ float __fsqrt_rn(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("sqrt.rn.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rn_ftz_f(x);
    #else
    //asm("sqrt.rn.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rn_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fsqrt_rz(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("sqrt.rz.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rz_ftz_f(x);
    #else
    //asm("sqrt.rz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rz_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fsqrt_rd(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("sqrt.rm.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rm_ftz_f(x);
    #else
    //asm("sqrt.rm.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rm_f(x);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fsqrt_ru(float x) {
    //float z;
    #if __CUDA_FTZ
    //asm("sqrt.rp.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rp_ftz_f(x);
    #else
    //asm("sqrt.rp.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rp_f(x);
    #endif
    //return z;
}

// ddiv
__attribute__((nv_linkonce_odr)) __inline__ double __ddiv_rn(double x, double y) {
    //double z;
    //asm("div.rn.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_div_rn_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ddiv_rz(double x, double y) {
    //double z;
    //asm("div.rz.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_div_rz_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ddiv_rd(double x, double y) {
    //double z;
    //asm("div.rm.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_div_rm_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ddiv_ru(double x, double y) {
    //double z;
    //asm("div.rp.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_div_rp_d(x, y);
}

// drcp
__attribute__((nv_linkonce_odr)) __inline__ double __drcp_rn(double x) {
    //double z;
    //asm("rcp.rn.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_rcp_rn_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ double __drcp_rz(double x) {
    //double z;
    //asm("rcp.rz.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_rcp_rz_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ double __drcp_rd(double x) {
    //double z;
    //asm("rcp.rm.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_rcp_rm_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ double __drcp_ru(double x) {
    //double z;
    //asm("rcp.rp.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_rcp_rp_d(x);
}

// dsqrt
__attribute__((nv_linkonce_odr)) __inline__ double __dsqrt_rn(double x) {
    //double z;
    //asm("sqrt.rn.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_sqrt_rn_d(x);
}

__attribute__((nv_linkonce_odr)) __inline__ double __dsqrt_rz(double x) {
    //double z;
    //asm("sqrt.rz.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_sqrt_rz_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dsqrt_rd(double x) {
    //double z;
    //asm("sqrt.rm.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_sqrt_rm_d(x);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dsqrt_ru(double x) {
    //double z;
    //asm("sqrt.rp.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_sqrt_rp_d(x);
}

#endif /* __CUDA_ARCH__ >= 200 */

__attribute__((nv_linkonce_odr)) __inline__ float sqrtf(float x) {
    //float z;
#if __CUDA_ARCH__ >= 200
  #if __CUDA_PREC_SQRT
    #if __CUDA_FTZ
    //asm("sqrt.rn.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rn_ftz_f(x);
    #else
    //asm("sqrt.rn.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_rn_f(x);
    #endif
  #else /* !__CUDA_PREC_SQRT */
    #if __CUDA_FTZ
    //asm("sqrt.approx.ftz.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_approx_ftz_f(x);
    #else
    //asm("sqrt.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_approx_f(x);
    #endif
  #endif
#else
    //asm("sqrt.approx.f32 \t%0, %1;" : "=f"(z) : "f"(x));
    return __nvvm_sqrt_approx_f(x);
#endif /* else __CUDA_ARCH__ >= 200 */
    //return z;
}

#if __CUDA_ARCH__ >= 130
// sqrt
__attribute__((nv_linkonce_odr)) __inline__ double sqrt(double x) {
    //double z;
    //asm("sqrt.rn.f64 \t%0, %1;" : "=d"(z) : "d"(x));
    //return z;
    return __nvvm_sqrt_rn_d(x);
}

// dadd
__attribute__((nv_linkonce_odr)) __inline__ double __dadd_rn(double x, double y) {
    //double z;
    //asm("add.rn.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_add_rn_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dadd_rz(double x, double y) {
    //double z;
    //asm("add.rz.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_add_rz_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dadd_rd(double x, double y) {
    //double z;
    //asm("add.rm.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_add_rm_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dadd_ru(double x, double y) {
    //double z;
    //asm("add.rp.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_add_rp_d(x, y);
}

// dmul
__attribute__((nv_linkonce_odr)) __inline__ double __dmul_rn(double x, double y) {
    //double z;
    //asm("mul.rn.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_mul_rn_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dmul_rz(double x, double y) {
    //double z;
    //asm("mul.rz.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_mul_rz_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dmul_rd(double x, double y) {
    //double z;
    //asm("mul.rm.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_mul_rm_d(x, y);
}
__attribute__((nv_linkonce_odr)) __inline__ double __dmul_ru(double x, double y) {
    //double z;
    //asm("mul.rp.f64 \t%0, %1, %2;" : "=d"(z) : "d"(x),"d"(y));
    //return z;
    return __nvvm_mul_rp_d(x, y);
}
#endif /* __CUDA_ARCH__ >= 130 */

#if __CUDA_ARCH__ >= 200
// fadd rd ru
__attribute__((nv_linkonce_odr)) __inline__ float __fadd_rd(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("add.rm.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rm_ftz_f(x,y);
    #else
    //asm("add.rm.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rm_f(x,y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fadd_ru(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("add.rp.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rp_ftz_f(x,y);
    #else
    //asm("add.rp.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rp_f(x,y);
    #endif
    //return z;
}

// fmul rd ru
__attribute__((nv_linkonce_odr)) __inline__ float __fmul_rd(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("mul.rm.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rm_ftz_f(x,y);
    #else
    //asm("mul.rm.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rm_f(x,y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmul_ru(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("mul.rp.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rp_ftz_f(x,y);
    #else
    //asm("mul.rp.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rp_f(x,y);
    #endif
    //return z;
}

#endif /* __CUDA_ARCH__ >= 200 */

// fadd rn rz
__attribute__((nv_linkonce_odr)) __inline__ float __fadd_rn(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("add.rn.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rn_ftz_f(x,y);
    #else
    //asm("add.rn.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rn_f(x,y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fadd_rz(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("add.rz.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rz_ftz_f(x,y);
    #else
    //asm("add.rz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_add_rz_f(x,y);
    #endif
    //return z;
}

// fmul rn rz
__attribute__((nv_linkonce_odr)) __inline__ float __fmul_rn(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("mul.rn.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rn_ftz_f(x,y);
    #else
    //asm("mul.rn.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rn_f(x,y);
    #endif
    //return z;
}
__attribute__((nv_linkonce_odr)) __inline__ float __fmul_rz(float x, float y) {
    //float z;
    #if __CUDA_FTZ
    //asm("mul.rz.ftz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rz_ftz_f(x,y);
    #else
    //asm("mul.rz.f32 \t%0, %1, %2;" : "=f"(z) : "f"(x),"f"(y));
    return __nvvm_mul_rz_f(x,y);
    #endif
    //return z;
}

//
// Conversion
//

// double to float
__attribute__((nv_linkonce_odr)) __inline__ float __double2float_rn(double d) {
    //float f;
    #if __CUDA_FTZ
    //asm("cvt.rn.ftz.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rn_ftz(d);
    #else
    //asm("cvt.rn.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rn(d);
    #endif
    //return f;
}
__attribute__((nv_linkonce_odr)) __inline__ float __double2float_rz(double d) {
    //float f;
    #if __CUDA_FTZ
    //asm("cvt.rz.ftz.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rz_ftz(d);
    #else
    //asm("cvt.rz.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rz(d);
    #endif
    //return f;
}
__attribute__((nv_linkonce_odr)) __inline__ float __double2float_rd(double d) {
    //float f;
    #if __CUDA_FTZ
    //asm("cvt.rm.ftz.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rm_ftz(d);
    #else
    //asm("cvt.rm.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rm(d);
    #endif
    //return f;
}
__attribute__((nv_linkonce_odr)) __inline__ float __double2float_ru(double d) {
    //float f;
    #if __CUDA_FTZ
    //asm("cvt.rp.ftz.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rp_ftz(d);
    #else
    //asm("cvt.rp.f32.f64 \t%0, %1;" : "=f"(f) : "d"(d));
    return __nvvm_d2f_rp(d);
    #endif
    //return f;
}
    
// double to int
__attribute__((nv_linkonce_odr)) __inline__ int __double2int_rn(double d) {
    //int f;
    //asm("cvt.rni.s32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2i_rn(d);
}
__attribute__((nv_linkonce_odr)) __inline__ int __double2int_rz(double d) {
    //int f;
    //asm("cvt.rzi.s32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2i_rz(d);
}
__attribute__((nv_linkonce_odr)) __inline__ int __double2int_rd(double d) {
    //int f;
    //asm("cvt.rmi.s32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2i_rm(d);
}
__attribute__((nv_linkonce_odr)) __inline__ int __double2int_ru(double d) {
    //int f;
    //asm("cvt.rpi.s32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2i_rp(d);
}

// double to uint
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __double2uint_rn(double d) {
    //unsigned int f;
    //asm("cvt.rni.u32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2ui_rn(d);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __double2uint_rz(double d) {
    //unsigned int f;
    //asm("cvt.rzi.u32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2ui_rz(d);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __double2uint_rd(double d) {
    //unsigned int f;
    //asm("cvt.rmi.u32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2ui_rm(d);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __double2uint_ru(double d) {
    //unsigned int f;
    //asm("cvt.rpi.u32.f64 \t%0, %1;" : "=r"(f) : "d"(d));
    //return f;
    return __nvvm_d2ui_rp(d);
}

// int to double 
__attribute__((nv_linkonce_odr)) __inline__ double __int2double_rn(int i) {
    //double d;
    //asm("cvt.rn.f64.s32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_i2d_rn(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __int2double_rz(int i) {
    //double d;
    //asm("cvt.rz.f64.s32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_i2d_rz(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __int2double_rd(int i) {
    //double d;
    //asm("cvt.rm.f64.s32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_i2d_rm(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __int2double_ru(int i) {
    //double d;
    //asm("cvt.rp.f64.s32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_i2d_rp(i);
}

// uint to double 
__attribute__((nv_linkonce_odr)) __inline__ double __uint2double_rn(unsigned int i) {
    //double d;
    //asm("cvt.rn.f64.u32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_ui2d_rn(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __uint2double_rz(unsigned int i) {
    //double d;
    //asm("cvt.rz.f64.u32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_ui2d_rz(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __uint2double_rd(unsigned int i) {
    //double d;
    //asm("cvt.rm.f64.u32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_ui2d_rm(i);
}
__attribute__((nv_linkonce_odr)) __inline__ double __uint2double_ru(unsigned int i) {
    //double d;
    //asm("cvt.rp.f64.u32 \t%0, %1;" : "=d"(d) : "r"(i));
    //return d;
    return __nvvm_ui2d_rp(i);
}

// float to int
__attribute__((nv_linkonce_odr)) __inline__ int __float2int_rn(float in) {
    //int out;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rn_ftz(in);
    #else
    //asm("cvt.rni.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rn(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ int __float2int_rz(float in) {
    //int out;
    #if __CUDA_FTZ
    //asm("cvt.rzi.ftz.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rz_ftz(in);
    #else
    //asm("cvt.rzi.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rz(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ int __float2int_rd(float in) {
    //int out;
    #if __CUDA_FTZ
    //asm("cvt.rmi.ftz.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rm_ftz(in);
    #else
    //asm("cvt.rmi.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rm(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ int __float2int_ru(float in) {
    //int out;
    #if __CUDA_FTZ
    //asm("cvt.rpi.ftz.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rp_ftz(in);
    #else
    //asm("cvt.rpi.s32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2i_rp(in);
    #endif
    //return out;
}

// float to uint
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __float2uint_rn(float in) {
    //unsigned int out;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rn_ftz(in);
    #else
    //asm("cvt.rni.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rn(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __float2uint_rz(float in) {
    //unsigned int out;
    #if __CUDA_FTZ
    //asm("cvt.rzi.ftz.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rz_ftz(in);
    #else
    //asm("cvt.rzi.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rz(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __float2uint_rd(float in) {
    //unsigned int out;
    #if __CUDA_FTZ
    //asm("cvt.rmi.ftz.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rm_ftz(in);
    #else
    //asm("cvt.rmi.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rm(in);
    #endif
    //return out;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned int __float2uint_ru(float in) {
    //unsigned int out;
    #if __CUDA_FTZ
    //asm("cvt.rpi.ftz.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rp_ftz(in);
    #else
    //asm("cvt.rpi.u32.f32 \t%0, %1;" : "=r"(out) : "f"(in));
    return __nvvm_f2ui_rp(in);
    #endif
    //return out;
}

// int to float
__attribute__((nv_linkonce_odr)) __inline__ float __int2float_rn(int in) {
    //float out;
    //asm("cvt.rn.f32.s32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_i2f_rn(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __int2float_rz(int in) {
    //float out;
    //asm("cvt.rz.f32.s32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_i2f_rz(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __int2float_rd(int in) {
    //float out;
    //asm("cvt.rm.f32.s32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_i2f_rm(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __int2float_ru(int in) {
    //float out;
    //asm("cvt.rp.f32.s32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_i2f_rp(in);
}

// unsigned int to float
__attribute__((nv_linkonce_odr)) __inline__ float __uint2float_rn(unsigned int in) {
    //float out;
    //asm("cvt.rn.f32.u32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_ui2f_rn(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __uint2float_rz(unsigned int in) {
    //float out;
    //asm("cvt.rz.f32.u32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_ui2f_rz(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __uint2float_rd(unsigned int in) {
    //float out;
    //asm("cvt.rm.f32.u32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_ui2f_rm(in);
}
__attribute__((nv_linkonce_odr)) __inline__ float __uint2float_ru(unsigned int in) {
    //float out;
    //asm("cvt.rp.f32.u32 \t%0, %1;" : "=f"(out) : "r"(in));
    //return out;
    return __nvvm_ui2f_rp(in);
}

// hiloint vs double
__attribute__((nv_linkonce_odr)) __inline__ double __hiloint2double(int a, int b) {
    //double d;
    //asm("mov.b64 \t%0, {%1, %2};" : "=d"(d) : "r"(b),"r"(a));
    //return d;
    return __nvvm_lohi_i2d(b, a);
}
__attribute__((nv_linkonce_odr)) __inline__ int __double2loint(double d) {
    //int a;
    //asm("{ \n\t"
    //    ".reg .b32 \ttemp; \n\t"
    //    "mov.b64 \t{%0, temp}, %1; \n\t"
    //    "}" : "=r"(a) : "d"(d));
    //return a;
    return __nvvm_d2i_lo(d);
}
__attribute__((nv_linkonce_odr)) __inline__ int __double2hiint(double d) {
    //int a;
    //asm("{ \n\t"
    //    ".reg .b32 \ttemp; \n\t"
    //    "mov.b64 \t{temp, %0}, %1; \n\t"
    //    "}" : "=r"(a) : "d"(d));
    //return a;
    return __nvvm_d2i_hi(d);
}

// float2ll
__attribute__((nv_linkonce_odr)) __inline__ long long __float2ll_rn(float f) {
    //long long l;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rn_ftz(f);
    #else
    //asm("cvt.rni.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rn(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ long long __float2ll_rz(float f) {
    //long long l;
    #if __CUDA_FTZ
    //asm("cvt.rzi.ftz.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rz_ftz(f);
    #else
    //asm("cvt.rzi.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rz(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ long long __float2ll_rd(float f) {
    //long long l;
    #if __CUDA_FTZ
    //asm("cvt.rmi.ftz.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rm_ftz(f);
    #else
    //asm("cvt.rmi.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rm(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ long long __float2ll_ru(float f) {
    //long long l;
    #if __CUDA_FTZ
    //asm("cvt.rpi.ftz.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rp_ftz(f);
    #else
    //asm("cvt.rpi.s64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ll_rp(f);
    #endif
    //return l;
}

// float2ull
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __float2ull_rn(float f) {
    //unsigned long long l;
    #if __CUDA_FTZ
    //asm("cvt.rni.ftz.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rn_ftz(f);
    #else
    //asm("cvt.rni.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rn(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __float2ull_rz(float f) {
    //unsigned long long l;
    #if __CUDA_FTZ
    //asm("cvt.rzi.ftz.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rz_ftz(f);
    #else
    //asm("cvt.rzi.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rz(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __float2ull_rd(float f) {
    //unsigned long long l;
    #if __CUDA_FTZ
    //asm("cvt.rmi.ftz.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rm_ftz(f);
    #else
    //asm("cvt.rmi.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rm(f);
    #endif
    //return l;
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __float2ull_ru(float f) {
    //unsigned long long l;
    #if __CUDA_FTZ
    //asm("cvt.rpi.ftz.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rp_ftz(f);
    #else
    //asm("cvt.rpi.u64.f32 \t%0, %1;" : "=l"(l) : "f"(f));
    return __nvvm_f2ull_rp(f);
    #endif
    //return l;
}

// double2ll
__attribute__((nv_linkonce_odr)) __inline__ long long __double2ll_rn(double f) {
    //long long l;
    //asm("cvt.rni.s64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ll_rn(f);
}
__attribute__((nv_linkonce_odr)) __inline__ long long __double2ll_rz(double f) {
    //long long l;
    //asm("cvt.rzi.s64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ll_rz(f);
}
__attribute__((nv_linkonce_odr)) __inline__ long long __double2ll_rd(double f) {
    //long long l;
    //asm("cvt.rmi.s64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ll_rm(f);
}
__attribute__((nv_linkonce_odr)) __inline__ long long __double2ll_ru(double f) {
    //long long l;
    //asm("cvt.rpi.s64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ll_rp(f);
}

// double2ull
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __double2ull_rn(double f) {
    //unsigned long long l;
    //asm("cvt.rni.u64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ull_rn(f);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __double2ull_rz(double f) {
    //unsigned long long l;
    //asm("cvt.rzi.u64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ull_rz(f);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __double2ull_rd(double f) {
    //unsigned long long l;
    //asm("cvt.rmi.u64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ull_rm(f);
}
__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __double2ull_ru(double f) {
    //unsigned long long l;
    //asm("cvt.rpi.u64.f64 \t%0, %1;" : "=l"(l) : "d"(f));
    //return l;
    return __nvvm_d2ull_rp(f);
}

// ll2float
__attribute__((nv_linkonce_odr)) __inline__ float __ll2float_rn(long long l) {
    //float f;
    //asm("cvt.rn.f32.s64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ll2f_rn(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ll2float_rz(long long l) {
    //float f;
    //asm("cvt.rz.f32.s64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ll2f_rz(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ll2float_rd(long long l) {
    //float f;
    //asm("cvt.rm.f32.s64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ll2f_rm(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ll2float_ru(long long l) {
    //float f;
    //asm("cvt.rp.f32.s64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ll2f_rp(l);
}

// ull2float
__attribute__((nv_linkonce_odr)) __inline__ float __ull2float_rn(unsigned long long l) {
    //float f;
    //asm("cvt.rn.f32.u64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ull2f_rn(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ull2float_rz(unsigned long long l) {
    //float f;
    //asm("cvt.rz.f32.u64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ull2f_rz(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ull2float_rd(unsigned long long l) {
    //float f;
    //asm("cvt.rm.f32.u64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ull2f_rm(l);
}
__attribute__((nv_linkonce_odr)) __inline__ float __ull2float_ru(unsigned long long l) {
    //float f;
    //asm("cvt.rp.f32.u64 \t%0, %1;" : "=f"(f) : "l"(l));
    //return f;
    return __nvvm_ull2f_rp(l);
}

// ll2double
__attribute__((nv_linkonce_odr)) __inline__ double __ll2double_rn(long long l) {
    //double f;
    //asm("cvt.rn.f64.s64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ll2d_rn(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ll2double_rz(long long l) {
    //double f;
    //asm("cvt.rz.f64.s64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ll2d_rz(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ll2double_rd(long long l) {
    //double f;
    //asm("cvt.rm.f64.s64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ll2d_rm(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ll2double_ru(long long l) {
    //double f;
    //asm("cvt.rp.f64.s64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ll2d_rp(l);
}

// ull2double
__attribute__((nv_linkonce_odr)) __inline__ double __ull2double_rn(unsigned long long l) {
    //double f;
    //asm("cvt.rn.f64.u64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ull2d_rn(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ull2double_rz(unsigned long long l) {
    //double f;
    //asm("cvt.rz.f64.u64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ull2d_rz(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ull2double_rd(unsigned long long l) {
    //double f;
    //asm("cvt.rm.f64.u64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ull2d_rm(l);
}
__attribute__((nv_linkonce_odr)) __inline__ double __ull2double_ru(unsigned long long l) {
    //double f;
    //asm("cvt.rp.f64.u64 \t%0, %1;" : "=d"(f) : "l"(l));
    //return f;
    return __nvvm_ull2d_rp(l);
}

// float2half, half2float
__attribute__((nv_linkonce_odr)) __inline__ unsigned short __float2half_rn(float f) {
    //unsigned short h;
    // To get around the type checking
    //asm("{ \n\t"
    //    ".reg .b16 \ttmp; \n\t"
    #if __CUDA_FTZ
    //    "cvt.rn.ftz.f16.f32 \ttmp, %1; \n\t"
    return __nvvm_f2h_rn_ftz(f);
    #else
    //    "cvt.rn.f16.f32 \ttmp, %1; \n\t"
    return __nvvm_f2h_rn(f);
    #endif
    //    "mov.b16 \t%0, tmp; \n\t"
    //    "}" : "=h"(h) : "f"(f));
    //return h;
}
__attribute__((nv_linkonce_odr)) __inline__ float __half2float(unsigned short h) {
    //float f;
    //asm("{ \n\t"
    //    ".reg .b16 \ttmp; \n\t"
    //    "mov.b16 \ttmp, %1; \n\t"
    //    "cvt.f32.f16 \t%0, tmp; \n\t"
    //    "}" : "=f"(f) : "h"(h));
    //return f;
    return __nvvm_h2f(h);
}

// 
// Re-interpretation
//

__attribute__((nv_linkonce_odr)) __inline__ float __int_as_float(int x) {
    //float z;
    //asm("mov.b32 \t%0, %1;" : "=f"(z) : "r"(x));
    //return z;
    return __nvvm_bitcast_i2f(x);
}

__attribute__((nv_linkonce_odr)) __inline__ int __float_as_int(float x) {
    //int z;
    //asm("mov.b32 \t%0, %1;" : "=r"(z) : "f"(x));
    //return z;
    return __nvvm_bitcast_f2i(x);
}
    
__attribute__((nv_linkonce_odr)) __inline__ double __longlong_as_double(long long x) {
    //double z;
    //asm("mov.b64 \t%0, %1;" : "=d"(z) : "l"(x));
    //return z;
    return __nvvm_bitcast_ll2d(x);
}

__attribute__((nv_linkonce_odr)) __inline__ long long  __double_as_longlong (double x) {
    //long long  z;
    //asm("mov.b64 \t%0, %1;" : "=l"(z) : "d"(x));
    //return z;
    return __nvvm_bitcast_d2ll(x);
}
    

// FIXME: once the intrinsics move to their final location in cudart/, the
// __nv_size_t workaround will not be needed, and we can use size_t 
// directly. 

#if (__NV_POINTER_SIZE == 32)
typedef unsigned int __nv_size_t;
#elif (__NV_POINTER_SIZE == 64)
typedef unsigned long long __nv_size_t;
#else
#error "unexpected value for __NV_POINTER_SIZE!"
#endif
__attribute__((nv_linkonce_odr)) __inline__ void* memcpy(void *dest, const void *src, __nv_size_t n) {
    __nvvm_memcpy((unsigned char *)dest, (unsigned char *)src, n, 
                  /*alignment=*/ 1);
    return dest;
}

__attribute__((nv_linkonce_odr)) __inline__ void* memset(void *dest, int c, __nv_size_t n) {
    __nvvm_memset((unsigned char *)dest, (unsigned char)c, n, 
                  /*alignment=*/1);
    return dest;
}


//
// Atomic Operations
//

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicAdd(int *p, int val) {
    return __nvvm_atom_add_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicAdd(unsigned int *p, unsigned int val) {
    return __nvvm_atom_add_gen_i((volatile int *)p, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __ullAtomicAdd(unsigned long long *p, unsigned long long val) {
    return __nvvm_atom_add_gen_ll((volatile long long *)p, (long long)val);
}
    
__attribute__((nv_linkonce_odr)) __inline__ float __fAtomicAdd(float *p, float val) {
    return __nvvm_atom_add_gen_f((volatile float *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicExch(int *p, int val) {
    return __nvvm_atom_xchg_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicExch(unsigned int *p, unsigned int val) {
    return __nvvm_atom_xchg_gen_i((volatile int *)p, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __ullAtomicExch(unsigned long long *p, unsigned long long val) {
    return __nvvm_atom_xchg_gen_ll((volatile long long *)p, (long long)val);
}
    
__attribute__((nv_linkonce_odr)) __inline__ float __fAtomicExch(float *p, float val) {
    int old = __nvvm_atom_xchg_gen_i((volatile int *)p, __float_as_int(val));
    return __int_as_float(old);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicMin(int *p, int val) {
    return __nvvm_atom_min_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ long long __illAtomicMin(long long *p, long long val) {
    return __nvvm_atom_min_gen_ll((volatile long long *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicMin(unsigned int *p, unsigned int val) {
    return __nvvm_atom_min_gen_ui((volatile unsigned int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __ullAtomicMin(unsigned long long *p, unsigned long long val) {
    return __nvvm_atom_min_gen_ull((volatile unsigned long long *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicMax(int *p, int val) {
    return __nvvm_atom_max_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ long long __illAtomicMax(long long *p, long long val) {
    return __nvvm_atom_max_gen_ll((volatile long long *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicMax(unsigned int *p, unsigned int val) {
    return __nvvm_atom_max_gen_ui((unsigned int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long __ullAtomicMax(unsigned long long *p, unsigned long long val) {
    return __nvvm_atom_max_gen_ull((volatile unsigned long long *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicInc(unsigned int *p, unsigned int val) {
    return __nvvm_atom_inc_gen_ui((unsigned int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicDec(unsigned int *p, unsigned int val) {
    return __nvvm_atom_dec_gen_ui((unsigned int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicCAS(int *p, int compare, int val) {
    return __nvvm_atom_cas_gen_i((int *)p, compare, val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicCAS(unsigned int *p, unsigned int compare, unsigned int val) {
    return (unsigned int)__nvvm_atom_cas_gen_i((volatile int *)p, (int)compare, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long int __ullAtomicCAS(unsigned long long int *p, unsigned long long int compare, unsigned long long int val) {
    return (unsigned long long int)__nvvm_atom_cas_gen_ll((volatile long long int *)p, (long long int)compare, (long long int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicAnd(int *p, int val) {
    return __nvvm_atom_and_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ long long int __llAtomicAnd(long long int *p, long long int val) {
    return __nvvm_atom_and_gen_ll((volatile long long int *)p, (long long)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicAnd(unsigned int *p, unsigned int val) {
    return (unsigned int)__nvvm_atom_and_gen_i((volatile int *)p, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long int __ullAtomicAnd(unsigned long long int *p, unsigned long long int val) {
    return __nvvm_atom_and_gen_ll((volatile long long int *)p, (long long)val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicOr(int *p, int val) {
    return __nvvm_atom_or_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ long long int __llAtomicOr(long long int *p, long long int val) {
    return __nvvm_atom_or_gen_ll((volatile long long int *)p, (long long)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicOr(unsigned int *p, unsigned int val) {
    return (unsigned int)__nvvm_atom_or_gen_i((volatile int *)p, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long int __ullAtomicOr(unsigned long long int *p, unsigned long long int val) {
    return __nvvm_atom_or_gen_ll((volatile long long int *)p, (long long)val);
}

__attribute__((nv_linkonce_odr)) __inline__ int __iAtomicXor(int *p, int val) {
    return __nvvm_atom_xor_gen_i((volatile int *)p, val);
}

__attribute__((nv_linkonce_odr)) __inline__ long long int __llAtomicXor(long long int *p, long long int val) {
    return __nvvm_atom_xor_gen_ll((volatile long long int *)p, (long long)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned int __uAtomicXor(unsigned int *p, unsigned int val) {
    return (unsigned int)__nvvm_atom_xor_gen_i((volatile int *)p, (int)val);
}

__attribute__((nv_linkonce_odr)) __inline__ unsigned long long int __ullAtomicXor(unsigned long long int *p, unsigned long long int val) {
    return __nvvm_atom_xor_gen_ll((volatile long long int *)p, (long long)val);
}

