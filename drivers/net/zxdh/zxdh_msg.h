/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2024 ZTE Corporation
 */

#ifndef ZXDH_MSG_H
#define ZXDH_MSG_H

#include <stdint.h>

#include <ethdev_driver.h>

#include "zxdh_ethdev_ops.h"

#define ZXDH_BAR0_INDEX                 0
#define ZXDH_CTRLCH_OFFSET              (0x2000)
#define ZXDH_MSG_CHAN_PFVFSHARE_OFFSET  (ZXDH_CTRLCH_OFFSET + 0x1000)

#define ZXDH_MAC_OFFSET                 (0x24000)
#define ZXDH_MAC_STATS_OFFSET           (0x1408)
#define ZXDH_MAC_BYTES_OFFSET           (0xb000)

#define ZXDH_MSIX_INTR_MSG_VEC_BASE   1
#define ZXDH_MSIX_INTR_MSG_VEC_NUM    3
#define ZXDH_MSIX_INTR_DTB_VEC        (ZXDH_MSIX_INTR_MSG_VEC_BASE + ZXDH_MSIX_INTR_MSG_VEC_NUM)
#define ZXDH_MSIX_INTR_DTB_VEC_NUM    1
#define ZXDH_INTR_NONQUE_NUM          (ZXDH_MSIX_INTR_MSG_VEC_NUM + ZXDH_MSIX_INTR_DTB_VEC_NUM + 1)
#define ZXDH_QUEUE_INTR_VEC_BASE      (ZXDH_MSIX_INTR_DTB_VEC + ZXDH_MSIX_INTR_DTB_VEC_NUM)
#define ZXDH_QUEUE_INTR_VEC_NUM       256

#define ZXDH_BAR_MSG_POLLING_SPAN     100
#define ZXDH_BAR_MSG_POLL_CNT_PER_MS  (1 * 1000 / ZXDH_BAR_MSG_POLLING_SPAN)
#define ZXDH_BAR_MSG_POLL_CNT_PER_S   (1 * 1000 * 1000 / ZXDH_BAR_MSG_POLLING_SPAN)
#define ZXDH_BAR_MSG_TIMEOUT_TH       (10 * 1000 * 1000 / ZXDH_BAR_MSG_POLLING_SPAN)

#define ZXDH_BAR_CHAN_MSG_SYNC         0

#define ZXDH_BAR_MSG_ADDR_CHAN_INTERVAL  (2 * 1024) /* channel size */
#define ZXDH_BAR_MSG_PLAYLOAD_OFFSET     (sizeof(struct zxdh_bar_msg_header))
#define ZXDH_BAR_MSG_PAYLOAD_MAX_LEN     \
	(ZXDH_BAR_MSG_ADDR_CHAN_INTERVAL - sizeof(struct zxdh_bar_msg_header))

#define ZXDH_MSG_ADDR_CHAN_INTERVAL       (2 * 1024) /* channel size */
#define ZXDH_MSG_PAYLOAD_MAX_LEN \
		(ZXDH_MSG_ADDR_CHAN_INTERVAL - sizeof(struct zxdh_bar_msg_header))

#define ZXDH_MSG_REPLYBODY_HEAD    sizeof(enum zxdh_reps_flag)
#define ZXDH_MSG_HEADER_SIZE       4
#define ZXDH_MSG_REPLY_BODY_MAX_LEN  \
		(ZXDH_MSG_PAYLOAD_MAX_LEN - sizeof(struct zxdh_msg_reply_head))

#define ZXDH_MSG_HEAD_LEN 8
#define ZXDH_MSG_REQ_BODY_MAX_LEN  \
		(ZXDH_MSG_PAYLOAD_MAX_LEN - ZXDH_MSG_HEAD_LEN)

#define ZXDH_MAC_FILTER            0xaa
#define ZXDH_MAC_UNFILTER          0xff
#define ZXDH_PROMISC_MODE          1
#define ZXDH_ALLMULTI_MODE         2
#define ZXDH_VLAN_STRIP_MSG_TYPE   0
#define ZXDH_QINQ_STRIP_MSG_TYPE   1

