/*
 * Copyright (c) 2012, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL FREESCALE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#ifndef _REGS_H
#define _REGS_H  1
#if 0//COMPILE_ERR
/* for ioremap */
// eim
extern u32	g_nwim_regs;
#define REGS_EIM_BASE (g_nwim_regs) //!< Base address for EIM


// gpio
extern u32 g_ngpio1_regs;
extern u32 g_ngpio2_regs;
extern u32 g_ngpio3_regs;
extern u32 g_ngpio4_regs;
extern u32 g_ngpio5_regs;
extern u32 g_ngpio6_regs;
extern u32 g_ngpio7_regs;

#define HW_GPIO_INSTANCE_COUNT (7) //!< Number of instances of the GPIO module.
#define HW_GPIO1 (1) //!< Instance number for GPIO1.
#define HW_GPIO2 (2) //!< Instance number for GPIO2.
#define HW_GPIO3 (3) //!< Instance number for GPIO3.
#define HW_GPIO4 (4) //!< Instance number for GPIO4.
#define HW_GPIO5 (5) //!< Instance number for GPIO5.
#define HW_GPIO6 (6) //!< Instance number for GPIO6.
#define HW_GPIO7 (7) //!< Instance number for GPIO7.
#define REGS_GPIO1_BASE (g_ngpio1_regs) //!< Base address for GPIO instance number 1.
#define REGS_GPIO2_BASE (g_ngpio2_regs) //!< Base address for GPIO instance number 2.
#define REGS_GPIO3_BASE (g_ngpio3_regs) //!< Base address for GPIO instance number 3.
#define REGS_GPIO4_BASE (g_ngpio4_regs) //!< Base address for GPIO instance number 4.
#define REGS_GPIO5_BASE (g_ngpio5_regs) //!< Base address for GPIO instance number 5.
#define REGS_GPIO6_BASE (g_ngpio6_regs) //!< Base address for GPIO instance number 6.
#define REGS_GPIO7_BASE (g_ngpio7_regs) //!< Base address for GPIO instance number 7.

//! @brief Get the base address of GPIO by instance number.
//! @param x GPIO instance number, from 1 through 7.
#define REGS_GPIO_BASE(x) ( (x) == HW_GPIO1 ? REGS_GPIO1_BASE : (x) == HW_GPIO2 ? REGS_GPIO2_BASE : (x) == HW_GPIO3 ? REGS_GPIO3_BASE : (x) == HW_GPIO4 ? REGS_GPIO4_BASE : (x) == HW_GPIO5 ? REGS_GPIO5_BASE : (x) == HW_GPIO6 ? REGS_GPIO6_BASE : (x) == HW_GPIO7 ? REGS_GPIO7_BASE : 0x00d00000)

//! @brief Get the instance number given a base address.
//! @param b Base address for an instance of GPIO.
#define REGS_GPIO_INSTANCE(b) ( (b) == REGS_GPIO1_BASE ? HW_GPIO1 : (b) == REGS_GPIO2_BASE ? HW_GPIO2 : (b) == REGS_GPIO3_BASE ? HW_GPIO3 : (b) == REGS_GPIO4_BASE ? HW_GPIO4 : (b) == REGS_GPIO5_BASE ? HW_GPIO5 : (b) == REGS_GPIO6_BASE ? HW_GPIO6 : (b) == REGS_GPIO7_BASE ? HW_GPIO7 : 0)
	
// sdma
extern u32	g_nsdma_regs;
#define HW_SDMAARM_INSTANCE_COUNT (1) //!< Number of instances of the SDMAARM module.
#define REGS_SDMAARM_BASE (g_nsdma_regs) //!< Base address for SDMAARM.
#endif

//
// define base address of the register block only if it is not already
// defined, which allows the compiler to override at build time for
// users who've mapped their registers to locations other than the
// physical location
//

#ifndef REGS_BASE
#define REGS_BASE 0x00000000
#endif

//
// common register types
//

#ifndef __LANGUAGE_ASM__
typedef unsigned char reg8_t;
typedef unsigned short reg16_t;
typedef unsigned int reg32_t;
#endif

//
// Typecast macro for C or asm. In C, the cast is applied, while in asm it is excluded. This is
// used to simplify macro definitions in the module register headers.
//
#ifndef __REG_VALUE_TYPE
    #ifndef __LANGUAGE_ASM__
        #define __REG_VALUE_TYPE(v, t) ((t)(v))
    #else
        #define __REG_VALUE_TYPE(v, t) (v)
    #endif
#endif

//
// macros for single instance registers
//

#define BF_SET(reg, field)       HW_##reg##_SET(BM_##reg##_##field)
#define BF_CLR(reg, field)       HW_##reg##_CLR(BM_##reg##_##field)
#define BF_TOG(reg, field)       HW_##reg##_TOG(BM_##reg##_##field)

#define BF_SETV(reg, field, v)   HW_##reg##_SET(BF_##reg##_##field(v))
#define BF_CLRV(reg, field, v)   HW_##reg##_CLR(BF_##reg##_##field(v))
#define BF_TOGV(reg, field, v)   HW_##reg##_TOG(BF_##reg##_##field(v))

#define BV_FLD(reg, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BV_VAL(reg, field, sym)  BV_##reg##_##field##__##sym

