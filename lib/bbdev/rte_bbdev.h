/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017 Intel Corporation
 */

#ifndef _RTE_BBDEV_H_
#define _RTE_BBDEV_H_

/**
 * @file rte_bbdev.h
 *
 * Wireless base band device abstraction APIs.
 *
 * This API allows an application to discover, configure and use a device to
 * process operations. An asynchronous API (enqueue, followed by later dequeue)
 * is used for processing operations.
 *
 * The functions in this API are not thread-safe when called on the same
 * target object (a device, or a queue on a device), with the exception that
 * one thread can enqueue operations to a queue while another thread dequeues
 * from the same queue.
 */

#include <stdint.h>
#include <stdbool.h>

#include <rte_compat.h>
#include <rte_cpuflags.h>

#include "rte_bbdev_op.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "rte_bbdev_trace_fp.h"

#ifndef RTE_BBDEV_MAX_DEVS
#define RTE_BBDEV_MAX_DEVS 128  /**< Max number of devices */
#endif

/*
 * Maximum size to be used to manage the enum rte_bbdev_enqueue_status
 * including padding for future enum insertion.
 * The enum values must be explicitly kept smaller or equal to this padded maximum size.
 */
#define RTE_BBDEV_ENQ_STATUS_SIZE_MAX 6

/** Flags indicate current state of BBDEV device */
enum rte_bbdev_state {
	RTE_BBDEV_UNUSED,
	RTE_BBDEV_INITIALIZED
};

/**
 * Get the total number of devices that have been successfully initialised.
 *
 * @return
 *   The total number of usable devices.
 */
uint16_t
rte_bbdev_count(void);

/**
 * Check if a device is valid.
 *
 * @param dev_id
 *   The identifier of the device.
 *
 * @return
 *   true if device ID is valid and device is attached, false otherwise.
 */
bool
rte_bbdev_is_valid(uint16_t dev_id);

/**
 * Get the next enabled device.
 *
 * @param dev_id
 *   The current device
 *
 * @return
 *   - The next device, or
 *   - RTE_BBDEV_MAX_DEVS if none found
 */
uint16_t
rte_bbdev_find_next(uint16_t dev_id);

/** Iterate through all enabled devices */
#define RTE_BBDEV_FOREACH(i) for (i = rte_bbdev_find_next(-1); \
		i < RTE_BBDEV_MAX_DEVS; \
		i = rte_bbdev_find_next(i))

/**
 * Setup up device queues.
 * This function must be called on a device before setting up the queues and
 * starting the device. It can also be called when a device is in the stopped
 * state. If any device queues have been configured their configuration will be
 * cleared by a call to this function.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param num_queues
 *   Number of queues to configure on device.
 * @param socket_id
 *   ID of a socket which will be used to allocate memory.
 *
 * @return
 *   - 0 on success
 *   - -ENODEV if dev_id is invalid or the device is corrupted
 *   - -EINVAL if num_queues is invalid, 0 or greater than maximum
 *   - -EBUSY if the identified device has already started
 *   - -ENOMEM if unable to allocate memory
 */
int
rte_bbdev_setup_queues(uint16_t dev_id, uint16_t num_queues, int socket_id);

/**
 * Enable interrupts.
 * This function may be called before starting the device to enable the
 * interrupts if they are available.
 *
 * @param dev_id
 *   The identifier of the device.
 *
 * @return
 *   - 0 on success
 *   - -ENODEV if dev_id is invalid or the device is corrupted
 *   - -EBUSY if the identified device has already started
 *   - -ENOTSUP if the interrupts are not supported by the device
 */
int
rte_bbdev_intr_enable(uint16_t dev_id);

/** Device queue configuration structure */
struct rte_bbdev_queue_conf {
	int socket;  /**< NUMA socket used for memory allocation */
	uint32_t queue_size;  /**< Size of queue */
	uint8_t priority;  /**< Queue priority */
	bool deferred_start; /**< Do not start queue when device is started. */
	enum rte_bbdev_op_type op_type; /**< Operation type */
};