enum ZXDH_DRIVER_TYPE {
	ZXDH_MSG_CHAN_END_MPF = 0,
	ZXDH_MSG_CHAN_END_PF,
	ZXDH_MSG_CHAN_END_VF,
	ZXDH_MSG_CHAN_END_RISC,
};

enum ZXDH_MSG_VEC {
	ZXDH_MSIX_FROM_PFVF = ZXDH_MSIX_INTR_MSG_VEC_BASE,
	ZXDH_MSIX_FROM_MPF,
	ZXDH_MSIX_FROM_RISCV,
	ZXDH_MSG_VEC_NUM,
};

enum ZXDH_BAR_MSG_RTN {
	ZXDH_BAR_MSG_OK = 0,
	ZXDH_BAR_MSG_ERR_MSGID,
	ZXDH_BAR_MSG_ERR_NULL,
	ZXDH_BAR_MSG_ERR_TYPE, /* Message type exception */
	ZXDH_BAR_MSG_ERR_MODULE, /* Module ID exception */
	ZXDH_BAR_MSG_ERR_BODY_NULL, /* Message body exception */
	ZXDH_BAR_MSG_ERR_LEN, /* Message length exception */
	ZXDH_BAR_MSG_ERR_TIME_OUT, /* Message sending length too long */
	ZXDH_BAR_MSG_ERR_NOT_READY, /* Abnormal message sending conditions*/
	ZXDH_BAR_MEG_ERR_NULL_FUNC, /* Empty receive processing function pointer*/
	ZXDH_BAR_MSG_ERR_REPEAT_REGISTER, /* Module duplicate registration*/
	ZXDH_BAR_MSG_ERR_UNGISTER, /* Repeated deregistration*/
	/*
	 * The sending interface parameter boundary structure pointer is empty
	 */
	ZXDH_BAR_MSG_ERR_NULL_PARA,
	ZXDH_BAR_MSG_ERR_REPSBUFF_LEN, /* The length of reps_buff is too short*/
	/*
	 * Unable to find the corresponding message processing function for this module
	 */
	ZXDH_BAR_MSG_ERR_MODULE_NOEXIST,
	/*
	 * The virtual address in the parameters passed in by the sending interface is empty
	 */
	ZXDH_BAR_MSG_ERR_VIRTADDR_NULL,
	ZXDH_BAR_MSG_ERR_REPLY, /* sync msg resp_error */
	ZXDH_BAR_MSG_ERR_MPF_NOT_SCANNED,
	ZXDH_BAR_MSG_ERR_KERNEL_READY,
	ZXDH_BAR_MSG_ERR_USR_RET_ERR,
	ZXDH_BAR_MSG_ERR_ERR_PCIEID,
	ZXDH_BAR_MSG_ERR_SOCKET, /* netlink sockte err */
};

enum ZXDH_BAR_MODULE_ID {
	ZXDH_BAR_MODULE_DBG = 0, /* 0:  debug */
	ZXDH_BAR_MODULE_TBL,     /* 1:  resource table */
	ZXDH_BAR_MODULE_MISX,    /* 2:  config msix */
	ZXDH_BAR_MODULE_SDA,     /* 3: */
	ZXDH_BAR_MODULE_RDMA,    /* 4: */
	ZXDH_BAR_MODULE_DEMO,    /* 5:  channel test */
	ZXDH_BAR_MODULE_SMMU,    /* 6: */
	ZXDH_BAR_MODULE_MAC,     /* 7:  mac rx/tx stats */
	ZXDH_BAR_MODULE_VDPA,    /* 8:  vdpa live migration */
	ZXDH_BAR_MODULE_VQM,     /* 9:  vqm live migration */
	ZXDH_BAR_MODULE_NP,      /* 10: vf msg callback np */
	ZXDH_BAR_MODULE_VPORT,   /* 11: get vport */
	ZXDH_BAR_MODULE_BDF,     /* 12: get bdf */
	ZXDH_BAR_MODULE_RISC_READY, /* 13: */
	ZXDH_BAR_MODULE_REVERSE,    /* 14: byte stream reverse */
	ZXDH_BAR_MDOULE_NVME,       /* 15: */
	ZXDH_BAR_MDOULE_NPSDK,      /* 16: */
	ZXDH_BAR_MODULE_NP_TODO,    /* 17: */
	ZXDH_MODULE_BAR_MSG_TO_PF,  /* 18: */
	ZXDH_MODULE_BAR_MSG_TO_VF,  /* 19: */

