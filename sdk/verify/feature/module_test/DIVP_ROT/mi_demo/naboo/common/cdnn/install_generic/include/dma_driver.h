/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef __DMA_DRIVER_H__
#define __DMA_DRIVER_H__

#include "cevaxm.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
    @addtogroup DMA_DRIVER QMAN/DDMA Driver
    @{
    @brief This module provides the API for working with the Data DMA (DDMA) engine through QMAN (Queue Manager) hardware.

    The DMA driver lets the developer create, enqueue, and track DMA requests.

    It contains the following functionality:
    - Allocate and delete a task queue in the Queue Manager hardware
    - Create descriptors for various DMA tasks
    - Update descriptors' parameters, such as tile width and height
    - Enqueue descriptors to the task queues
    - Track the completion of the tasks by polling and interrupt-based synchronization mechanisms
    @}
    */

#include <stdint.h>
#include "ceva_hw_cfg.h"

#if defined XM6 || defined VECC_XM6
    #ifndef CEVA_DMA_EXTENDED_DESC
       #define CEVA_DMA_EXTENDED_DESC
    #endif
#define CEVA_DMA_SUPPORT_3D
#endif //defined XM6 || defined VECC_XM6

/**
*   @addtogroup DMA_DRIVER_API QMAN/DDMA Driver API
*    @ingroup DMA_DRIVER
*   @{
*/

#ifdef CEVA_DMA_EXTENDED_DESC
#define DMA_DRIVER_VERSION     "XM4/XM6-2.0.0 (extended_desc)"
#define DDMA_DESC_SIZE_BYTES 32
#define DMA_MAX_TILE_HEIGHT      ((1<<16)-1)    /**< Max tile height*/
#define DMA_MAX_TILE_WIDTH       ((1<<16)-1)    /**< Max tile width (bytes) */
#define DMA_MAX_1D_SIZE          ((1<<22)-1)    /**< Max transfer size for 1D (bytes) */
#define DMA_MAX_INTERNAL_STRIDE  ((1<<16)-1)    /**< Max stride for internal memory (bytes) */
#define DMA_MAX_EXTERNAL_STRIDE  ((1<<16)-1)    /**< Max stride for external memory (bytes) */
#else // !CEVA_DMA_EXTENDED_DESC
#define DMA_DRIVER_VERSION     "XM4-1.3.3"
#define DDMA_DESC_SIZE_BYTES 16
// DMA transfer limits
#define DMA_MAX_TILE_HEIGHT      ((1<<10)-1)    /**< Max tile height*/
#define DMA_MAX_TILE_WIDTH       ((1<<10)-1)    /**< Max tile width (bytes) */
#define DMA_MAX_1D_SIZE          ((1<<20)-1)    /**< Max transfer size for 1D (bytes) */
#define DMA_MAX_INTERNAL_STRIDE  ((1<<10)-1)    /**< Max stride for internal memory (bytes) */
#define DMA_MAX_EXTERNAL_STRIDE  ((1<<16)-1)    /**< Max stride for external memory (bytes) */
#endif // !CEVA_DMA_EXTENDED_DESC

/** @} */

/**
*   @addtogroup DMA_DRIVER_TYPES QMAN/DDMA Driver Data Types
*    @ingroup DMA_DRIVER
*   @{
*/

/** @brief DMA task descriptor data structure (opaque) */
typedef struct {
    uint8_t bytes[DDMA_DESC_SIZE_BYTES];
} dma_desc_t;


/** @brief DMA return status enumeration */
typedef enum
{
    DMA_STATUS_OK,                              /**< Status OK */
    DMA_STATUS_ERROR_PARAMS,                    /**< Error in function parameters */
    DMA_STATUS_ALLOC_FAILED,                    /**< Failed to allocate queue */
    DMA_STATUS_ENQUEUE_FAILED,                  /**< Failed to send new task to queue */
    DMA_STATUS_MANAGER_FAILURE,                 /**< Queue manager failure */
    DMA_STATUS_NOT_IMPLEMENTED                  /**< Function not implemented yet */
} dma_status_e;