/**
 * Configure a queue on a device.
 * This function can be called after device configuration, and before starting.
 * It can also be called when the device or the queue is in the stopped state.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param conf
 *   The queue configuration. If NULL, a default configuration will be used.
 *
 * @return
 *   - 0 on success
 *   - EINVAL if the identified queue size or priority are invalid
 *   - EBUSY if the identified queue or its device have already started
 */
int
rte_bbdev_queue_configure(uint16_t dev_id, uint16_t queue_id,
		const struct rte_bbdev_queue_conf *conf);

/**
 * Start a device.
 * This is the last step needed before enqueueing operations is possible.
 *
 * @param dev_id
 *   The identifier of the device.
 *
 * @return
 *   - 0 on success
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_start(uint16_t dev_id);

/**
 * Stop a device.
 * The device can be reconfigured, and restarted after being stopped.
 *
 * @param dev_id
 *   The identifier of the device.
 *
 * @return
 *   - 0 on success
 */
int
rte_bbdev_stop(uint16_t dev_id);

/**
 * Close a device.
 * The device cannot be restarted without reconfiguration!
 *
 * @param dev_id
 *   The identifier of the device.
 *
 * @return
 *   - 0 on success
 */
int
rte_bbdev_close(uint16_t dev_id);

/**
 * Start a specified queue on a device.
 * This is only needed if the queue has been stopped, or if the deferred_start
 * flag has been set when configuring the queue.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   - 0 on success
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_queue_start(uint16_t dev_id, uint16_t queue_id);

/**
 * Stop a specified queue on a device, to allow re configuration.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   - 0 on success
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_queue_stop(uint16_t dev_id, uint16_t queue_id);

/**
 * Flags to indicate the reason why a previous enqueue may not have
 * consumed all requested operations.
 * In case of multiple reasons the latter supersedes a previous one.
 * The related macro RTE_BBDEV_ENQ_STATUS_SIZE_MAX can be used
 * as an absolute maximum for notably sizing array
 * while allowing for future enumeration insertion.
 */
enum rte_bbdev_enqueue_status {
	RTE_BBDEV_ENQ_STATUS_NONE,             /**< Nothing to report. */
	RTE_BBDEV_ENQ_STATUS_QUEUE_FULL,       /**< Not enough room in queue. */
	RTE_BBDEV_ENQ_STATUS_RING_FULL,        /**< Not enough room in ring. */
	RTE_BBDEV_ENQ_STATUS_INVALID_OP,       /**< Operation was rejected as invalid. */
	/* Note: RTE_BBDEV_ENQ_STATUS_SIZE_MAX must be larger or equal to maximum enum value. */
};

/**
 * Flags to indicate the status of the device.
 */
enum rte_bbdev_device_status {
	RTE_BBDEV_DEV_NOSTATUS,        /**< Nothing being reported. */
	RTE_BBDEV_DEV_NOT_SUPPORTED,   /**< Device status is not supported on the PMD. */
	RTE_BBDEV_DEV_RESET,           /**< Device in reset and un-configured state. */
	RTE_BBDEV_DEV_CONFIGURED,      /**< Device is configured and ready to use. */
	RTE_BBDEV_DEV_ACTIVE,          /**< Device is configured and VF is being used. */
	RTE_BBDEV_DEV_FATAL_ERR,       /**< Device has hit a fatal uncorrectable error. */
	RTE_BBDEV_DEV_RESTART_REQ,     /**< Device requires application to restart. */
	RTE_BBDEV_DEV_RECONFIG_REQ,    /**< Device requires application to reconfigure queues. */
	RTE_BBDEV_DEV_CORRECT_ERR,     /**< Warning of a correctable error event happened. */
};

/** Device statistics. */
struct rte_bbdev_stats {
	uint64_t enqueued_count;  /**< Count of all operations enqueued */
	uint64_t dequeued_count;  /**< Count of all operations dequeued */
	/** Total error count on operations enqueued */
	uint64_t enqueue_err_count;
	/** Total error count on operations dequeued */
	uint64_t dequeue_err_count;
	/** Total warning count on operations enqueued. */
	uint64_t enqueue_warn_count;
	/** Total warning count on operations dequeued. */
	uint64_t dequeue_warn_count;
	/** Total enqueue status count based on *rte_bbdev_enqueue_status* enum. */
	uint64_t enqueue_status_count[RTE_BBDEV_ENQ_STATUS_SIZE_MAX];
	/** CPU cycles consumed by the (HW/SW) accelerator device to offload
	 *  the enqueue request to its internal queues.
	 *  - For a HW device this is the cycles consumed in MMIO write
	 *  - For a SW (vdev) device, this is the processing time of the
	 *     bbdev operation
	 */
	uint64_t acc_offload_cycles;
	/** Available number of enqueue batch on that queue. */
	uint16_t enqueue_depth_avail;
};

