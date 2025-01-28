/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025 Yunsilicon Technology Co., Ltd.
 */

#ifndef _XSC_ETHDEV_H_
#define _XSC_ETHDEV_H_

#include "xsc_dev.h"

struct xsc_ethdev_priv {
	struct rte_eth_dev *eth_dev;
	struct rte_pci_device *pci_dev;
	struct xsc_dev *xdev;
	struct xsc_repr_port *repr_port;
	struct xsc_dev_config config;
	struct rte_eth_dev_data *dev_data;
	struct rte_ether_addr mac[XSC_MAX_MAC_ADDRESSES];
	struct rte_eth_rss_conf rss_conf;

	int representor_id;
	uint32_t ifindex;
	uint16_t mtu;
	uint8_t isolated;
	uint8_t is_representor;

	uint32_t mode:7;
	uint32_t member_bitmap:8;
	uint32_t funcid_type:3;
	uint32_t funcid:14;

	uint16_t eth_type;
	uint16_t qp_set_id;

	uint16_t num_sq;
	uint16_t num_rq;

	uint16_t flags;
	struct xsc_txq_data *(*txqs)[];
	struct xsc_rxq_data *(*rxqs)[];
};

#define TO_XSC_ETHDEV_PRIV(dev) ((struct xsc_ethdev_priv *)(dev)->data->dev_private)

#endif /* _XSC_ETHDEV_H_ */
