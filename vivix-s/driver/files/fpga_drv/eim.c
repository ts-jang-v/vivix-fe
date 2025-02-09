/*
 * Copyright (c) 2011-2012, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include "eim.h"

static eim_cfg_t cfg_table[] = {
/*	ID						BOOL	SHF	   REGISTER		MASK 		*/
    {GCR1_CSEN, true, 0, EIM_GCR1, BM_EIM_CS0GCR1_CSEN},
    {GCR1_SWR, true, 1, EIM_GCR1, BM_EIM_CS0GCR1_SWR},
    {GCR1_SDR, true, 2, EIM_GCR1, BM_EIM_CS0GCR1_SRD},
    {GCR1_MUM, true, 3, EIM_GCR1, BM_EIM_CS0GCR1_MUM},
    {GCR1_WFL, true, 4, EIM_GCR1, BM_EIM_CS0GCR1_WFL},
    {GCR1_RFL, true, 5, EIM_GCR1, BM_EIM_CS0GCR1_RFL},
    {GCR1_CRE, true, 6, EIM_GCR1, BM_EIM_CS0GCR1_CRE},
    {GCR1_CREP, true, 7, EIM_GCR1, BM_EIM_CS0GCR1_CREP},
    {GCR1_BL, false, 8, EIM_GCR1, BM_EIM_CS0GCR1_BL},
    {GCR1_WC, true, 11, EIM_GCR1, BM_EIM_CS0GCR1_WC},
    {GCR1_BCD, false, 12, EIM_GCR1, BM_EIM_CS0GCR1_BCD},
    {GCR1_BCS, false, 14, EIM_GCR1, BM_EIM_CS0GCR1_BCS},
    {GCR1_DSZ, false, 16, EIM_GCR1, BM_EIM_CS0GCR1_DSZ},
    {GCR1_SP, true, 19, EIM_GCR1, BM_EIM_CS0GCR1_SP},
    {GCR1_CSREC, false, 20, EIM_GCR1, BM_EIM_CS0GCR1_CSREC},
    {GCR1_AUS, true, 23, EIM_GCR1, BM_EIM_CS0GCR1_AUS},
    {GCR1_GBC, false, 24, EIM_GCR1, BM_EIM_CS0GCR1_GBC},
    {GCR1_WP, true, 27, EIM_GCR1, BM_EIM_CS0GCR1_WP},
    {GCR1_PSZ, false, 28, EIM_GCR1, BM_EIM_CS0GCR1_PSZ},

    {GCR2_ADH, false, 0, EIM_GCR2, BM_EIM_CS0GCR2_ADH},
    {GCR2_DAPS, false, 4, EIM_GCR2, BM_EIM_CS0GCR2_DAPS},
    {GCR2_DAE, true, 8, EIM_GCR2, BM_EIM_CS0GCR2_DAE},
    {GCR2_DAP, true, 9, EIM_GCR2, BM_EIM_CS0GCR2_DAP},
    {GCR2_MUX16_BYP_GRANT, true, 12, EIM_GCR2, BM_EIM_CS0GCR2_MUX16_BYP_GRANT},

    {RCR1_RCSN, false, 0, EIM_RCR1, BM_EIM_CS0RCR1_RCSN},
    {RCR1_RCSA, false, 4, EIM_RCR1, BM_EIM_CS0RCR1_RCSA},
    {RCR1_OEN, false, 8, EIM_RCR1, BM_EIM_CS0RCR1_OEN},
    {RCR1_OEA, false, 12, EIM_RCR1, BM_EIM_CS0RCR1_OEA},
    {RCR1_RADVN, false, 16, EIM_RCR1, BM_EIM_CS0RCR1_RADVN},
    {RCR1_RAL, true, 19, EIM_RCR1, BM_EIM_CS0RCR1_RAL},
    {RCR1_RADVA, false, 20, EIM_RCR1, BM_EIM_CS0RCR1_RADVA},
    {RCR1_RWSC, false, 24, EIM_RCR1, BM_EIM_CS0RCR1_RWSC},

    {RCR2_RBEN, false, 0, EIM_RCR2, BM_EIM_CS0RCR2_RBEN},
    {RCR2_RBE, true, 3, EIM_RCR2, BM_EIM_CS0RCR2_RBE},
    {RCR2_RBEA, false, 4, EIM_RCR2, BM_EIM_CS0RCR2_RBEA},
    {RCR2_RL, false, 8, EIM_RCR2, BM_EIM_CS0RCR2_RL},
    {RCR2_PAT, false, 12, EIM_RCR2, BM_EIM_CS0RCR2_PAT},
    {RCR2_APR, true, 15, EIM_RCR2, BM_EIM_CS0RCR2_APR},

    {WCR1_WCSN, false, 0, EIM_WCR1, BM_EIM_CS0WCR1_WCSN},
    {WCR1_WCSA, false, 3, EIM_WCR1, BM_EIM_CS0WCR1_WCSA},
    {WCR1_WEN, false, 6, EIM_WCR1, BM_EIM_CS0WCR1_WEN},
    {WCR1_WEA, false, 9, EIM_WCR1, BM_EIM_CS0WCR1_WEA},
    {WCR1_WBEN, false, 12, EIM_WCR1, BM_EIM_CS0WCR1_WBEN},
    {WCR1_WBEA, false, 15, EIM_WCR1, BM_EIM_CS0WCR1_WBEA},
    {WCR1_WADVN, false, 18, EIM_WCR1, BM_EIM_CS0WCR1_WADVN},
    {WCR1_WADVA, false, 21, EIM_WCR1, BM_EIM_CS0WCR1_WADVA},
    {WCR1_WWSC, false, 24, EIM_WCR1, BM_EIM_CS0WCR1_WWSC},
    {WCR1_WBED, true, 30, EIM_WCR1, BM_EIM_CS0WCR1_WBED},
    {WCR1_WAL, true, 31, EIM_WCR1, BM_EIM_CS0WCR1_WAL},

    {WCR2_WBCDD, true, 0, EIM_WCR2, BM_EIM_CS0WCR2_WBCDD},

    {WCR_BCM, true, 0, EIM_WCR, BM_EIM_WCR_BCM },
    {WCR_GBCD, false, 1, EIM_WCR, BM_EIM_WCR_GBCD},
    {WCR_CON_BLK_SEL, true, 0, EIM_WCR, BM_EIM_WCR_CONT_BCLK_SEL},
    {WCR_INTEN, true, 4, EIM_WCR, BM_EIM_WCR_INTEN},
    {WCR_INTPOL, true, 5, EIM_WCR, BM_EIM_WCR_INTPOL},
    {WCR_WDOG_EN, true, 8, EIM_WCR, BM_EIM_WCR_WDOG_EN},
    {WCR_WDOG_LIMIT, false, 9, EIM_WCR, BM_EIM_WCR_WDOG_LIMIT},

    {WIAR_IPS_REG, true, 0, EIM_WIAR, BM_EIM_WIAR_IPS_REQ},
    {WIAR_IPS_ACK, true, 1, EIM_WIAR, BM_EIM_WIAR_IPS_ACK},
    {WIAR_INT, true, 2, EIM_WIAR, BM_EIM_WIAR_INT},
    {WIAR_ERRST, true, 3, EIM_WIAR, BM_EIM_WIAR_ERRST},
    {WIAR_ACLK_EN, true, 4, EIM_WIAR, BM_EIM_WIAR_ACLK_EN},

    {NUM_CFG, false, 0, EIM_NUM_REG, 0x00000000},
};