/**
 * Retrieve the general I/O statistics of a device.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param stats
 *   Pointer to structure to where statistics will be copied. On error, this
 *   location may or may not have been modified.
 *
 * @return
 *   - 0 on success
 *   - EINVAL if invalid parameter pointer is provided
 */
int
rte_bbdev_stats_get(uint16_t dev_id, struct rte_bbdev_stats *stats);

/**
 * Reset the statistics of a device.
 *
 * @param dev_id
 *   The identifier of the device.
 * @return
 *   - 0 on success
 */
int
rte_bbdev_stats_reset(uint16_t dev_id);

/** Device information supplied by the device's driver */

/* Structure rte_bbdev_driver_info 8< */
struct rte_bbdev_driver_info {
	/** Driver name */
	const char *driver_name;

	/** Maximum number of queues supported by the device */
	unsigned int max_num_queues;
	/** Maximum number of queues supported per operation type */
	unsigned int num_queues[RTE_BBDEV_OP_TYPE_SIZE_MAX];
	/** Priority level supported per operation type */
	unsigned int queue_priority[RTE_BBDEV_OP_TYPE_SIZE_MAX];
	/** Queue size limit (queue size must also be power of 2) */
	uint32_t queue_size_lim;
	/** Set if device off-loads operation to hardware  */
	bool hardware_accelerated;
	/** Max value supported by queue priority for DL */
	uint8_t max_dl_queue_priority;
	/** Max value supported by queue priority for UL */
	uint8_t max_ul_queue_priority;
	/** Set if device supports per-queue interrupts */
	bool queue_intr_supported;
	/** Device Status */
	enum rte_bbdev_device_status device_status;
	/** HARQ memory available in kB */
	uint32_t harq_buffer_size;
	/** Minimum alignment of buffers, in bytes */
	uint16_t min_alignment;
	/** Byte endianness (RTE_BIG_ENDIAN/RTE_LITTLE_ENDIAN) supported
	 *  for input/output data
	 */
	uint8_t data_endianness;
	/** Default queue configuration used if none is supplied  */
	struct rte_bbdev_queue_conf default_queue_conf;
	/** Device operation capabilities */
	const struct rte_bbdev_op_cap *capabilities;
	/** Device cpu_flag requirements */
	const enum rte_cpu_flag_t *cpu_flag_reqs;
	/** FFT windowing width for 2048 FFT - size defined in capability. */
	uint16_t *fft_window_width;
};
/* >8 End of structure rte_bbdev_driver_info. */

/** Macro used at end of bbdev PMD list */
#define RTE_BBDEV_END_OF_CAPABILITIES_LIST() \
	{ RTE_BBDEV_OP_NONE }

/**
 * Device information structure used by an application to discover a devices
 * capabilities and current configuration
 */

/* Structure rte_bbdev_info 8< */
struct rte_bbdev_info {
	int socket_id;  /**< NUMA socket that device is on */
	const char *dev_name;  /**< Unique device name */
	const struct rte_device *device; /**< Device Information */
	uint16_t num_queues;  /**< Number of queues currently configured */
	bool started;  /**< Set if device is currently started */
	struct rte_bbdev_driver_info drv;  /**< Info from device driver */
};
/* >8 End of structure rte_bbdev_info. */

/**
 * Retrieve information about a device.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param dev_info
 *   Pointer to structure to where information will be copied. On error, this
 *   location may or may not have been modified.
 *
 * @return
 *   - 0 on success
 *   - EINVAL if invalid parameter pointer is provided
 */
int
rte_bbdev_info_get(uint16_t dev_id, struct rte_bbdev_info *dev_info);

/** Queue information */
struct rte_bbdev_queue_info {
	/** Current device configuration */
	struct rte_bbdev_queue_conf conf;
	/** Set if queue is currently started */
	bool started;
};

