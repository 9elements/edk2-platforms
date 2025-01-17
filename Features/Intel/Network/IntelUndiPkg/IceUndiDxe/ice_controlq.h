/**************************************************************************

Copyright (c) 2016 - 2021, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _ICE_CONTROLQ_H_
#define _ICE_CONTROLQ_H_

#include "ice_adminq_cmd.h"


/* Maximum buffer lengths for all control queue types */
#define ICE_AQ_MAX_BUF_LEN 4096
#ifndef NO_MBXQ_SUPPORT
#define ICE_MBXQ_MAX_BUF_LEN 4096
#endif /* !NO_MBXQ_SUPPORT */
#ifndef NO_SBQ_SUPPORT
#define ICE_SBQ_MAX_BUF_LEN 512
#endif /* !NO_SBQ_SUPPORT */

#define ICE_CTL_Q_DESC(R, i) \
	(&(((struct ice_aq_desc *)((R).desc_buf.va))[i]))

#define ICE_CTL_Q_DESC_UNUSED(R) \
	((u16)((((R)->next_to_clean > (R)->next_to_use) ? 0 : (R)->count) + \
	       (R)->next_to_clean - (R)->next_to_use - 1))

/* Defines that help manage the driver vs FW API checks.
#ifdef FPGA_SUPPORT
 * Current defines help differentiate between FPGA versions.
#endif
 * Take a look at ice_aq_ver_check in ice_controlq.c for actual usage.
 */
#ifdef SIMICS_SUPPORT
#define EXP_FW_API_VER_BRANCH		0x00
#define EXP_FW_API_VER_MAJOR		0x01
#define EXP_FW_API_VER_MINOR		0x07
#else
#define EXP_FW_API_VER_BRANCH		0x00
#define EXP_FW_API_VER_MAJOR		0x01
#define EXP_FW_API_VER_MINOR		0x05
#endif

/* Different control queue types: These are mainly for SW consumption. */
enum ice_ctl_q {
	ICE_CTL_Q_UNKNOWN = 0,
	ICE_CTL_Q_ADMIN,
#ifndef NO_MBXQ_SUPPORT
	ICE_CTL_Q_MAILBOX,
#endif /* !NO_MBXQ_SUPPORT */
#ifndef NO_SBQ_SUPPORT
	ICE_CTL_Q_SB,
#endif /* !NO_SBQ_SUPPORT */
#ifdef QV_SUPPORT
	ICE_CTL_Q_CPK_FW,
	ICE_CTL_Q_CPK_SB,
	ICE_CTL_Q_DSC_FW,
	ICE_CTL_Q_HLP_FW,
	ICE_CTL_Q_HLP_SB,
	ICE_CTL_Q_IPSEC_SB,
#endif /* QV_SUPPORT */
};

/* Control Queue timeout settings - max delay 1s */
#define ICE_CTL_Q_SQ_CMD_TIMEOUT	10000 /* Count 10000 times */
#ifdef PLATF_SUPPORT
#define ICE_CTL_Q_SQ_CMD_TIMEOUT_HAPS	100000
#define ICE_CTL_Q_SQ_CMD_TIMEOUT_BY_PLATF(hw) ((hw)->platf_type == \
			ICE_CNV_HAPS ? ICE_CTL_Q_SQ_CMD_TIMEOUT_HAPS : \
			ICE_CTL_Q_SQ_CMD_TIMEOUT)
#endif /* PLATF_SUPPORT */
#define ICE_CTL_Q_SQ_CMD_USEC		100   /* Check every 100usec */
#define ICE_CTL_Q_ADMIN_INIT_TIMEOUT	10    /* Count 10 times */
#define ICE_CTL_Q_ADMIN_INIT_MSEC	100   /* Check every 100msec */

struct ice_ctl_q_ring {
	void *dma_head;			/* Virtual address to DMA head */
	struct ice_dma_mem desc_buf;	/* descriptor ring memory */
	void *cmd_buf;			/* command buffer memory */

	union {
		struct ice_dma_mem *sq_bi;
		struct ice_dma_mem *rq_bi;
	} r;

	u16 count;		/* Number of descriptors */

	/* used for interrupt processing */
	u16 next_to_use;
	u16 next_to_clean;

	/* used for queue tracking */
	u32 head;
	u32 tail;
	u32 len;
	u32 bah;
	u32 bal;
	u32 len_mask;
	u32 len_ena_mask;
	u32 len_crit_mask;
	u32 head_mask;
};

/* sq transaction details */
struct ice_sq_cd {
#if defined(QV_SUPPORT)
	void *callback; /* cast from type ICE_CTL_Q_CALLBACK */
	u64 cookie;
	u16 flags_ena;
	u16 flags_dis;
	u8 async;
	u8 postpone;
#endif /* !EXTERNAL_RELEASE || QV_SUPPORT */
	struct ice_aq_desc *wb_desc;
};

#define ICE_CTL_Q_DETAILS(R, i) (&(((struct ice_sq_cd *)((R).cmd_buf))[i]))

/* rq event information */
struct ice_rq_event_info {
	struct ice_aq_desc desc;
	u16 msg_len;
	u16 buf_len;
	u8 *msg_buf;
};

/* Control Queue information */
struct ice_ctl_q_info {
	enum ice_ctl_q qtype;
	struct ice_ctl_q_ring rq;	/* receive queue */
	struct ice_ctl_q_ring sq;	/* send queue */
	u32 sq_cmd_timeout;		/* send queue cmd write back timeout */
	u16 num_rq_entries;		/* receive queue depth */
	u16 num_sq_entries;		/* send queue depth */
	u16 rq_buf_size;		/* receive queue buffer size */
	u16 sq_buf_size;		/* send queue buffer size */
	enum ice_aq_err sq_last_status;	/* last status on send queue */
	struct ice_lock sq_lock;		/* Send queue lock */
	struct ice_lock rq_lock;		/* Receive queue lock */
};

#endif /* _ICE_CONTROLQ_H_ */