	ZXDH_MODULE_FLASH = 32,
	ZXDH_BAR_MODULE_OFFSET_GET = 33,
	ZXDH_BAR_EVENT_OVS_WITH_VCB = 36,

	ZXDH_BAR_MSG_MODULE_NUM = 100,
};

enum ZXDH_RES_TBL_FILED {
	ZXDH_TBL_FIELD_PCIEID     = 0,
	ZXDH_TBL_FIELD_BDF        = 1,
	ZXDH_TBL_FIELD_MSGCH      = 2,
	ZXDH_TBL_FIELD_DATACH     = 3,
	ZXDH_TBL_FIELD_VPORT      = 4,
	ZXDH_TBL_FIELD_PNLID      = 5,
	ZXDH_TBL_FIELD_PHYPORT    = 6,
	ZXDH_TBL_FIELD_SERDES_NUM = 7,
	ZXDH_TBL_FIELD_NP_PORT    = 8,
	ZXDH_TBL_FIELD_SPEED      = 9,
	ZXDH_TBL_FIELD_HASHID     = 10,
	ZXDH_TBL_FIELD_NON,
};

enum ZXDH_TBL_MSG_TYPE {
	ZXDH_TBL_TYPE_READ,
	ZXDH_TBL_TYPE_WRITE,
	ZXDH_TBL_TYPE_NON,
};

enum pciebar_layout_type {
	ZXDH_URI_VQM      = 0,
	ZXDH_URI_SPINLOCK = 1,
	ZXDH_URI_FWCAP    = 2,
	ZXDH_URI_FWSHR    = 3,
	ZXDH_URI_DRS_SEC  = 4,
	ZXDH_URI_RSV      = 5,
	ZXDH_URI_CTRLCH   = 6,
	ZXDH_URI_1588     = 7,
	ZXDH_URI_QBV      = 8,
	ZXDH_URI_MACPCS   = 9,
	ZXDH_URI_RDMA     = 10,
	ZXDH_URI_MNP      = 11,
	ZXDH_URI_MSPM     = 12,
	ZXDH_URI_MVQM     = 13,
	ZXDH_URI_MDPI     = 14,
	ZXDH_URI_NP       = 15,
	ZXDH_URI_MAX,
};

/* riscv msg opcodes */
enum zxdh_agent_msg_type {
	ZXDH_MAC_STATS_GET = 10,
	ZXDH_MAC_STATS_RESET,
	ZXDH_MAC_LINK_GET = 14,
	ZXDH_VQM_DEV_STATS_GET = 21,
	ZXDH_VQM_DEV_STATS_RESET,
	ZXDH_VQM_QUEUE_STATS_GET = 24,
	ZXDH_VQM_QUEUE_STATS_RESET,
};

enum zxdh_msg_type {
	ZXDH_NULL = 0,
	ZXDH_VF_PORT_INIT = 1,
	ZXDH_VF_PORT_UNINIT = 2,
	ZXDH_MAC_ADD = 3,
	ZXDH_MAC_DEL = 4,
	ZXDH_RSS_ENABLE = 7,
	ZXDH_RSS_RETA_SET = 8,
	ZXDH_RSS_RETA_GET = 9,
	ZXDH_RSS_HF_SET = 15,
	ZXDH_RSS_HF_GET = 16,
	ZXDH_VLAN_FILTER_SET = 17,
	ZXDH_VLAN_FILTER_ADD = 18,
	ZXDH_VLAN_FILTER_DEL = 19,
	ZXDH_VLAN_OFFLOAD = 21,

	ZXDH_PORT_ATTRS_SET = 25,
	ZXDH_PORT_PROMISC_SET = 26,

	ZXDH_GET_NP_STATS = 31,

	ZXDH_MSG_TYPE_END,
};

struct zxdh_msix_para {
	uint16_t pcie_id;
	uint16_t vector_risc;
	uint16_t vector_pfvf;
	uint16_t vector_mpf;
	uint64_t virt_addr;
	uint16_t driver_type; /* refer to DRIVER_TYPE */
};

