/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <ethdev_driver.h>

const char usage_info[] = "usage: dpdk-devname\n";
/* Initialization of Environment Abstraction Layer (EAL). 8< */
int main(int argc, char **argv)
{
    int ret;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");
    /* >8 End of initialization of Environment Abstraction Layer */

    struct rte_eth_dev_info device_info;
    char device_name_by_port[RTE_ETH_NAME_MAX_LEN] = {0};
    for (uint16_t portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
    {
        if (!rte_eth_dev_is_valid_port(portid))
            continue;

        ret = rte_eth_dev_info_get(portid, &device_info);
        if (ret < 0)
        {
            fprintf(stderr,
                    "Invalid or no info for port %i, err: %s\n",
                    portid, rte_strerror(ret));
            continue;
        }
        ret = rte_eth_dev_get_name_by_port(portid, device_name_by_port);
        if (ret < 0)
        {
            fprintf(stderr,
                    "No name info returned for port %i, err: %s\n",
                    portid, rte_strerror(ret));
            continue;
        }
        struct rte_eth_dev_owner device_owner;
        ret = rte_eth_dev_owner_get(portid, &device_owner);
        if (ret < 0)
        {
            fprintf(stderr, "Could not get ownership for port %i (%s)\n",
                    portid, device_name_by_port);
            memset(&device_owner, 0, sizeof(struct rte_eth_dev_owner));
        }

        fprintf(stdout, "dpdk-devname found port:%i "
               "driver:%s "
               "eth_dev_info_name:%s "
               "get_name_by_port_name:%s "
               "owner_id:0x%016lx "
               "owner_name:%s\n",
               portid,
               device_info.driver_name,
               device_info.device->name,
               device_name_by_port,
               device_owner.id,
               device_owner.name);
    }

    /* clean up the EAL */
    rte_eal_cleanup();

    return 0;
}

// netvsc with matching vmbus device, use that
// netvsc without matching vmbus device name, probably management interface?
// ownership should make difference
