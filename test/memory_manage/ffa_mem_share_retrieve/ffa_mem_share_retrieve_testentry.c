/*
 * Copyright (c) 2021, Arm Limited or its affliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"

void ffa_mem_share_retrieve_testentry(uint32_t test_num)
{
    /* Execute test for EP combination: client=SP1, server=SP2 */
    if (IS_TEST_FAIL(val_execute_test(test_num, SP1, SP2)))
        return;

    /* Execute test for EP combination: client=VM1, server=SP1 */
    if (IS_TEST_FAIL(val_execute_test(test_num, VM1, SP1)))
        return;

    /* Execute test for EP combination: client=VM1, server=VM2 */
    if (IS_TEST_FAIL(val_execute_test(test_num, VM1, VM2)))
        return;
}
