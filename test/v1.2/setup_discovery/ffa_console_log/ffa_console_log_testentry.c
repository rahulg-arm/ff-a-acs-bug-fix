/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"

void ffa_console_log_testentry(uint32_t test_num)
{
    /* Execute test for EP combination: client=SP1, server=NONE */
    if (IS_TEST_FAIL(val_execute_test(test_num, SP1, NO_SERVER_EP)))
        return;

    /* Execute test for EP combination: client=SP2, server=NONE */
    if (IS_TEST_FAIL(val_execute_test(test_num, SP2, NO_SERVER_EP)))
        return;

    /* Execute test for EP combination: client=SP3, server=NONE */
    if (IS_TEST_FAIL(val_execute_test(test_num, SP3, NO_SERVER_EP)))
        return;

    /* Execute test for EP combination: client=SP4, server=NONE */
    if (IS_TEST_FAIL(val_execute_test(test_num, SP4, NO_SERVER_EP)))
        return;
}
