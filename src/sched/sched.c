/**
 * Copyright (c) 2016 Bradley Kim Schleusner < bradschl@gmail.com >
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sched.h"
#include "timermath/timermath.h"
#include "strdup/strdup.h"

// ------------------------------------------------------------ Private settings

#define SHORT_NAME_LENGTH               16


// -------------------------------------------------------------- Private types

struct sched_ctx {
    // The current tick to run when the (now >= last_tick_time + tick_period)
    uint32_t                        current_tick;

    // Time of execution of the last tick
    uint32_t                        last_tick_time;
    uint32_t                        tick_period;
    struct tm_math                  tm;

    // Task linked list root pointer
    struct sched_task *             root;

    // Time function
    sched_get_time_fn               get_time;
    void *                          hint;
};

struct sched_task {
    // Linked list storage
    struct sched_ctx *              ctx;
    struct sched_task *             next;


    // Execution will happen when (tick_mask & current_tick != 0), or it will
    // be executed in the idle loop if (tick_mask == 0)
    uint32_t                        tick_mask;


    // Task execution callback
    sched_task_fn                   execute;
    void *                          hint;


    // Task name
    char                            short_name[SHORT_NAME_LENGTH];
    char *                          long_name;


    // Task stats
    uint32_t                        average_time;
    uint32_t                        max_time;
};


// ---------------------------------------------------------- Private functions

// --------------------- Task functions

static struct sched_task *
get_last_linked_task(struct sched_ctx * ctx)
{
    struct sched_task * last;
    for (last = ctx->root; NULL != last; last = last->next) {
        if (NULL == last->next) {
            break;
        }
    }
    return last;
}


static void
unlink_task(struct sched_task * task)
{
    if (NULL != task) {
        struct sched_ctx * ctx = task->ctx;
        struct sched_task * next = task->next;
        task->ctx = NULL;
        task->next  = NULL;

        struct sched_task * prev = NULL;
        if (NULL != ctx) {
            if (ctx->root == task) {
                ctx->root = next;
            } else {
                for(prev = ctx->root; prev != NULL; prev = prev->next) {
                    if (task == prev->next) {
                        break;
                    }
                }
            }
        }

        if (NULL != prev) {
            prev->next = next;
        }
    }
}

static void
link_task(struct sched_ctx *ctx, struct sched_task * task)
{
    task->ctx = ctx;
    task->next = NULL;

    if (NULL == ctx->root) {
        ctx->root = task;
    } else {
        struct sched_task * last = get_last_linked_task(ctx);
        last->next = task;
    }
}


static void
update_task_stats(struct sched_task * task, int32_t exec_time)
{
    if (exec_time >= 0) {
        uint32_t t = (uint32_t) exec_time;
        task->average_time = (task->average_time + t) >> 1;
        if (t > task->max_time) {
            task->max_time = t;
        }
    }
}


static bool
get_task_info(struct sched_task * task, struct sched_task_info * info)
{
    bool success = false;

    if ((NULL != task) && (NULL != info)) {
        info->_iter = task->next;

        if (NULL != task->long_name) {
            info->name = task->long_name;
        } else {
            info->name = task->short_name;
        }

        info->average_time = task->average_time;
        info->max_time = task->max_time;
        success = true;
    }

    return success;
}


// ---------------- Scheduler functions

static inline uint32_t
rot_left_1(uint32_t a)
{
    return (a << 1) | (a >> ((sizeof(a) * 8) - 1));
}


static void
execute_idle_tasks(struct sched_ctx *ctx)
{
    struct sched_task * task;
    for (task = ctx->root; NULL != task; task = task->next) {
        if (0 == task->tick_mask) {
            uint32_t start = ctx->get_time(ctx->hint);

            task->execute(task->hint);

            uint32_t stop = ctx->get_time(ctx->hint);
            update_task_stats(task, tm_get_diff(&ctx->tm, stop, start));
        }
    }
}


static void
execute_current_tick(struct sched_ctx *ctx)
{
    struct sched_task * task;
    for (task = ctx->root; NULL != task; task = task->next) {
        if(0 != (ctx->current_tick & task->tick_mask)) {
            uint32_t start = ctx->get_time(ctx->hint);

            task->execute(task->hint);

            uint32_t stop = ctx->get_time(ctx->hint);
            update_task_stats(task, tm_get_diff(&ctx->tm, stop, start));
        }
    }
}



// ----------------------------------------------------------- Public functions

// ---------------- Scheduler functions

struct sched_ctx *
sched_alloc_context(void * hint,
                    sched_get_time_fn get_time_fn,
                    uint32_t max_time,
                    uint32_t tick_period)
{
    if ((max_time < 4) || (tick_period < 1) || (tick_period >= (max_time >> 1))) {
        return NULL;
    }

    struct sched_ctx * ctx = (struct sched_ctx *) malloc(sizeof(struct sched_ctx));

    if (NULL != ctx) {
        ctx->current_tick = 0;
        ctx->last_tick_time = 0;
        ctx->tick_period = tick_period;
        tm_initialize(&ctx->tm, max_time);
        ctx->root = NULL;
        ctx->get_time = get_time_fn;
        ctx->hint = hint;
    }

    return ctx;
}


void
sched_free_context(struct sched_ctx * ctx)
{
    if (NULL != ctx) {
        while(NULL != ctx->root) {
            unlink_task(ctx->root);
        }

        free(ctx);
    }
}


void
sched_run(struct sched_ctx * ctx)
{
    bool execute_tick = false;

    uint32_t now = ctx->get_time(ctx->hint);

    if (0 == ctx->current_tick) {
        ctx->current_tick = 0x00000001;
        ctx->last_tick_time = now;
        execute_tick = true;
    } else {
        int32_t delta = tm_get_diff(&ctx->tm, now, ctx->last_tick_time);
        if (delta < 0) {
            ctx->last_tick_time = now;
            execute_tick = true;
        } else if (((uint32_t) delta) >= ctx->tick_period) {
            ctx->last_tick_time =
                tm_offset(&ctx->tm, ctx->last_tick_time, ctx->tick_period);
            execute_tick = true;
        }
    }

    if (execute_tick) {
        execute_current_tick(ctx);
        ctx->current_tick = rot_left_1(ctx->current_tick);
    } else {
        execute_idle_tasks(ctx);
    }
}


void
sched_reset(struct sched_ctx * ctx)
{
    if (NULL != ctx) {
        ctx->current_tick = 0;
    }
}


// --------------------- Task functions

struct sched_task *
sched_alloc_task(struct sched_ctx * ctx,
                 void * hint,
                 sched_task_fn task_fn,
                 const char * name,
                 uint32_t tick_mask)
{
    struct sched_task * task = NULL;
    if (NULL == ctx) {
        goto out;
    }

    task = (struct sched_task *) malloc(sizeof(struct sched_task));
    if (NULL == task) {
        goto out;
    }

    task->ctx = NULL;
    task->next = NULL;

    task->tick_mask = tick_mask;
    task->execute = task_fn;
    task->hint = hint;

    task->average_time = 0;
    task->max_time = 0;

    task->long_name = NULL;
    task->short_name[0] = '\0';

    if (NULL != name) {
        if (strlen(name) < SHORT_NAME_LENGTH) {
            strcpy(task->short_name, name);
        } else {
            task->long_name = strdup(name);
            if(NULL == task->long_name) {
                goto out_name_fail;
            }
        }
    }

    link_task(ctx, task);
    goto out;

    out_name_fail:
        free(task);
        task = NULL;

    out:
        return task;
}


void
sched_free_task(struct sched_task * task)
{
    if (NULL != task) {
        unlink_task(task);

        if (NULL != task->long_name) {
            free(task->long_name);
        }

        free(task);
    }
}

// ------------------------------------------------------------- Task debugging

bool
sched_get_first_task_info(struct sched_ctx * ctx,
                          struct sched_task_info * info)
{
    bool success = false;

    if (NULL != ctx) {
        success = get_task_info(ctx->root, info);
    }

    return success;
}


bool
sched_get_next_task_info(struct sched_task_info * info)
{
    bool success = false;

    if (NULL != info) {
        struct sched_task * next = (struct sched_task *) info->_iter;
        success = get_task_info(next, info);
    }

    return success;
}


void
sched_reset_stats(struct sched_ctx * ctx)
{
    if (NULL != ctx) {
        struct sched_task * task;
        for (task = ctx->root; NULL != task; task = task->next) {
            task->average_time = 0;
            task->max_time = 0;
        }
    }
}
