/*
 * Copyright (c) 2021, Arm Limited or its affliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "val_exceptions.h"

static void (*irq_callback)(void);
static bool (*exception_callback)(void);

/**
 * Handles an IRQ at the current exception level.
 *
 * Returns false so that the value of elr_el1 is restored from the stack, in
 * case there are nested exceptions.
 */
bool val_irq_current(void)
{
    if (irq_callback != NULL)
    {
        irq_callback();
    }
    else
    {
        LOG(ERROR, "Got unexpected interrupt.\n", 0, 0);
    }

    return false;
}

static bool default_sync_current_exception(void)
{
    uint64_t esr = val_esr_el1_read();
    uint64_t elr = val_elr_el1_read();
    uint64_t ec = esr >> 26;

    switch (ec)
    {
        case EC_DATA_ABORT_SAME_EL:
            LOG(ERROR, "Data abort: pc=%x, esr=%x", elr, esr);
            LOG(ERROR, ", ec=%x", ec, 0);

            if (!(esr & (1U << 10)))
            { /* Check FnV bit. */
                LOG(ERROR, ", far=%x\n", val_far_el1_read(), 0);
            }
            else
            {
                LOG(ERROR, ", far=invalid\n", 0, 0);
            }

            break;

        default:
            LOG(ERROR, "Unknown sync exception pc=%x, esr=%x",
                 elr, esr);
            LOG(ERROR, ", ec=%x\n", ec, 0);
    }

    for (;;)
    {
        /* do nothing */
    }
    return false;
}

/**
 * Handles a synchronous exception at the current exception level.
 *
 * Returns true if the value of elr_el1 should be kept as-is rather than
 * restored from the stack. This enables exception handlers to indicate whether
 * they have changed the value of elr_el1 (e.g., to skip the faulting
 * instruction).
 */
bool val_sync_exception_current(void)
{
    if (exception_callback != NULL)
    {
        return exception_callback();
    }
    return default_sync_current_exception();
}

void val_exception_setup(void (*irq)(void), bool (*exception)(void))
{
    irq_callback = irq;
    exception_callback = exception;
}