/*!
 * @brief Initialize EIM controller.
 *
 * Only setup the data port size and enable the chip select.
 * 
 * @param cs     Chip select
 * @param dsz    Data port size
 * @param mum    Multiplexed Mode.
 * @param aus    Address unshift
 * 
 * @return true on success, false on fail
 */
uint32_t eim_init(uint32_t cs, uint32_t dsz, uint32_t mum, uint32_t aus)
{
	//!Init clock
	
    if (cs >= EIM_NUM_CS) {
        printk("Invalid chip select.\n");
        return false;
    }

    if ((dsz > REG_CSCFG_MAX_DSZ) || (dsz < REG_CSCFG_MIN_DSZ)) {
        printk("Invalid data port size.\n");
        return false;
    }

    /*! Set data port size */
    HW_EIM_CS_REGS[cs].GCR1.B.DSZ = dsz;

    /*! Set multiplexed mode */
    HW_EIM_CS_REGS[cs].GCR1.B.MUM = mum;

    /*! Set address unshift */
    HW_EIM_CS_REGS[cs].GCR1.B.AUS = aus;

    /*! Enable chip select */
    HW_EIM_CS_REGS[cs].GCR1.B.CSEN = 1;

    /*! Init IOMUX */
   // eim_iomux_config();

    return true;
}
//uint32_t eim_init(uint32_t cs, uint32_t dsz, uint32_t mum, uint32_t aus)
//{
//    /*
//     * Init clock
//     */
//
//    /* Init EIM controller */
//    eim_reg_base = (eim_reg_p) WEIM_BASE_ADDR;
//
//    if (cs >= EIM_NUM_CS) {
//        printk("Invalid chip select.\n");
//       return false;
//    }
//
//    if ((dsz > REG_CSCFG_MAX_DSZ) || (dsz < REG_CSCFG_MIN_DSZ)) {
//        printk("Invalid data port size.\n");
//        return false;
//    }
//
//    /* Set data port size */
//    eim_reg_base->reg_cs[cs].gcr1 &= ~REG_CSCFG_MSK_DSZ;
//    eim_reg_base->reg_cs[cs].gcr1 |= dsz << REG_CSCFG_DSZ_SHIFT;
//
//    /* Set multiplexed mode */
//    if (mum == true) {
//        eim_reg_base->reg_cs[cs].gcr1 |= REG_CSCFG_MSK_MUM;
//    } else {
//        eim_reg_base->reg_cs[cs].gcr1 &= ~REG_CSCFG_MSK_MUM;
//    }
//
//    /* Set address unshift */
//    if (aus == true) {
//        eim_reg_base->reg_cs[cs].gcr1 |= REG_CSCFG_MSK_SHFT;
//    } else {
//        eim_reg_base->reg_cs[cs].gcr1 &= ~REG_CSCFG_MSK_SHFT;
//    }
//
//    /* Enable chip select */
//    eim_reg_base->reg_cs[cs].gcr1 |= REG_CSCFG_MSK_CSEN;
//
//    /* Init IOMUX */
//    eim_iomux_config();
//
//    return true;
//}

