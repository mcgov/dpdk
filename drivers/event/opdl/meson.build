# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2018 Luca Boccassi <bluca@debian.org>

sources = files(
        'opdl_evdev.c',
        'opdl_evdev_init.c',
        'opdl_evdev_xstats.c',
        'opdl_ring.c',
        'opdl_test.c',
)
deps += ['bus_vdev']
require_iova_in_mbuf = false
cflags += no_wvla_cflag