/**
 * Retrieve information about a specific queue on a device.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param queue_info
 *   Pointer to structure to where information will be copied. On error, this
 *   location may or may not have been modified.
 *
 * @return
 *   - 0 on success
 *   - EINVAL if invalid parameter pointer is provided
 */
int
rte_bbdev_queue_info_get(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_queue_info *queue_info);

/** @internal The data structure associated with each queue of a device. */
struct rte_bbdev_queue_data {
	void *queue_private;  /**< Driver-specific per-queue data */
	struct rte_bbdev_queue_conf conf;  /**< Current configuration */
	struct rte_bbdev_stats queue_stats;  /**< Queue statistics */
	enum rte_bbdev_enqueue_status enqueue_status; /**< Enqueue status when op is rejected */
	bool started;  /**< Queue state */
};

/** @internal Enqueue encode operations for processing on queue of a device. */
typedef uint16_t (*rte_bbdev_enqueue_enc_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_enc_op **ops,
		uint16_t num);

/** @internal Enqueue decode operations for processing on queue of a device. */
typedef uint16_t (*rte_bbdev_enqueue_dec_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_dec_op **ops,
		uint16_t num);

/** @internal Enqueue FFT operations for processing on queue of a device. */
typedef uint16_t (*rte_bbdev_enqueue_fft_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_fft_op **ops,
		uint16_t num);

/** @internal Enqueue MLD-TS operations for processing on queue of a device. */
typedef uint16_t (*rte_bbdev_enqueue_mldts_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_mldts_op **ops,
		uint16_t num);

/** @internal Dequeue encode operations from a queue of a device. */
typedef uint16_t (*rte_bbdev_dequeue_enc_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_enc_op **ops, uint16_t num);

/** @internal Dequeue decode operations from a queue of a device. */
typedef uint16_t (*rte_bbdev_dequeue_dec_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_dec_op **ops, uint16_t num);

/** @internal Dequeue FFT operations from a queue of a device. */
typedef uint16_t (*rte_bbdev_dequeue_fft_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_fft_op **ops, uint16_t num);

/** @internal Dequeue MLDTS operations from a queue of a device. */
typedef uint16_t (*rte_bbdev_dequeue_mldts_ops_t)(
		struct rte_bbdev_queue_data *q_data,
		struct rte_bbdev_mldts_op **ops, uint16_t num);

#define RTE_BBDEV_NAME_MAX_LEN  64  /**< Max length of device name */

/**
 * @internal The data associated with a device, with no function pointers.
 * This structure is safe to place in shared memory to be common among
 * different processes in a multi-process configuration. Drivers can access
 * these fields, but should never write to them!
 */
struct rte_bbdev_data {
	char name[RTE_BBDEV_NAME_MAX_LEN]; /**< Unique identifier name */
	void *dev_private;  /**< Driver-specific private data */
	uint16_t num_queues;  /**< Number of currently configured queues */
	struct rte_bbdev_queue_data *queues;  /**< Queue structures */
	uint16_t dev_id;  /**< Device ID */
	int socket_id;  /**< NUMA socket that device is on */
	bool started;  /**< Device run-time state */
	RTE_ATOMIC(uint16_t) process_cnt;  /** Counter of processes using the device */
};

/* Forward declarations */
struct rte_bbdev_ops;
struct rte_bbdev_callback;
struct rte_intr_handle;

/** Structure to keep track of registered callbacks */
RTE_TAILQ_HEAD(rte_bbdev_cb_list, rte_bbdev_callback);

/**
 * @internal The data structure associated with a device. Drivers can access
 * these fields, but should only write to the *_ops fields.
 */