/** @brief DMA task direction enumeration */
#ifdef CEVA_DMA_EXTENDED_DESC
typedef enum
{
    DMA_DIR_INTERNAL_EXTERNAL,                  /**< Internal to External transfer */
    DMA_DIR_EXTERNAL_INTERNAL,                  /**< External to Internal transfer */
    DMA_DIR_INTERNAL_INTERNAL                   /**< Internal to Internal transfer */
} dma_transfer_dir_e;
#else
typedef enum
{
    DMA_DIR_EXTERNAL_INTERNAL,                  /**< External to Internal transfer */
    DMA_DIR_INTERNAL_EXTERNAL,                  /**< Internal to External transfer */
    DMA_DIR_INTERNAL_INTERNAL                   /**< Internal to Internal transfer */
} dma_transfer_dir_e;
#endif

/** @brief DMA task internal memory access enumeration */
typedef enum
{
    DMA_TYPE_LINEAR = 0,                        /**< Linear Transfer */
    DMA_TYPE_RESERVED,                          /**< Reserved */
    DMA_TYPE_ONE_BANK_READ,                     /**< Source of transfer is from internal one bank */
    DMA_TYPE_TWO_BANK_READ,                     /**< Source of transfer is from internal two consecutive banks */
    DMA_TYPE_DUPLICATE_ONE_BANK_WRITE,          /**< Destination of transfer is to internal one bank and should be duplicate to all other banks */
    DMA_TYPE_DUPLICATE_TWO_BANK_WRITE,          /**< Destination of transfer is to internal two consecutive banks and should be duplicate to all other consecutive banks */
    DMA_TYPE_ONE_BANK_WRITE,                    /**< Destination of transfer is to internal one bank */
    DMA_TYPE_TWO_BANK_WRITE                     /**< Destination of transfer is to internal two consecutive banks*/
} dma_transfer_type_e;

/** @brief DMA queue base structure */
typedef struct {
    uint32_t read_ptr;                          /**< current read pointer */
    uint32_t last_core_id;                      /**< last core ID */
    uint32_t write_ptr;                         /**< current write pointer */
    uint32_t id;                                /**< the queue's ID */
    uint32_t start_address;                     /**< pointer to the memory of the queue */
    uint32_t queue_depth_register;              /**< depth register */
    volatile uint32_t current_sync_value;       /**< current sync point value */
    uint32_t sync_counter;                      /**< current sync point counter */
    dma_desc_t sync_message_desc;               /**< sync point descriptor */
} dma_queue_base_t;


/** @brief DMA sync message type */
typedef void* dma_sync_message_t;

