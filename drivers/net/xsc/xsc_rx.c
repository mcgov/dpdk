/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025 Yunsilicon Technology Co., Ltd.
 */

#include <rte_io.h>

#include "xsc_log.h"
#include "xsc_defs.h"
#include "xsc_dev.h"
#include "xsc_ethdev.h"
#include "xsc_cmd.h"
#include "xsc_rx.h"

#define XSC_MAX_RECV_LEN 9800

static void
xsc_rxq_initialize(struct xsc_dev *xdev, struct xsc_rxq_data *rxq_data)
{
	const uint32_t wqe_n = rxq_data->wqe_s;
	uint32_t i;
	uint32_t seg_len = 0;
	struct xsc_hwinfo *hwinfo = &xdev->hwinfo;
	uint32_t rx_ds_num = hwinfo->recv_seg_num;
	uint32_t log2ds = rte_log2_u32(rx_ds_num);
	uintptr_t addr;
	struct rte_mbuf *mbuf;
	void *jumbo_buffer_pa = xdev->jumbo_buffer_pa;
	void *jumbo_buffer_va = xdev->jumbo_buffer_va;
	volatile struct xsc_wqe_data_seg *seg;
	volatile struct xsc_wqe_data_seg *seg_next;

	for (i = 0; (i != wqe_n); ++i) {
		mbuf = (*rxq_data->elts)[i];
		seg = &((volatile struct xsc_wqe_data_seg *)rxq_data->wqes)[i * rx_ds_num];
		addr = (uintptr_t)rte_pktmbuf_iova(mbuf);
		if (rx_ds_num == 1)
			seg_len = XSC_MAX_RECV_LEN;
		else
			seg_len = rte_pktmbuf_data_len(mbuf);
		*seg = (struct xsc_wqe_data_seg){
			.va = rte_cpu_to_le_64(addr),
			.seg_len = rte_cpu_to_le_32(seg_len),
			.lkey = 0,
		};

		if (rx_ds_num != 1) {
			seg_next = seg + 1;
			if (jumbo_buffer_va == NULL) {
				jumbo_buffer_pa = rte_malloc(NULL, XSC_MAX_RECV_LEN, 0);
				if (jumbo_buffer_pa == NULL) {
					/* Rely on mtu */
					seg->seg_len = XSC_MAX_RECV_LEN;
					PMD_DRV_LOG(ERR, "Failed to malloc jumbo_buffer");
					continue;
				} else {
					jumbo_buffer_va =
						(void *)rte_malloc_virt2iova(jumbo_buffer_pa);
					if ((rte_iova_t)jumbo_buffer_va == RTE_BAD_IOVA) {
						seg->seg_len = XSC_MAX_RECV_LEN;
						PMD_DRV_LOG(ERR, "Failed to turn jumbo_buffer");
						continue;
					}
				}
				xdev->jumbo_buffer_pa = jumbo_buffer_pa;
				xdev->jumbo_buffer_va = jumbo_buffer_va;
			}
			*seg_next = (struct xsc_wqe_data_seg){
				.va = rte_cpu_to_le_64((uint64_t)jumbo_buffer_va),
				.seg_len = rte_cpu_to_le_32(XSC_MAX_RECV_LEN - seg_len),
				.lkey = 0,
			};
		}
	}

	rxq_data->rq_ci = wqe_n;
	rxq_data->sge_n = log2ds;

	union xsc_recv_doorbell recv_db = {
		.recv_data = 0
	};

	recv_db.next_pid = wqe_n << log2ds;
	recv_db.qp_num = rxq_data->qpn;
	rte_write32(rte_cpu_to_le_32(recv_db.recv_data), rxq_data->rq_db);
}

