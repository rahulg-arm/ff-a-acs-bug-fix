/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"

#define WD_TIME_OUT 100U

static volatile uint32_t managed_exit_received;
static uint32_t mei_id;

static int mei_irq_handler(void)
{
    managed_exit_received = true;
    val_secure_intr_disable(mei_id, INTERRUPT_TYPE_FIQ);
    LOG(DBG, "MEI Handler Processed");
    return 0;
}

static uint32_t mei_enabled_sp4_setup(ffa_args_t args)
{
    ffa_args_t payload;
    uint32_t status = VAL_SUCCESS;
    ffa_endpoint_id_t sender = args.arg1 & 0xffff;
    ffa_endpoint_id_t receiver = (args.arg1 >> 16) & 0xffff;
    ffa_endpoint_id_t receiver_1 = val_get_endpoint_id(SP2);
    uint32_t test_run_data = (uint32_t)args.arg3;

    test_run_data = TEST_RUN_DATA(GET_TEST_NUM((uint32_t)test_run_data),
      (uint32_t)sender, (uint32_t)receiver_1, GET_TEST_TYPE((uint32_t)test_run_data));

    val_select_server_fn_direct(test_run_data, 0, 0, 0, 0);

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 = FFA_FEATURE_MEI;
    val_ffa_features(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Failed to retrieve MEI err %x", payload.arg2);
        status =  VAL_ERROR_POINT(1);
        goto exit;
    }

    mei_id = (uint32_t)payload.arg2;
    if (val_irq_register_handler(mei_id, mei_irq_handler))
    {
        LOG(ERROR, "MEI interrupt register failed");
        status = VAL_ERROR_POINT(2);
        goto exit;
    }
    val_secure_intr_enable(mei_id, INTERRUPT_TYPE_FIQ);
    LOG(DBG, "MEI Handler Registered, MEI ID %x", mei_id);

    /* Wait for the message. */
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload = val_resp_client_fn_direct((uint32_t)args.arg3, 0, 0, 0, 0, 0);
    if (payload.fid != FFA_MSG_SEND_DIRECT_REQ_64)
    {
        LOG(ERROR, "Direct request failed, fid=0x%x, err 0x%x",
                  payload.fid, payload.arg2);
        status =  VAL_ERROR_POINT(3);
        goto free_interrupt;
    }

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver_1;
    val_ffa_msg_send_direct_req_64(&payload);
    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "DIRECT_RESP_64 not received fid %x err %x", payload.fid, payload.arg2);
        status = VAL_ERROR_POINT(4);
        goto free_interrupt;
    }

    if (!managed_exit_received)
    {
        LOG(ERROR, "MEI not triggered");
        status =  VAL_ERROR_POINT(5);
        goto free_interrupt;
    }

    /* Wait for the message. */
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver;
    val_ffa_msg_send_direct_resp_64(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Direct response failed err %x", payload.arg2);
        status =  VAL_ERROR_POINT(6);
        goto exit;
    }

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver_1;
    val_ffa_msg_send_direct_req_64(&payload);
    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "DIRECT_RESP_64 not received fid %x err %x", payload.fid, payload.arg2);
        status = VAL_ERROR_POINT(7);
        goto free_interrupt;
    }

free_interrupt:
    if (val_irq_unregister_handler(mei_id))
    {
        LOG(ERROR, "IRQ handler unregister failed");
        status = status ? status : VAL_ERROR_POINT(8);
    }

exit:
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver;
    val_ffa_msg_send_direct_resp_64(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Direct response failed err %x", payload.arg2);
        status = status ? status : VAL_ERROR_POINT(9);
    }

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload = val_select_server_fn_direct(test_run_data, 0, 0, 0, 0);
    return status ? status : (uint32_t)payload.arg3;
}

static uint32_t mei_enabled_sp2_setup(ffa_args_t args)
{
    ffa_args_t payload;
    uint32_t status = VAL_SUCCESS;
    ffa_endpoint_id_t sender = args.arg1 & 0xffff;
    ffa_endpoint_id_t receiver = (args.arg1 >> 16) & 0xffff;

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 = FFA_FEATURE_MEI;
    val_ffa_features(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Failed to retrieve MEI err %x", payload.arg2);
        status =  VAL_ERROR_POINT(1);
        goto exit;
    }

    mei_id = (uint32_t)payload.arg2;
    if (val_irq_register_handler(mei_id, mei_irq_handler))
    {
        LOG(ERROR, "MEI interrupt register failed");
        status = VAL_ERROR_POINT(2);
        goto exit;
    }
    LOG(DBG, "MEI Handler Registered, MEI ID %x", mei_id);

    val_secure_intr_enable(mei_id, INTERRUPT_TYPE_FIQ);

    /* Wait for the message. */
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload = val_resp_client_fn_direct((uint32_t)args.arg3, 0, 0, 0, 0, 0);
    if (payload.fid != FFA_MSG_SEND_DIRECT_REQ_64)
    {
        LOG(ERROR, "Direct request failed, fid=0x%x, err 0x%x",
                  payload.fid, payload.arg2);
        status =  VAL_ERROR_POINT(3);
        goto free_interrupt;
    }

    /* Wait for WD interrupt */
    val_sp_sleep(WD_TIME_OUT);

    if (managed_exit_received != true)
    {
        LOG(ERROR, "MEI not triggered");
        status =  VAL_ERROR_POINT(4);
        goto free_interrupt;
    }
    LOG(DBG, "SP Sleep Complete, managed_exit_received %x", managed_exit_received);

    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver;
    val_ffa_msg_send_direct_resp_64(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Direct response failed err %x", payload.arg2);
        status = status ? status : VAL_ERROR_POINT(5);
    }

free_interrupt:
    if (val_irq_unregister_handler(mei_id))
    {
        LOG(ERROR, "IRQ handler unregister failed");
        status = status ? status : VAL_ERROR_POINT(6);
    }

exit:
    val_memset(&payload, 0, sizeof(ffa_args_t));
    payload.arg1 =  ((uint32_t)sender << 16) | receiver;
    val_ffa_msg_send_direct_resp_64(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "Direct response failed err %x", payload.arg2);
        status = status ? status : VAL_ERROR_POINT(7);
    }
    return status;
}

uint32_t vm_to_sp_managed_exit_2_server(ffa_args_t args)
{

    uint32_t status = VAL_SUCCESS;
    ffa_endpoint_id_t curr_ep_logical_id;

    curr_ep_logical_id = val_get_curr_endpoint_logical_id();

    if (curr_ep_logical_id == SP4)
    {
        status = mei_enabled_sp4_setup(args);
    }
    else if (curr_ep_logical_id == SP2)
    {
        status = mei_enabled_sp2_setup(args);
    }

    return status;
}