/** @} */

    /**
    *   @addtogroup DMA_DRIVER_API QMAN/DDMA Driver API
    *    @ingroup DMA_DRIVER
    *   @{
    */

    /** @brief Enqueues a new task to queue
    * @param[in]     queue - pointer to the queue
    * @param[in]     desc - pointer to the descriptor
    * @param[in]     src - source address
    * @param[in]     dst - destination address
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the operation completed successfully
    * @retval #DMA_STATUS_ENQUEUE_FAILED - failed to enqueue the task, because the queue is full
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue or desc or there was an error in parameters
    */
    dma_status_e dma_enqueue_desc(dma_queue_base_t* queue, dma_desc_t* desc, void* src, void* dst);

    /**
    * @brief Allocates a new queue and initialize its hardware resources
    * @param[out]    out_queue - pointer to the queue base structure (must be aligned to 32 bytes)
    * @param[in]     start_address - pointer to the queue's start address (must be aligned to 32 bytes)
    * @param[in]     priority - the queue's priority
    * @param[in]     depth - the number of descriptors in the queue (between 2 and 16383). This value is rounded down to the nearest power of two value.
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - a queue allocated successfully
    * @retval #DMA_STATUS_ALLOC_FAILED - failed to allocate a queue: no available queue found
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer in out_queue or there was an error in parameters
    */
    dma_status_e dma_allocate_queue(dma_queue_base_t* out_queue, uint32_t* start_address, uint32_t priority, uint32_t depth);

    /**
    * @brief Deletes the queue and free its hardware resources
    * @param[in]     queue - pointer to the queue base structure
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the queue was deleted successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer
    */
    dma_status_e dma_delete_queue(dma_queue_base_t* queue);

    /**
    * @brief Creates an internal message transfer task descriptor
    * @param[out]     desc - pointer to the descriptor to initialize
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was initialized successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer
    */
    dma_status_e dma_create_internal_message_desc(dma_desc_t* desc);

    /**
    * @brief Create an external message transfer task descriptor
    * @param[out]     desc - pointer to the descriptor to initialize
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was initialized successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer
    */
    dma_status_e dma_create_external_message_desc(dma_desc_t* desc);

    /**
    * @brief Creates a one-dimensional transfer task descriptor
    * @param[out]    desc - pointer to the descriptor to initialize
    * @param[in]     length - number of bytes to copy
    * @param[in]     dir - direction of the transfer
    * @param[in]     tr_type - internal memory access type
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was initialized successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_create_1d_desc(dma_desc_t* desc, uint32_t length, dma_transfer_dir_e dir, dma_transfer_type_e tr_type);

    /**
    * @brief Creates a two-dimensional transfer task descriptor
    * @param[out]    desc - pointer to the descriptor to initialize
    * @param[in]     width - number of bytes in a row to copy
    * @param[in]     height - number of rows to copy
    * @param[in]     src_stride - source stride in bytes
    * @param[in]     dst_stride - destination stride in bytes
    * @param[in]     dir - direction of the transfer
    * @param[in]     tr_type - internal memory access type
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was initialized successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_create_2d_desc(dma_desc_t* desc,
        uint32_t width,
        uint32_t height,
        uint32_t src_stride,
        uint32_t dst_stride,
        dma_transfer_dir_e dir,
        dma_transfer_type_e tr_type);

    /**
    * @brief Updates the length parameter of a one-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     length - number of bytes to copy
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_1d_desc_size(dma_desc_t* desc, uint32_t length);

    /**
    * @brief Updates the tile dimensions in a two-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     height - number of rows in tile
    * @param[in]     width - number of bytes int a tile width
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_2d_desc_size(dma_desc_t* desc, uint32_t height, uint32_t width);

    /**
    * @brief Updates the stride parameters in a two-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     src_stride - source stride in bytes
    * @param[in]     dst_stride - destination stride in bytes
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_2d_desc_stride(dma_desc_t* desc, uint32_t src_stride, uint32_t dst_stride);

#ifdef CEVA_DMA_SUPPORT_3D
    /**
    * @brief Creates a three-dimensional transfer task descriptor
    * @param[out]    desc - pointer to the descriptor to initialize
    * @param[in]     width - number of bytes in a row to copy
    * @param[in]     height - number of rows to copy
    * @param[in]     num_planes - number of planes
    * @param[in]     src_stride - source line stride in bytes
    * @param[in]     dst_stride - destination line stride in bytes
    * @param[in]     src_plane_stride - source plane stride in bytes
    * @param[in]     dst_plane_stride - destination plane stride in bytes
    * @param[in]     dir - direction of the transfer
    * @param[in]     tr_type - internal memory access type
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was initialized successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */

    dma_status_e dma_create_3d_desc(dma_desc_t* desc,
        uint32_t width,
        uint32_t height,
        uint32_t num_planes,
        uint32_t src_stride,
        uint32_t dst_stride,
        uint32_t src_plane_stride,
        uint32_t dst_plane_stride,
        dma_transfer_dir_e dir,
        dma_transfer_type_e tr_type);

    /**
    * @brief Updates the tile dimensions in a three-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     height - number of rows in tile
    * @param[in]     width - number of bytes int a tile width
    * @param[in]     num_planes - number of planes
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_3d_desc_size(dma_desc_t* desc, uint32_t height, uint32_t width, uint32_t num_planes);

    /**
    * @brief Updates the stride parameters in a three-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     src_stride - source line stride in bytes
    * @param[in]     dst_stride - destination line stride in bytes
    * @param[in]     src_plane_stride - source plane stride in bytes
    * @param[in]     dst_plane_stride - destination plane stride in bytes
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_3d_desc_stride(dma_desc_t* desc, uint32_t src_stride, uint32_t dst_stride, uint32_t src_plane_stride, uint32_t dst_plane_stride);

#endif // CEVA_DMA_SUPPORT_3D

// API for setting write barrier is not supported yet
#if 0
    /**
    * @brief Updates write barrier flag in an external message transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     write_barrier_en - barrier enable flag
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_external_message_desc_write_barrier(dma_desc_t* desc, uint32_t write_barrier_en);

    /**
    * @brief Updates write barrier flag in a one-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     write_barrier_en - barrier enable flag
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_1d_desc_write_barrier(dma_desc_t* desc, uint32_t write_barrier_en);

    /**
    * @brief Updates write barrier flag in a two-dimensional transfer task descriptor
    * @param[in,out] desc - pointer to the descriptor to update
    * @param[in]     write_barrier_en - boolean barrier enable flag
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the task descriptor was updated successfully
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer or there was an error in parameters
    */
    dma_status_e dma_update_2d_desc_write_barrier(dma_desc_t* desc, uint32_t write_barrier_en);