struct zxdh_msix_msg {
	uint16_t pcie_id;
	uint16_t vector_risc;
	uint16_t vector_pfvf;
	uint16_t vector_mpf;
};

struct zxdh_pci_bar_msg {
	uint64_t virt_addr; /* bar addr */
	void    *payload_addr;
	uint16_t payload_len;
	uint16_t emec;
	uint16_t src; /* refer to BAR_DRIVER_TYPE */
	uint16_t dst; /* refer to BAR_DRIVER_TYPE */
	uint16_t module_id;
	uint16_t src_pcieid;
	uint16_t dst_pcieid;
	uint16_t usr;
};

struct __rte_packed_begin zxdh_bar_msix_reps {
	uint16_t pcie_id;
	uint16_t check;
	uint16_t vport;
	uint16_t rsv;
} __rte_packed_end;

struct __rte_packed_begin zxdh_bar_offset_reps {
	uint16_t check;
	uint16_t rsv;
	uint32_t offset;
	uint32_t length;
} __rte_packed_end;

struct __rte_packed_begin zxdh_bar_recv_msg {
	uint8_t reps_ok;
	uint16_t reps_len;
	uint8_t rsv;
	union __rte_packed_begin {
		struct zxdh_bar_msix_reps msix_reps;
		struct zxdh_bar_offset_reps offset_reps;
	} __rte_packed_end;
} __rte_packed_end;

struct zxdh_msg_recviver_mem {
	void *recv_buffer; /* first 4B is head, followed by payload */
	uint64_t buffer_len;
};

struct zxdh_bar_msg_header {
	uint8_t valid : 1; /* used by __bar_chan_msg_valid_set/get */
	uint8_t sync  : 1;
	uint8_t emec  : 1; /* emergency */
	uint8_t ack   : 1; /* ack msg */
	uint8_t poll  : 1;
	uint8_t usr   : 1;
	uint8_t rsv;
	uint16_t module_id;
	uint16_t len;
	uint16_t msg_id;
	uint16_t src_pcieid;
	uint16_t dst_pcieid; /* used in PF-->VF */
};

struct zxdh_bar_offset_params {
	uint64_t virt_addr;  /* Bar space control space virtual address */
	uint16_t pcie_id;
	uint16_t type;  /* Module types corresponding to PCIBAR planning */
};

struct zxdh_bar_offset_res {
	uint32_t bar_offset;
	uint32_t bar_length;
};

struct zxdh_offset_get_msg {
	uint16_t pcie_id;
	uint16_t type;
};

struct __rte_packed_begin zxdh_msg_reply_head {
	uint8_t flag;
	uint16_t reps_len;
	uint8_t resvd;
} __rte_packed_end;

enum zxdh_reps_flag {
	ZXDH_REPS_FAIL,
	ZXDH_REPS_SUCC = 0xaa,
};

struct __rte_packed_begin zxdh_link_info_msg {
	uint8_t autoneg;
	uint8_t link_state;
	uint8_t blink_enable;
	uint8_t duplex;
	uint32_t speed_modes;
	uint32_t speed;
} __rte_packed_end;

struct zxdh_rss_reta {
	uint32_t reta[RTE_ETH_RSS_RETA_SIZE_256];
};

struct zxdh_rss_hf {
	uint32_t rss_hf;
};

struct __rte_packed_begin zxdh_msg_reply_body {
	enum zxdh_reps_flag flag;
	union __rte_packed_begin {
		uint8_t reply_data[ZXDH_MSG_REPLY_BODY_MAX_LEN - sizeof(enum zxdh_reps_flag)];
		struct zxdh_hw_np_stats np_stats;
		struct zxdh_link_info_msg link_msg;
		struct zxdh_rss_hf rss_hf;
		struct zxdh_rss_reta rss_reta;
		struct zxdh_hw_vqm_stats vqm_stats;
	} __rte_packed_end;
} __rte_packed_end;

struct __rte_packed_begin zxdh_msg_reply_info {
	struct zxdh_msg_reply_head reply_head;
	struct zxdh_msg_reply_body reply_body;
} __rte_packed_end;

