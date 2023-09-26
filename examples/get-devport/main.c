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
#include <rte_string_fns.h>
#include <ethdev_driver.h>

/* Initialization of Environment Abstraction Layer (EAL). 8< */
int main(int argc, char **argv)
{
    int ret;
    if (argc < 2)
    {
        fprintf(stderr, "usage: dpdk-get-devport [device name]\n");
    }
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");

    /* collect device name from arg, enforce max len */
    char device_name[RTE_ETH_NAME_MAX_LEN];
    strlcpy(device_name, argv[1], RTE_ETH_NAME_MAX_LEN);

    /* get port by name */
    uint16_t port_id;
    ret = rte_eth_dev_get_port_by_name(device_name, &port_id);
    if (ret != 0)
    {
        fprintf(stderr,
                "Could not find port for eth dev named %s, err: %s\n",
                device_name, rte_strerror(ret));
        return ret;
    }
    
    /* get port ownership info  */
    struct rte_eth_dev_owner owner;
    ret = rte_eth_dev_owner_get(port_id, &owner);
    if (ret < 0)
    {
        fprintf(stderr, "Could not get ownership for port %i (%s)\n",
                port_id, device_name);
        memset(&owner, 0, sizeof(struct rte_eth_dev_owner));
    }

    printf("Device %s\n"
           "  port_id: %i\n  owner_id: 0x%016lx\n  owner_name:%s\n",
           device_name, port_id, owner.id, owner.name);

    /* clean up the EAL */
    rte_eal_cleanup();

    return 0;
}