#endif

    /**
    * @brief Enqueues a new sync point
    * @param[in]     queue - pointer to the queue base structure
    * @param[out]    value - pointer to the new sync point value, which can be used with #dma_wait_sync_point
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - enqueue operation completed successfully
    * @retval #DMA_STATUS_ENQUEUE_FAILED - failed to enqueue the sync point, because the queue is full
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue or value or there was an error in parameters
    */
    dma_status_e dma_enqueue_sync_point(dma_queue_base_t* queue, uint32_t* value);

    /**
    * @brief Waits for a sync point
    *
    * The function blocks until the expected sync point value is obtained. The function uses a busy-wait polling loop.
    * @param[in]     queue - pointer to the queue base structure
    * @param[in]     value - expected sync point value (obtained from #dma_enqueue_sync_point)
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue
    */
    dma_status_e dma_wait_sync_point(dma_queue_base_t* queue, uint32_t value);

    /**
    * @brief Resets the sync point value and counter to 0
    * @param[in]     queue - pointer to the queue base structure
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue
    */
    dma_status_e dma_reset_sync_point(dma_queue_base_t* queue);

    /**
    * @brief Initializes the Queue Manager driver
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_MANAGER_FAILURE - Queue Manager driver initialization failed
    */
    dma_status_e dma_init_manager();

    /**
    * @brief Deinitializes the Queue Manager driver
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_MANAGER_FAILURE - Queue Manager driver deinitialization failed
    */
    dma_status_e dma_deinit_manager();

    /**
    * @brief Enables the queue
    * @param[in,out]     queue - pointer to the queue base structure
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue
    */
    dma_status_e dma_enable_queue(dma_queue_base_t* queue);

    /**
    * @brief Disables the queue
    * @param[in,out]     queue - pointer to the queue base structure
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - no error
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue
    */
    dma_status_e dma_disable_queue(dma_queue_base_t* queue);


    /** @brief Enqueues a new task to the queue with the synchronization message to be read by the DDMA interrupt handler
    * @param[in]     queue - pointer to the queue base structure
    * @param[in]     desc - pointer to the task descriptor
    * @param[in]     src - source address
    * @param[in]     dst - destination address
    * @param[in]     message - synchronization message for the interrupt handler
    * @return #dma_status_e status code
    * @retval #DMA_STATUS_OK - the operation completed successfully
    * @retval #DMA_STATUS_ENQUEUE_FAILED - failed to enqueue the task, because the queue is full
    * @retval #DMA_STATUS_ERROR_PARAMS - if passed the NULL pointer to queue or desc or there was an error in the parameters
    * @note  Sync FIFO mechanism relies on 32-bit addresses and cannot be simulated on Win64
    */
    dma_status_e dma_enqueue_desc_with_sync(dma_queue_base_t* queue, dma_desc_t* desc, void* src, void* dst, dma_sync_message_t message);


    /** @brief Retrieves the next synchronization message
    *
    *  This function is intended to be invoked by the DDMA interrupt handler
    *  @param[out] p_message - pointer to the location for storing the synchronization message
    *  @retval non-zero if there is a synchronization message. When zero is returned, the value written to p_message should be ignored.
    */
    uint32_t dma_get_sync_message(dma_sync_message_t* p_message);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif //__DMA_DRIVER_H__