struct __rte_cache_aligned rte_bbdev {
	/** Enqueue encode function */
	rte_bbdev_enqueue_enc_ops_t enqueue_enc_ops;
	/** Enqueue decode function */
	rte_bbdev_enqueue_dec_ops_t enqueue_dec_ops;
	/** Dequeue encode function */
	rte_bbdev_dequeue_enc_ops_t dequeue_enc_ops;
	/** Dequeue decode function */
	rte_bbdev_dequeue_dec_ops_t dequeue_dec_ops;
	/** Enqueue encode function */
	rte_bbdev_enqueue_enc_ops_t enqueue_ldpc_enc_ops;
	/** Enqueue decode function */
	rte_bbdev_enqueue_dec_ops_t enqueue_ldpc_dec_ops;
	/** Dequeue encode function */
	rte_bbdev_dequeue_enc_ops_t dequeue_ldpc_enc_ops;
	/** Dequeue decode function */
	rte_bbdev_dequeue_dec_ops_t dequeue_ldpc_dec_ops;
	/** Enqueue FFT function */
	rte_bbdev_enqueue_fft_ops_t enqueue_fft_ops;
	/** Dequeue FFT function */
	rte_bbdev_dequeue_fft_ops_t dequeue_fft_ops;
	const struct rte_bbdev_ops *dev_ops;  /**< Functions exported by PMD */
	struct rte_bbdev_data *data;  /**< Pointer to device data */
	enum rte_bbdev_state state;  /**< If device is currently used or not */
	struct rte_device *device; /**< Backing device */
	/** User application callback for interrupts if present */
	struct rte_bbdev_cb_list list_cbs;
	struct rte_intr_handle *intr_handle; /**< Device interrupt handle */
	/** Enqueue MLD-TS function */
	rte_bbdev_enqueue_mldts_ops_t enqueue_mldts_ops;
	/** Dequeue MLD-TS function */
	rte_bbdev_dequeue_mldts_ops_t dequeue_mldts_ops;
};

/** @internal array of all devices */
extern struct rte_bbdev rte_bbdev_devices[];

/**
 * Enqueue a burst of processed encode operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the @p ops array).
 */
static inline uint16_t
rte_bbdev_enqueue_enc_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_enc_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_TURBO_DEC));
	return dev->enqueue_enc_ops(q_data, ops, num_ops);
}

/**
 * Enqueue a burst of processed decode operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the @p ops array).
 */
static inline uint16_t
rte_bbdev_enqueue_dec_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_dec_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_TURBO_ENC));
	return dev->enqueue_dec_ops(q_data, ops, num_ops);
}

/**
 * Enqueue a burst of processed encode operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the @p ops array).
 */
static inline uint16_t
rte_bbdev_enqueue_ldpc_enc_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_enc_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_LDPC_ENC));
	return dev->enqueue_ldpc_enc_ops(q_data, ops, num_ops);
}

/**
 * Enqueue a burst of processed decode operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the @p ops array).
 */
static inline uint16_t
rte_bbdev_enqueue_ldpc_dec_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_dec_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_LDPC_DEC));
	return dev->enqueue_ldpc_dec_ops(q_data, ops, num_ops);
}

/**
 * Enqueue a burst of FFT operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued.
 *   Must have at least @p num_ops entries.
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued.
 *   (This is the number of processed entries in the @p ops array.)
 */
static inline uint16_t
rte_bbdev_enqueue_fft_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_fft_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_FFT));
	return dev->enqueue_fft_ops(q_data, ops, num_ops);
}

/**
 * Enqueue a burst of MLDTS operations to a queue of the device.
 * This functions only enqueues as many operations as currently possible and
 * does not block until @p num_ops entries in the queue are available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the @p ops array).
 */
static inline uint16_t
rte_bbdev_enqueue_mldts_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_mldts_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	rte_bbdev_trace_enqueue(dev_id, queue_id, (void **)ops, num_ops,
			rte_bbdev_op_type_str(RTE_BBDEV_OP_MLDTS));
	return dev->enqueue_mldts_ops(q_data, ops, num_ops);
}

/**
 * Dequeue a burst of processed encode operations from a queue of the device.
 * This functions returns only the current contents of the queue,
 * and does not block until @ num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to.
 *   Must have at least @p num_ops entries, i.e.
 *   a pointer to a table of void * pointers (ops) that will be filled.
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued.
 *   (This is the number of entries copied into the @p ops array.)
 */
static inline uint16_t
rte_bbdev_dequeue_enc_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_enc_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_enc_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_TURBO_ENC));
	return num_ops_dequeued;
}

/**
 * Dequeue a burst of processed decode operations from a queue of the device.
 * This functions returns only the current contents of the queue, and does not
 * block until @ num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   @p num_ops entries
 *   ie. A pointer to a table of void * pointers (ops) that will be filled.
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of entries
 *   copied into the @p ops array).
 */

