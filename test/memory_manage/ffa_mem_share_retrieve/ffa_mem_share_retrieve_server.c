/*
 * Copyright (c) 2021, Arm Limited or its affliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"

uint32_t ffa_mem_share_retrieve_server(ffa_args_t args)
{
    ffa_args_t payload;
    uint32_t status = VAL_SUCCESS;
    ffa_endpoint_id_t sender = args.arg1 & 0xffff;
    ffa_endpoint_id_t receiver = (args.arg1 >> 16) & 0xffff;
    uint32_t fid = (uint32_t)args.arg4;
    mb_buf_t mb;
    uint8_t *pages = NULL;
    uint8_t *tmp_buf = NULL;
    uint32_t i;
    uint64_t size = PAGE_SIZE_4K;
    mem_region_init_t mem_region_init;
    ffa_memory_handle_t handle;
    struct ffa_memory_region_constituent constituents[1];
    const uint32_t constituents_count = sizeof(constituents) /
                sizeof(struct ffa_memory_region_constituent);

    mb.send = val_memory_alloc(size);
    mb.recv = val_memory_alloc(size);
    if (mb.send == NULL || mb.recv == NULL)
    {
        LOG(ERROR, "\tFailed to allocate RxTx buffer\n", 0, 0);
        status = VAL_ERROR_POINT(1);
        goto free_memory;
    }

    /* Map TX and RX buffers */
    if (val_rxtx_map_64((uint64_t)mb.send, (uint64_t)mb.recv, (uint32_t)(size/PAGE_SIZE_4K)))
    {
        LOG(ERROR, "\tRxTx Map failed\n", 0, 0);
        status = VAL_ERROR_POINT(2);
        goto free_memory;
    }

    pages = (uint8_t *)val_memory_alloc(size);
    if (!pages)
    {
        LOG(ERROR, "\tMemory allocation failed\n", 0, 0);
        status = VAL_ERROR_POINT(3);
        goto rxtx_unmap;
    }

    tmp_buf = (uint8_t *)val_memory_alloc(size);
    if (!tmp_buf)
    {
        LOG(ERROR, "\tMemory allocation failed\n", 0, 0);
        status = VAL_ERROR_POINT(4);
        goto rxtx_unmap;
    }

    /* Wait for the message. */
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload = val_resp_client_fn_direct((uint32_t)args.arg3, 0, 0, 0, 0, 0);
    if (payload.fid != FFA_MSG_SEND_DIRECT_REQ_64)
    {
        LOG(ERROR, "\tDirect request failed, fid=0x%x, err 0x%x\n",
                  payload.fid, payload.arg2);
        status =  VAL_ERROR_POINT(5);
        goto rxtx_unmap;
    }

    constituents[0].address = val_mem_virt_to_phys((void *)pages);
    constituents[0].page_count = 1;

    handle = payload.arg3;

    mem_region_init.memory_region = mb.send;
    mem_region_init.sender = receiver;
    mem_region_init.receiver = sender;
    mem_region_init.tag = 0;
    mem_region_init.flags = 0;
    mem_region_init.data_access = FFA_DATA_ACCESS_RW;
    mem_region_init.instruction_access = FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED;
    mem_region_init.type = FFA_MEMORY_NORMAL_MEM;
    mem_region_init.cacheability = FFA_MEMORY_CACHE_WRITE_BACK;
    mem_region_init.shareability = FFA_MEMORY_OUTER_SHAREABLE;
    val_ffa_memory_region_init(&mem_region_init, constituents, constituents_count);

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 = mem_region_init.total_length;
    payload.arg2 = mem_region_init.fragment_length;
    if (fid == FFA_MEM_SHARE_64)
        val_ffa_mem_retrieve_64(&payload);
    else
        val_ffa_mem_retrieve_32(&payload);

    if (payload.fid != FFA_MEM_RETRIEVE_RESP_32)
    {
        LOG(ERROR, "\tMem retrieve request failed err %x\n", payload.arg2, 0);
        status =  VAL_ERROR_POINT(6);
        goto rxtx_unmap;
    }

    val_memset(tmp_buf, 0xab, size);

    if (val_memcmp(pages, tmp_buf, size))
    {
        LOG(ERROR, "\tData mismatch\n", 0, 0);
        status =  VAL_ERROR_POINT(7);
        goto rx_release;
    }

    /* Check memory write access. */
    for (i = 0; i < PAGE_SIZE_4K; ++i)
    {
        ++pages[i];
    }

    /* relinquish the memory and notify the sender. */
    ffa_mem_relinquish_init((struct ffa_mem_relinquish *)mb.send, handle, 0, sender);
    val_memset(&payload, 0, sizeof(ffa_args_t));
    val_ffa_mem_relinquish(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "\tMem relinquish failed err %d\n", payload.arg2, 0);
        status = VAL_ERROR_POINT(8);
        goto rx_release;
    }

rx_release:
    if (val_rx_release())
    {
        LOG(ERROR, "\tval_rx_release failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(9);
    }

rxtx_unmap:
    if (val_rxtx_unmap(sender))
    {
        LOG(ERROR, "\tRXTX_UNMAP failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(10);
    }

free_memory:
    if (val_memory_free(mb.recv, size) || val_memory_free(mb.send, size))
    {
        LOG(ERROR, "\tfree_rxtx_buffers failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(11);
    }

    if (val_memory_free(pages, size))
    {
        LOG(ERROR, "\tval_mem_free failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(12);
    }

    if (val_memory_free(tmp_buf, size))
    {
        LOG(ERROR, "\tval_mem_free failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(13);
    }

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver;
    val_ffa_msg_send_direct_resp_64(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "\tDirect response failed err %d\n", payload.arg2, 0);
        status = status ? status : VAL_ERROR_POINT(14);
    }

    return status;
}