static int
xsc_rss_qp_create(struct xsc_ethdev_priv *priv, int port_id)
{
	struct xsc_cmd_create_multiqp_mbox_in *in;
	struct xsc_cmd_create_qp_request *req;
	struct xsc_cmd_create_multiqp_mbox_out *out;
	uint8_t log_ele;
	uint64_t iova;
	int wqe_n;
	int in_len, out_len, cmd_len;
	int entry_total_len, entry_len;
	uint8_t log_rq_sz, log_sq_sz = 0;
	uint32_t wqe_total_len;
	int j, ret;
	uint16_t i, pa_num;
	int rqn_base;
	struct xsc_rxq_data *rxq_data;
	struct xsc_dev *xdev = priv->xdev;
	struct xsc_hwinfo *hwinfo = &xdev->hwinfo;
	char name[RTE_ETH_NAME_MAX_LEN] = { 0 };

	rxq_data = xsc_rxq_get(priv, 0);
	log_ele = rte_log2_u32(sizeof(struct xsc_wqe_data_seg));
	wqe_n = rxq_data->wqe_s;
	log_rq_sz = rte_log2_u32(wqe_n * hwinfo->recv_seg_num);
	wqe_total_len = 1 << (log_rq_sz + log_sq_sz + log_ele);

	pa_num = (wqe_total_len + XSC_PAGE_SIZE - 1) / XSC_PAGE_SIZE;
	entry_len = sizeof(struct xsc_cmd_create_qp_request) + sizeof(uint64_t) * pa_num;
	entry_total_len = entry_len * priv->num_rq;

	in_len = sizeof(struct xsc_cmd_create_multiqp_mbox_in) + entry_total_len;
	out_len = sizeof(struct xsc_cmd_create_multiqp_mbox_out) + entry_total_len;
	cmd_len = RTE_MAX(in_len, out_len);
	in = malloc(cmd_len);
	memset(in, 0, cmd_len);
	if (in == NULL) {
		rte_errno = ENOMEM;
		PMD_DRV_LOG(ERR, "Alloc rss qp create cmd memory failed");
		goto error;
	}

	in->qp_num = rte_cpu_to_be_16((uint16_t)priv->num_rq);
	in->qp_type = XSC_QUEUE_TYPE_RAW;
	in->req_len = rte_cpu_to_be_32(cmd_len);

	for (i = 0; i < priv->num_rq; i++) {
		rxq_data = (*priv->rxqs)[i];
		req = (struct xsc_cmd_create_qp_request *)(&in->data[0] + entry_len * i);
		req->input_qpn = rte_cpu_to_be_16(0); /* useless for eth */
		req->pa_num = rte_cpu_to_be_16(pa_num);
		req->qp_type = XSC_QUEUE_TYPE_RAW;
		req->log_rq_sz = log_rq_sz;
		req->cqn_recv = rte_cpu_to_be_16((uint16_t)rxq_data->cqn);
		req->cqn_send = req->cqn_recv;
		req->glb_funcid = rte_cpu_to_be_16((uint16_t)hwinfo->func_id);
		/* Alloc pas addr */
		snprintf(name, sizeof(name), "wqe_mem_rx_%d_%d", port_id, i);
		rxq_data->rq_pas = rte_memzone_reserve_aligned(name,
							       (XSC_PAGE_SIZE * pa_num),
							       SOCKET_ID_ANY,
							       0, XSC_PAGE_SIZE);
		if (rxq_data->rq_pas == NULL) {
			rte_errno = ENOMEM;
			PMD_DRV_LOG(ERR, "Alloc rxq pas memory failed");
			goto error;
		}

		iova = rxq_data->rq_pas->iova;
		for (j = 0; j < pa_num; j++)
			req->pas[j] = rte_cpu_to_be_64(iova + j * XSC_PAGE_SIZE);
	}

	in->hdr.opcode = rte_cpu_to_be_16(XSC_CMD_OP_CREATE_MULTI_QP);
	out = (struct xsc_cmd_create_multiqp_mbox_out *)in;
	ret = xsc_dev_mailbox_exec(xdev, in, in_len, out, out_len);
	if (ret != 0 || out->hdr.status != 0) {
		PMD_DRV_LOG(ERR,
			    "Create rss rq failed, port id=%d, qp_num=%d, ret=%d, out.status=%u",
			    port_id, priv->num_rq, ret, out->hdr.status);
		rte_errno = ENOEXEC;
		goto error;
	}
	rqn_base = rte_be_to_cpu_32(out->qpn_base) & 0xffffff;

	for (i = 0; i < priv->num_rq; i++) {
		rxq_data = xsc_rxq_get(priv, i);
		rxq_data->wqes = rxq_data->rq_pas->addr;
		if (!xsc_dev_is_vf(xdev))
			rxq_data->rq_db = (uint32_t *)((uint8_t *)xdev->bar_addr +
					  XSC_PF_RX_DB_ADDR);
		else
			rxq_data->rq_db = (uint32_t *)((uint8_t *)xdev->bar_addr +
					  XSC_VF_RX_DB_ADDR);

		rxq_data->qpn = rqn_base + i;
		xsc_dev_modify_qp_status(xdev, rxq_data->qpn, 1, XSC_CMD_OP_RTR2RTS_QP);
		xsc_rxq_initialize(xdev, rxq_data);
		rxq_data->cq_ci = 0;
		priv->dev_data->rx_queue_state[i] = RTE_ETH_QUEUE_STATE_STARTED;
		PMD_DRV_LOG(INFO, "Port %u create rx qp, wqe_s:%d, wqe_n:%d, qp_db=%p, qpn:%d",
			    port_id,
			    rxq_data->wqe_s, rxq_data->wqe_n,
			    rxq_data->rq_db, rxq_data->qpn);
	}

	free(in);
	return 0;

error:
	free(in);
	return -rte_errno;
}