static inline uint16_t
rte_bbdev_dequeue_dec_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_dec_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_dec_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_TURBO_DEC));
	return num_ops_dequeued;
}


/**
 * Dequeue a burst of processed encode operations from a queue of the device.
 * This functions returns only the current contents of the queue, and does not
 * block until @ num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of entries
 *   copied into the @p ops array).
 */
static inline uint16_t
rte_bbdev_dequeue_ldpc_enc_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_enc_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_ldpc_enc_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_LDPC_ENC));
	return num_ops_dequeued;
}

/**
 * Dequeue a burst of processed decode operations from a queue of the device.
 * This functions returns only the current contents of the queue, and does not
 * block until @ num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of entries
 *   copied into the @p ops array).
 */
static inline uint16_t
rte_bbdev_dequeue_ldpc_dec_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_dec_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_ldpc_dec_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_LDPC_DEC));
	return num_ops_dequeued;
}

/**
 * Dequeue a burst of FFT operations from a queue of the device.
 * This functions returns only the current contents of the queue, and does not
 * block until @ num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of entries
 *   copied into the @p ops array).
 */
static inline uint16_t
rte_bbdev_dequeue_fft_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_fft_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_fft_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_FFT));
	return num_ops_dequeued;
}

/**
 * Dequeue a burst of MLDTS operations from a queue of the device.
 * This functions returns only the current contents of the queue, and does not
 * block until @p num_ops is available.
 * This function does not provide any error notification to avoid the
 * corresponding overhead.
 *
 * @param dev_id
 *   The identifier of the device.
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   @p num_ops entries
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of entries
 *   copied into the @p ops array).
 */
static inline uint16_t
rte_bbdev_dequeue_mldts_ops(uint16_t dev_id, uint16_t queue_id,
		struct rte_bbdev_mldts_op **ops, uint16_t num_ops)
{
	struct rte_bbdev *dev = &rte_bbdev_devices[dev_id];
	struct rte_bbdev_queue_data *q_data = &dev->data->queues[queue_id];
	uint16_t num_ops_dequeued = dev->dequeue_mldts_ops(q_data, ops, num_ops);
	if (num_ops_dequeued > 0)
		rte_bbdev_trace_dequeue(dev_id, queue_id, (void **)ops, num_ops,
				num_ops_dequeued, rte_bbdev_op_type_str(RTE_BBDEV_OP_MLDTS));
	return num_ops_dequeued;
}

/** Definitions of device event types */
enum rte_bbdev_event_type {
	RTE_BBDEV_EVENT_UNKNOWN,  /**< unknown event type */
	RTE_BBDEV_EVENT_ERROR,  /**< error interrupt event */
	RTE_BBDEV_EVENT_DEQUEUE,  /**< dequeue event */
	RTE_BBDEV_EVENT_MAX  /**< max value of this enum */
};

/**
 * Typedef for application callback function registered by application
 * software for notification of device events
 *
 * @param dev_id
 *   Device identifier
 * @param event
 *   Device event to register for notification of.
 * @param cb_arg
 *   User specified parameter to be passed to user's callback function.
 * @param ret_param
 *   To pass data back to user application.
 */
typedef void (*rte_bbdev_cb_fn)(uint16_t dev_id,
		enum rte_bbdev_event_type event, void *cb_arg,
		void *ret_param);

/**
 * Register a callback function for specific device id. Multiple callbacks can
 * be added and will be called in the order they are added when an event is
 * triggered. Callbacks are called in a separate thread created by the DPDK EAL.
 *
 * @param dev_id
 *   Device id.
 * @param event
 *   The event that the callback will be registered for.
 * @param cb_fn
 *   User supplied callback function to be called.
 * @param cb_arg
 *   Pointer to parameter that will be passed to the callback.
 *
 * @return
 *   Zero on success, negative value on failure.
 */
int
rte_bbdev_callback_register(uint16_t dev_id, enum rte_bbdev_event_type event,
		rte_bbdev_cb_fn cb_fn, void *cb_arg);