/*!
 * @brief Setup EIM configuration, each field per call.
 *
 * @param cs     Chip select
 * @param cfg    CFG ID
 * @param value  CFG value
 *
 * @return true on success, false on fail
 */
uint32_t eim_cfg_set(uint32_t cs, uint32_t cfg, uint32_t value)
{
    uint32_t idx;
    volatile reg32_t *reg;

    /*! Lookup CFG in table */
    for (idx = 0; cfg_table[idx].id != NUM_CFG; idx++) {
        if (cfg == cfg_table[idx].id) {
            break;
        }
    }

    if (cfg_table[idx].id == NUM_CFG) {
        printk("Invalid CFG id.\n");
        return false;
    }

    /*! Validate CS */
    if (cs >= EIM_NUM_CS) {
        printk("Invalid CS value.\n");
        return false;
    }

    /* Obtain register address */
    switch (cfg_table[idx].reg) {
    case EIM_GCR1:
        reg = &(HW_EIM_CS_REGS[cs].GCR1.U);
        break;
    case EIM_GCR2:
        reg = &(HW_EIM_CS_REGS[cs].GCR2.U);
        break;
    case EIM_RCR1:
        reg = &(HW_EIM_CS_REGS[cs].RCR1.U);
        break;
    case EIM_RCR2:
        reg = &(HW_EIM_CS_REGS[cs].RCR2.U);
        break;
    case EIM_WCR1:
        reg = &(HW_EIM_CS_REGS[cs].WCR1.U);
        break;
    case EIM_WCR2:
        reg = &(HW_EIM_CS_REGS[cs].WCR2.U);
        break;
    case EIM_WCR:
        reg = &(HW_EIM_WCR.U);
        break;
    case EIM_WIAR:
        reg = &(HW_EIM_WIAR.U);
        break;
    default:                   /* should never happen */
        return false;
    }

    /* Boolean CFG */
    if (cfg_table[idx].boolean == true) {
        if (value == true) {
            *reg |= cfg_table[idx].mask;
        } else {
            *reg &= ~cfg_table[idx].mask;
        }
    }

    /* Integer CFG */
    else {
        if (value > (cfg_table[idx].mask >> cfg_table[idx].shift)) {
            printk("Invalid CFG value.\n");
            return false;
        }

        *reg = (*reg & (~cfg_table[idx].mask)) | (value << cfg_table[idx].shift);
    }

    return true;
}
 
//uint32_t eim_cfg_set(uint32_t cs, uint32_t cfg, uint32_t value)
//{
//    uint32_t idx;
//    volatile uint32_t *reg;
//
//    /* Lookup CFG in table */
//    for (idx = 0; cfg_table[idx].id != NUM_CFG; idx++) {
//        if (cfg == cfg_table[idx].id) {
//            break;
//        }
//    }
//
//    if (cfg_table[idx].id == NUM_CFG) {
//        printk("Invalid CFG id.\n");
//        return false;
//    }
//
//    /* Validate CS */
//    if (cs >= EIM_NUM_CS) {
//        printk("Invalid CS value.\n");
//        return false;
//    }
//
//    /* Obtain register address */
//    switch (cfg_table[idx].reg) {
//    case EIM_GCR1:
//        reg = &eim_reg_base->reg_cs[cs].gcr1;
//        break;
//    case EIM_GCR2:
//        reg = &eim_reg_base->reg_cs[cs].gcr2;
//        break;
//    case EIM_RCR1:
//        reg = &eim_reg_base->reg_cs[cs].rcr1;
//        break;
//    case EIM_RCR2:
//        reg = &eim_reg_base->reg_cs[cs].rcr2;
//        break;
//    case EIM_WCR1:
//        reg = &eim_reg_base->reg_cs[cs].wcr1;
//        break;
//    case EIM_WCR2:
//        reg = &eim_reg_base->reg_cs[cs].wcr2;
//        break;
//    case EIM_WCR:
//        reg = &eim_reg_base->wcr;
//        break;
//    case EIM_WIAR:
//        reg = &eim_reg_base->wiar;
//        break;
//    default:                   /* should never happen */
//        return false;
//    }
//
//    /* Boolean CFG */
//    if (cfg_table[idx].boolean == true) {
//        if (value == true) {
//            *reg |= cfg_table[idx].mask;
//        } else {
//            *reg &= ~cfg_table[idx].mask;
//        }
//    }
//
//    /* Integer CFG */
//    else {
//        if (value > (cfg_table[idx].mask >> cfg_table[idx].shift)) {
//            printk("Invalid CFG value.\n");
//            return false;
//        }
//
//        *reg = (*reg & (~cfg_table[idx].mask)) | (value << cfg_table[idx].shift);
//    }
//
//    return true;
//}