struct __rte_packed_begin zxdh_vf_init_msg {
	uint8_t link_up;
	uint8_t rsv;
	uint16_t base_qid;
	uint8_t rss_enable;
} __rte_packed_end;

struct __rte_packed_begin zxdh_msg_head {
	enum zxdh_msg_type msg_type;
	uint16_t  vport;
	uint16_t  vf_id;
	uint16_t pcieid;
} __rte_packed_end;

struct __rte_packed_begin zxdh_port_attr_set_msg {
	uint32_t mode;
	uint32_t value;
	uint8_t allmulti_follow;
} __rte_packed_end;

struct __rte_packed_begin zxdh_mac_filter {
	uint8_t mac_flag;
	uint8_t filter_flag;
	struct rte_ether_addr mac;
} __rte_packed_end;

struct __rte_packed_begin zxdh_port_promisc_msg {
	uint8_t mode;
	uint8_t value;
	uint8_t mc_follow;
} __rte_packed_end;

struct zxdh_vlan_filter {
	uint16_t vlan_id;
};

struct zxdh_vlan_filter_set {
	uint8_t enable;
};

struct __rte_packed_begin zxdh_vlan_offload {
	uint8_t enable;
	uint8_t type;
} __rte_packed_end;

struct zxdh_rss_enable {
	uint8_t enable;
};

struct __rte_packed_begin zxdh_agent_msg_head {
	enum zxdh_agent_msg_type msg_type;
	uint8_t panel_id;
	uint8_t phyport;
	uint8_t rsv;
	uint16_t vf_id;
	uint16_t pcie_id;
} __rte_packed_end;

struct __rte_packed_begin zxdh_msg_info {
	union {
		uint8_t head_len[ZXDH_MSG_HEAD_LEN];
		struct zxdh_msg_head msg_head;
		struct zxdh_agent_msg_head agent_msg_head;
	};
	union __rte_packed_begin {
		uint8_t datainfo[ZXDH_MSG_REQ_BODY_MAX_LEN];
		struct zxdh_vf_init_msg vf_init_msg;
		struct zxdh_port_attr_set_msg port_attr_msg;
		struct zxdh_link_info_msg link_msg;
		struct zxdh_mac_filter mac_filter_msg;
		struct zxdh_port_promisc_msg port_promisc_msg;
		struct zxdh_vlan_filter vlan_filter_msg;
		struct zxdh_vlan_filter_set vlan_filter_set_msg;
		struct zxdh_vlan_offload vlan_offload_msg;
		struct zxdh_rss_reta rss_reta;
		struct zxdh_rss_enable rss_enable;
		struct zxdh_rss_hf rss_hf;
	} __rte_packed_end data;
} __rte_packed_end;

typedef int (*zxdh_bar_chan_msg_recv_callback)(void *pay_load, uint16_t len,
		void *reps_buffer, uint16_t *reps_len, void *dev);

int zxdh_get_bar_offset(struct zxdh_bar_offset_params *paras, struct zxdh_bar_offset_res *res);
int zxdh_msg_chan_init(void);
int zxdh_bar_msg_chan_exit(void);
int zxdh_msg_chan_hwlock_init(struct rte_eth_dev *dev);

int zxdh_msg_chan_enable(struct rte_eth_dev *dev);
int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in,
		struct zxdh_msg_recviver_mem *result);

int zxdh_bar_irq_recv(uint8_t src, uint8_t dst, uint64_t virt_addr, void *dev);
void zxdh_msg_head_build(struct zxdh_hw *hw, enum zxdh_msg_type type,
		struct zxdh_msg_info *msg_info);
int zxdh_vf_send_msg_to_pf(struct rte_eth_dev *dev,  void *msg_req,
			uint16_t msg_req_len, void *reply, uint16_t reply_len);
void zxdh_agent_msg_build(struct zxdh_hw *hw, enum zxdh_agent_msg_type type,
		struct zxdh_msg_info *msg_info);
int32_t zxdh_send_msg_to_riscv(struct rte_eth_dev *dev, void *msg_req,
			uint16_t msg_req_len, void *reply, uint16_t reply_len,
			enum ZXDH_BAR_MODULE_ID module_id);

#endif /* ZXDH_MSG_H */