/**
 * Unregister a callback function for specific device id.
 *
 * @param dev_id
 *   The device identifier.
 * @param event
 *   The event that the callback will be unregistered for.
 * @param cb_fn
 *   User supplied callback function to be unregistered.
 * @param cb_arg
 *   Pointer to the parameter supplied when registering the callback.
 *   (void *)-1 means to remove all registered callbacks with the specified
 *   function address.
 *
 * @return
 *   - 0 on success
 *   - EINVAL if invalid parameter pointer is provided
 *   - EAGAIN if the provided callback pointer does not exist
 */
int
rte_bbdev_callback_unregister(uint16_t dev_id, enum rte_bbdev_event_type event,
		rte_bbdev_cb_fn cb_fn, void *cb_arg);

/**
 * Enable a one-shot interrupt on the next operation enqueued to a particular
 * queue. The interrupt will be triggered when the operation is ready to be
 * dequeued. To handle the interrupt, an epoll file descriptor must be
 * registered using rte_bbdev_queue_intr_ctl(), and then an application
 * thread/lcore can wait for the interrupt using rte_epoll_wait().
 *
 * @param dev_id
 *   The device identifier.
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   - 0 on success
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_queue_intr_enable(uint16_t dev_id, uint16_t queue_id);

/**
 * Disable a one-shot interrupt on the next operation enqueued to a particular
 * queue (if it has been enabled).
 *
 * @param dev_id
 *   The device identifier.
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   - 0 on success
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_queue_intr_disable(uint16_t dev_id, uint16_t queue_id);

/**
 * Control interface for per-queue interrupts.
 *
 * @param dev_id
 *   The device identifier.
 * @param queue_id
 *   The index of the queue.
 * @param epfd
 *   Epoll file descriptor that will be associated with the interrupt source.
 *   If the special value RTE_EPOLL_PER_THREAD is provided, a per thread epoll
 *   file descriptor created by the EAL is used (RTE_EPOLL_PER_THREAD can also
 *   be used when calling rte_epoll_wait()).
 * @param op
 *   The operation be performed for the vector.RTE_INTR_EVENT_ADD or
 *   RTE_INTR_EVENT_DEL.
 * @param data
 *   User context, that will be returned in the epdata.data field of the
 *   rte_epoll_event structure filled in by rte_epoll_wait().
 *
 * @return
 *   - 0 on success
 *   - ENOTSUP if interrupts are not supported by the identified device
 *   - negative value on failure - as returned from PMD
 */
int
rte_bbdev_queue_intr_ctl(uint16_t dev_id, uint16_t queue_id, int epfd, int op,
		void *data);

/**
 * Convert device status from enum to string.
 *
 * @param status
 *   Device status as enum.
 *
 * @returns
 *   Device status as string or NULL if invalid.
 */
const char*
rte_bbdev_device_status_str(enum rte_bbdev_device_status status);

/**
 * Convert queue status from enum to string.
 *
 * @param status
 *   Queue status as enum.
 *
 * @returns
 *   Queue status as string or NULL if op_type is invalid.
 */
const char*
rte_bbdev_enqueue_status_str(enum rte_bbdev_enqueue_status status);

/**
 * Dump operations info from device to a file.
 * This API is used for debugging provided input operations, not a dataplane API.
 *
 *  @param dev_id
 *    The device identifier.
 *
 *  @param queue_index
 *    Index of queue.
 *
 *  @param file
 *    A pointer to a file for output.
 *
 * @returns
 *   - 0 on success
 *   - ENOTSUP if interrupts are not supported by the identified device
 *   - negative value on failure - as returned from PMD
 *
 */
__rte_experimental
int
rte_bbdev_queue_ops_dump(uint16_t dev_id, uint16_t queue_index, FILE *file);


/**
 * String of parameters related to the parameters of an operation of a given type.
 *
 *  @param op
 *    Pointer to an operation.
 *
 *  @param op_type
 *    Operation type enum.
 *
 *  @param str
 *    String being describing the operations.
 *
 *  @param len
 *    Size of the string buffer.
 *
 * @returns
 *   String describing the provided operation.
 *
 */
__rte_experimental
char *
rte_bbdev_ops_param_string(void *op, enum rte_bbdev_op_type op_type, char *str, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _RTE_BBDEV_H_ */
