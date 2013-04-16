/*
 * SEC Descriptor Construction Library
 * Basic job descriptor construction
 *
 * Copyright (c) 2012 Freescale Semiconductor, Inc.
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
#include <desc_constr.h>
#include <jobdesc.h>

#define KEY_BLOB_SIZE			32
#define MAC_SIZE			16

void inline_cnstr_jobdesc_blob_encap(uint32_t *desc, uint8_t *key_idnfr,
				     uint8_t *plain_txt, uint8_t *enc_blob,
				     uint32_t in_sz)
{

	dma_addr_t dma_addr_key_idnfr, dma_addr_in, dma_addr_out;
	uint32_t key_sz = KEY_IDNFR_SZ_BYTES;
	/* output blob will have 32 bytes key blob in beginning and
	 * 16 byte HMAC identifier at end of data blob */
	uint32_t out_sz = in_sz + KEY_BLOB_SIZE + MAC_SIZE;

	dma_addr_key_idnfr = virt_to_phys((void *)key_idnfr);
	dma_addr_in	= virt_to_phys((void *)plain_txt);
	dma_addr_out	= virt_to_phys((void *)enc_blob);

	init_job_desc(desc, 0);
	append_key(desc, dma_addr_key_idnfr, key_sz, CLASS_2);

	if (in_sz > 0xffff) {
		append_seq_in_ptr(desc, dma_addr_in, 0, SQIN_EXT);
		append_cmd(desc, in_sz);
	} else {
		append_seq_in_ptr(desc, dma_addr_in, in_sz, 0);
	}

	if (out_sz > 0xffff) {
		append_seq_out_ptr(desc, dma_addr_out, 0, SQOUT_EXT);
		append_cmd(desc, out_sz);
	} else {
		append_seq_out_ptr(desc, dma_addr_out, out_sz, 0);
	}
	append_operation(desc, OP_TYPE_ENCAP_PROTOCOL | OP_PCLID_BLOB);
}

void inline_cnstr_jobdesc_blob_decap(uint32_t *desc, uint8_t *key_idnfr,
				     uint8_t *enc_blob, uint8_t *plain_txt,
				     uint32_t out_sz)
{
	dma_addr_t dma_addr_key_idnfr, dma_addr_in, dma_addr_out;
	uint32_t key_sz = KEY_IDNFR_SZ_BYTES;
	uint32_t in_sz = out_sz + KEY_BLOB_SIZE + MAC_SIZE;

	dma_addr_key_idnfr = virt_to_phys((void *)key_idnfr);
	dma_addr_in	= virt_to_phys((void *)enc_blob);
	dma_addr_out	= virt_to_phys((void *)plain_txt);

	init_job_desc(desc, 0);
	append_key(desc, dma_addr_key_idnfr, key_sz, CLASS_2);
	if (in_sz > 0xffff) {
		append_seq_in_ptr(desc, dma_addr_in, 0, SQIN_EXT);
		append_cmd(desc, in_sz);
	} else {
		append_seq_in_ptr(desc, dma_addr_in, in_sz, 0);
	}

	if (out_sz > 0xffff) {
		append_seq_out_ptr(desc, dma_addr_out, 0, SQOUT_EXT);
		append_cmd(desc, out_sz);
	} else {
		append_seq_out_ptr(desc, dma_addr_out, out_sz, 0);
	}

	append_operation(desc, OP_TYPE_DECAP_PROTOCOL | OP_PCLID_BLOB);
}


/* Change key size to bytes form bits in calling function*/
void inline_cnstr_jobdesc_pkha_rsaexp(uint32_t *desc,
				      struct pk_in_params *pkin, uint8_t *out,
				      uint32_t out_siz)
{
	dma_addr_t dma_addr_e, dma_addr_a, dma_addr_n, dma_addr_out;

	dma_addr_e = virt_to_phys((void *)pkin->e);
	dma_addr_a = virt_to_phys((void *)pkin->a);
	dma_addr_n = virt_to_phys((void *)pkin->n);
	dma_addr_out = virt_to_phys((void *)out);

	init_job_desc(desc, 0);
	append_key(desc, dma_addr_e, pkin->e_siz, KEY_DEST_PKHA_E | CLASS_1);

	append_fifo_load(desc, dma_addr_a,
			 pkin->a_siz, LDST_CLASS_1_CCB | FIFOLD_TYPE_PK_A);

	append_fifo_load(desc, dma_addr_n,
			 pkin->n_siz, LDST_CLASS_1_CCB | FIFOLD_TYPE_PK_N);

	append_operation(desc, OP_TYPE_PK | OP_ALG_PK | OP_ALG_PKMODE_MOD_EXPO);

	append_fifo_store(desc, dma_addr_out, out_siz,
			  LDST_CLASS_1_CCB | FIFOST_TYPE_PKHA_B);
}

void inline_cnstr_jobdesc_sha256(uint32_t *desc,
				 uint8_t *msg, uint32_t msgsz,
				 uint8_t *digest)
{
	/* SHA 256 , output is of length 32 words */
	uint32_t storelen = 32;
	dma_addr_t dma_addr_in, dma_addr_out;

	dma_addr_in = virt_to_phys((void *)msg);
	dma_addr_out = virt_to_phys((void *)digest);

	init_job_desc(desc, 0);
	append_operation(desc,
			 OP_TYPE_CLASS2_ALG | OP_ALG_ALGSEL_SHA256 |
			 OP_ALG_AAI_HASH | OP_ALG_AS_INITFINAL | OP_ALG_ENCRYPT
			 | OP_ALG_ICV_OFF);
	if (msgsz > 0xffff) {
		append_fifo_load(desc, dma_addr_in, 0,
				 LDST_CLASS_2_CCB | FIFOLDST_SGF |
				 FIFOLD_TYPE_MSG | FIFOLD_TYPE_LAST2 |
				 FIFOLDST_EXT);
		append_cmd(desc, msgsz);
	} else
		append_fifo_load(desc, dma_addr_in, msgsz,
				 LDST_CLASS_2_CCB | FIFOLDST_SGF |
				 FIFOLD_TYPE_MSG | FIFOLD_TYPE_LAST2);
	append_store(desc, dma_addr_out, storelen,
		     LDST_CLASS_2_CCB | LDST_SRCDST_BYTE_CONTEXT);
}
