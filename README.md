# Embedded C Scheduler
Task scheduler library for embedded (bare metal) projects. It has support for both tick based scheduling, as well as hand coded slot scheduling. It has no direct hardware dependencies, so it is portable to many platforms.

## Installation
This library can be installed into your project using [clib](https://github.com/clibs/clib):

```
$ clib install bradschl/embedded-c-scheduler
```

## Example
```C
#include <project.h>
#include "sched/sched.h"

static uint32_t
get_current_time(void * hint)
{
    (void) hint;

    // Timer is 1us resolution. The hardware counts down from the maximum
    // value, so the raw value needs to be flipped to be compatible with the
    // scheduler
    return Timer_1_INIT_PERIOD - Timer_1_ReadCounter();
}


struct foo_task_ctx {
    /* ... */
}

static void
foo_task(void * hint)
{
    struct foo_task_ctx * ctx = (struct foo_task_ctx *) hint;
    /* Do tasks stuff */
}


int
main()
{
    // Create the scheduler, 1000us (1ms) per tick
    Timer_1_Start();

    struct sched_ctx * scheduler = sched_alloc_context(
        NULL, get_current_time, Timer_1_INIT_PERIOD, 1000);

    ASSERT_NOT_NULL(scheduler);


    // Create the task
    struct foo_task_ctx foo_ctx;

    struct sched_task * foo_task_handle = sched_alloc_task(
        scheduler, foo_ctx, foo_task, "my foo task", TASK_TICK_4);

    ASSERT_NOT_NULL(foo_task_handle);


    for(;;) {
        sched_run(scheduler);

        /* ...  */
    }
}
```

## API
See [sched.h](src/sched/sched.h) for the C API.

## Dependencies and Resources
This library uses heap when allocating structures. After initialization, additional allocations will not be made. This should be fine for an embedded target, since memory fragmentation only happens if memory is freed.

Compiled, this library is only a few kilobytes. Runtime memory footprint is very small, and is dependent on the number of tasks allocated.

## License
MIT license for all files.