#define BF_RD(reg, field)        HW_##reg.B.field
#define BF_WR(reg, field, v)     BW_##reg##_##field(v)

#define BF_CS1(reg, f1, v1)  \
        (HW_##reg##_CLR(BM_##reg##_##f1),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1)))

#define BF_CS2(reg, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2)))

#define BF_CS3(reg, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3)))

#define BF_CS4(reg, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4)))

#define BF_CS5(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5)))

#define BF_CS6(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6)))

#define BF_CS7(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7)))

#define BF_CS8(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7 |      \
                        BM_##reg##_##f8),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7) |  \
                        BF_##reg##_##f8(v8)))

//
// macros for multiple instance registers
//

#define BF_SETn(reg, n, field)       HW_##reg##_SET(n, BM_##reg##_##field)
#define BF_CLRn(reg, n, field)       HW_##reg##_CLR(n, BM_##reg##_##field)
#define BF_TOGn(reg, n, field)       HW_##reg##_TOG(n, BM_##reg##_##field)

#define BF_SETVn(reg, n, field, v)   HW_##reg##_SET(n, BF_##reg##_##field(v))
#define BF_CLRVn(reg, n, field, v)   HW_##reg##_CLR(n, BF_##reg##_##field(v))
#define BF_TOGVn(reg, n, field, v)   HW_##reg##_TOG(n, BF_##reg##_##field(v))

#define BV_FLDn(reg, n, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BV_VALn(reg, n, field, sym)  BV_##reg##_##field##__##sym

#define BF_RDn(reg, n, field)        HW_##reg(n).B.field
#define BF_WRn(reg, n, field, v)     BW_##reg##_##field(n, v)

#define BF_CS1n(reg, n, f1, v1)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1))))

#define BF_CS2n(reg, n, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2))))

#define BF_CS3n(reg, n, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3))))

#define BF_CS4n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4))))

#define BF_CS5n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5))))

#define BF_CS6n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6))))

#define BF_CS7n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7))))

#define BF_CS8n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7 |       \
                            BM_##reg##_##f8)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7) |   \
                            BF_##reg##_##f8(v8))))

//
// macros for single instance MULTI-BLOCK registers
//

#define BFn_SET(reg, blk, field)       HW_##reg##_SET(blk, BM_##reg##_##field)
#define BFn_CLR(reg, blk, field)       HW_##reg##_CLR(blk, BM_##reg##_##field)
#define BFn_TOG(reg, blk, field)       HW_##reg##_TOG(blk, BM_##reg##_##field)

#define BFn_SETV(reg, blk, field, v)   HW_##reg##_SET(blk, BF_##reg##_##field(v))
#define BFn_CLRV(reg, blk, field, v)   HW_##reg##_CLR(blk, BF_##reg##_##field(v))
#define BFn_TOGV(reg, blk, field, v)   HW_##reg##_TOG(blk, BF_##reg##_##field(v))

#define BVn_FLD(reg, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BVn_VAL(reg, field, sym)  BV_##reg##_##field##__##sym

#define BFn_RD(reg, blk, field)        HW_##reg(blk).B.field
#define BFn_WR(reg, blk, field, v)     BW_##reg##_##field(blk, v)

#define BFn_CS1(reg, blk, f1, v1)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1)))

#define BFn_CS2(reg, blk, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2)))

#define BFn_CS3(reg, blk, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3)))

#define BFn_CS4(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4)))

#define BFn_CS5(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5)))

#define BFn_CS6(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6)))

#define BFn_CS7(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7)))

#define BFn_CS8(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7 |      \
                        BM_##reg##_##f8),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7) |  \
                        BF_##reg##_##f8(v8)))

//
// macros for MULTI-BLOCK multiple instance registers
//

#define BFn_SETn(reg, blk, n, field)       HW_##reg##_SET(blk, n, BM_##reg##_##field)
#define BFn_CLRn(reg, blk, n, field)       HW_##reg##_CLR(blk, n, BM_##reg##_##field)
#define BFn_TOGn(reg, blk, n, field)       HW_##reg##_TOG(blk, n, BM_##reg##_##field)

#define BFn_SETVn(reg, blk, n, field, v)   HW_##reg##_SET(blk, n, BF_##reg##_##field(v))
#define BFn_CLRVn(reg, blk, n, field, v)   HW_##reg##_CLR(blk, n, BF_##reg##_##field(v))
#define BFn_TOGVn(reg, blk, n, field, v)   HW_##reg##_TOG(blk, n, BF_##reg##_##field(v))

#define BVn_FLDn(reg, blk, n, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BVn_VALn(reg, blk, n, field, sym)  BV_##reg##_##field##__##sym

#define BFn_RDn(reg, blk, n, field)        HW_##reg(n).B.field
#define BFn_WRn(reg, blk, n, field, v)     BW_##reg##_##field(n, v)

#define BFn_CS1n(reg, blk, n, f1, v1)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1))))

#define BFn_CS2n(reg, blk, n, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2))))

#define BFn_CS3n(reg, blk, n, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3))))

#define BFn_CS4n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4))))

#define BFn_CS5n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5))))

#define BFn_CS6n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6))))

#define BFn_CS7n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7))))

#define BFn_CS8n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7 |       \
                            BM_##reg##_##f8)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7) |   \
                            BF_##reg##_##f8(v8))))

#endif // _REGS_H

////////////////////////////////////////////////////////////////////////////////
