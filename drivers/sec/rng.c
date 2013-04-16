/*
 * RNG initialization
 *
 * Derived from linux caam driver ctrl.c
 *
 * Copyright 2013 Freescale Semiconductor, Inc.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <common.h>
#include <malloc.h>
#include <jr.h>
#include <desc_constr.h>
#include <desc.h>

u8 get_rng_vid(void)
{
	ccsr_sec_t *sec = (void *)CONFIG_SYS_FSL_SEC_ADDR;
	u32 cha_vid = in_be32(&sec->chavid_ls);

	return (cha_vid & SEC_CHAVID_RNG_LS_MASK) >> SEC_CHAVID_LS_RNG_SHIFT;
}

/*
 * Descriptor to instantiate RNG State Handle 0 in normal mode and
 * load the JDKEK, TDKEK and TDSK registers
 */
static void build_instantiation_desc(u32 *desc)
{
	u32 *jump_cmd;

	init_job_desc(desc, 0);

	/* INIT RNG in non-test mode */
	append_operation(desc, OP_TYPE_CLASS1_ALG | OP_ALG_ALGSEL_RNG |
			 OP_ALG_AS_INIT);

	/* wait for done */
	jump_cmd = append_jump(desc, JUMP_CLASS_CLASS1);
	set_jump_tgt_here(desc, jump_cmd);

	/*
	 * load 1 to clear written reg:
	 * resets the done interrrupt and returns the RNG to idle.
	 */
	append_load_imm_u32(desc, 1, LDST_SRCDST_WORD_CLRW);

	/* generate secure keys (non-test) */
	append_operation(desc, OP_TYPE_CLASS1_ALG | OP_ALG_ALGSEL_RNG |
			 OP_ALG_RNG4_SK);
}

void rng4_init_done(uint32_t desc, uint32_t status, void *arg)
{
	struct result *instantiation = arg;
	instantiation->status = status;
	instantiation->err = caam_jr_strstatus(instantiation->outstr, status);
	instantiation->done = 1;
}

static int instantiate_rng(struct jobring *jr)
{
	struct result op;
	u32 *desc;
	int ret = 0;
	unsigned long long timeval;
	unsigned long long timeout;

	memset(&op, 0, sizeof(struct result));

	desc = malloc(CAAM_CMD_SZ * 6);
	if (!desc) {
		printf("cannot allocate RNG init descriptor memory\n");
		return -1;
	}

	build_instantiation_desc(desc);
	ret = jr_enqueue(jr, desc, rng4_init_done, &op);
	if (ret)
		return -1;

	timeval = get_ticks();
	timeout = usec2ticks(CONFIG_SEC_DEQ_TIMEOUT);

	while (op.done != 1) {
		if (jr_dequeue(jr)) {
			printf("RNG Instantiation :deq error\n");
			return -1;
		}

		if ((get_ticks() - timeval) > timeout) {
			printf("RNG Instantiation : SEC Dequeue timed out\n");
			return -1;
		}
	}

	if (op.status) {
		printf("RNG: Instantiation failed with error %x\n", op.status);
		return -1;
	}

	return ret;
}

/*
 * By default, the TRNG runs for 200 clocks per sample;
 * 1600 clocks per sample generates better entropy.
 */
static void kick_trng(void)
{
	ccsr_sec_t *sec = (void *)CONFIG_SYS_FSL_SEC_ADDR;
	struct rng4tst __iomem *rng =
			(struct rng4tst __iomem *)&sec->rng;
	u32 val;

	/* put RNG4 into program mode */
	setbits_be32(&rng->rtmctl, RTMCTL_PRGM);
	/* 1600 clocks per sample */
	val = in_be32(&rng->rtsdctl);
	val = (val & ~RTSDCTL_ENT_DLY_MASK) | (1600 << RTSDCTL_ENT_DLY_SHIFT);
	out_be32(&rng->rtsdctl, val);
	/* min. freq. count */
	out_be32(&rng->rtfreqmin, 400);
	/* max. freq. count */
	out_be32(&rng->rtfreqmax, 6400);
	/* put RNG4 into run mode */
	clrbits_be32(&rng->rtmctl, RTMCTL_PRGM);
}

int rng_init(struct jobring *jr)
{
	int ret;
	ccsr_sec_t *sec = (void *)CONFIG_SYS_FSL_SEC_ADDR;
	struct rng4tst __iomem *rng =
			(struct rng4tst __iomem *)&sec->rng;

	u32 rdsta = in_be32(&rng->rdsta);

	/* Check if RNG state 0 handler is already instantiated */
	if (rdsta & RNG_STATE0_HANDLE_INSTANTIATED) {
		printf("RNG: Already instantiated\n");
		return 0;
	}

	kick_trng();

	ret = instantiate_rng(jr);
	if (ret)
		return ret;

	 /* Enable RDB bit so that RNG works faster */
	setbits_be32(&sec->scfgr, SEC_SCFGR_RDBENABLE);

	return ret;
}