int
xsc_rxq_rss_obj_new(struct xsc_ethdev_priv *priv, uint16_t port_id)
{
	int ret;
	uint32_t i;
	struct xsc_dev *xdev = priv->xdev;
	struct xsc_rxq_data *rxq_data;
	struct xsc_rx_cq_params cq_params = {0};
	struct xsc_rx_cq_info cq_info = {0};

	/* Create CQ */
	for (i = 0; i < priv->num_rq; ++i) {
		rxq_data = xsc_rxq_get(priv, i);

		memset(&cq_params, 0, sizeof(cq_params));
		memset(&cq_info, 0, sizeof(cq_info));
		cq_params.port_id = rxq_data->port_id;
		cq_params.qp_id = rxq_data->idx;
		cq_params.wqe_s = rxq_data->wqe_s;

		ret = xsc_dev_rx_cq_create(xdev, &cq_params, &cq_info);
		if (ret) {
			PMD_DRV_LOG(ERR, "Port %u rxq %u create cq fail", port_id, i);
			rte_errno = errno;
			goto error;
		}

		rxq_data->cq = cq_info.cq;
		rxq_data->cqe_n = cq_info.cqe_n;
		rxq_data->cqe_s = 1 << rxq_data->cqe_n;
		rxq_data->cqe_m = rxq_data->cqe_s - 1;
		rxq_data->cqes = cq_info.cqes;
		rxq_data->cq_db = cq_info.cq_db;
		rxq_data->cqn = cq_info.cqn;

		PMD_DRV_LOG(INFO, "Port %u create rx cq, cqe_s:%d, cqe_n:%d, cq_db=%p, cqn:%d",
			    port_id,
			    rxq_data->cqe_s, rxq_data->cqe_n,
			    rxq_data->cq_db, rxq_data->cqn);
	}

	ret = xsc_rss_qp_create(priv, port_id);
	if (ret != 0) {
		PMD_DRV_LOG(ERR, "Port %u rss rxq create fail", port_id);
		goto error;
	}
	return 0;

error:
	return -rte_errno;
}

int
xsc_rxq_elts_alloc(struct xsc_rxq_data *rxq_data)
{
	uint32_t elts_s = rxq_data->wqe_s;
	struct rte_mbuf *mbuf;
	uint32_t i;

	for (i = 0; (i != elts_s); ++i) {
		mbuf = rte_pktmbuf_alloc(rxq_data->mp);
		if (mbuf == NULL) {
			PMD_DRV_LOG(ERR, "Port %u rxq %u empty mbuf pool",
				    rxq_data->port_id, rxq_data->idx);
			rte_errno = ENOMEM;
			goto error;
		}

		mbuf->port = rxq_data->port_id;
		mbuf->nb_segs = 1;
		rte_pktmbuf_data_len(mbuf) = rte_pktmbuf_data_room_size(rxq_data->mp)
						- mbuf->data_off;
		rte_pktmbuf_pkt_len(mbuf) = rte_pktmbuf_data_room_size(rxq_data->mp)
						- mbuf->data_off;
		(*rxq_data->elts)[i] = mbuf;
	}

	return 0;
error:
	elts_s = i;
	for (i = 0; (i != elts_s); ++i) {
		if ((*rxq_data->elts)[i] != NULL)
			rte_pktmbuf_free_seg((*rxq_data->elts)[i]);
		(*rxq_data->elts)[i] = NULL;
	}

	PMD_DRV_LOG(ERR, "Port %u rxq %u start failed, free elts",
		    rxq_data->port_id, rxq_data->idx);

	return -rte_errno;
}

void
xsc_rxq_elts_free(struct xsc_rxq_data *rxq_data)
{
	uint16_t i;

	if (rxq_data->elts == NULL)
		return;
	for (i = 0; i != rxq_data->wqe_s; ++i) {
		if ((*rxq_data->elts)[i] != NULL)
			rte_pktmbuf_free_seg((*rxq_data->elts)[i]);
		(*rxq_data->elts)[i] = NULL;
	}

	PMD_DRV_LOG(DEBUG, "Port %u rxq %u free elts", rxq_data->port_id, rxq_data->idx);
}

void
xsc_rxq_rss_obj_release(struct xsc_dev *xdev, struct xsc_rxq_data *rxq_data)
{
	struct xsc_cmd_destroy_qp_mbox_in in = { .hdr = { 0 } };
	struct xsc_cmd_destroy_qp_mbox_out out = { .hdr = { 0 } };
	int ret, in_len, out_len;
	uint32_t qpn = rxq_data->qpn;

	xsc_dev_modify_qp_status(xdev, qpn, 1, XSC_CMD_OP_QP_2RST);

	in_len = sizeof(struct xsc_cmd_destroy_qp_mbox_in);
	out_len = sizeof(struct xsc_cmd_destroy_qp_mbox_out);
	in.hdr.opcode = rte_cpu_to_be_16(XSC_CMD_OP_DESTROY_QP);
	in.qpn = rte_cpu_to_be_32(rxq_data->qpn);

	ret = xsc_dev_mailbox_exec(xdev, &in, in_len, &out, out_len);
	if (ret != 0 || out.hdr.status != 0) {
		PMD_DRV_LOG(ERR,
			    "Release rss rq failed, port id=%d, qid=%d, err=%d, out.status=%u",
			    rxq_data->port_id, rxq_data->idx, ret, out.hdr.status);
		rte_errno = ENOEXEC;
		return;
	}

	rte_memzone_free(rxq_data->rq_pas);

	if (rxq_data->cq != NULL)
		xsc_dev_destroy_cq(xdev, rxq_data->cq);
	rxq_data->cq = NULL;
}
