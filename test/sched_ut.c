
#include "describe/describe.h"
#include "sched/sched.h"


uint32_t
mock_get_time(void * hint)
{
    uint32_t t = 0;
    if(NULL != hint) {
        t = *((uint32_t *) hint);
    }
    return t;
}

void
mock_task(void * hint)
{
    if (NULL != hint) {
        uint32_t * call_count = (uint32_t *) hint;
        ++(*call_count);
    }
}


int main(int argc, char const *argv[])
{
    (void) argv;
    (void) argc;

    uint32_t max_time = 255;
    uint32_t tick_period = 1;

    describe("The scheduler context") {

        struct sched_ctx * ctx = NULL;
        it("can be allocated")  {
            ctx = sched_alloc_context(NULL, mock_get_time, max_time, tick_period);
            assert_not_null(ctx);
        }

        struct sched_task * task = NULL;
        it("can allocate a task") {
            task = sched_alloc_task(ctx, NULL, mock_task, "mock_task", TASK_TICK_IDLE);
            assert_not_null(task);
        }

        it("can free a task") {
            sched_free_task(task);
        }

        it("can be freed") {
            sched_free_context(ctx);
        }

    }

    describe("The scheduler") {
        uint32_t now = 0;

        struct sched_ctx * ctx = NULL;
        it("can allocate a context")  {
            ctx = sched_alloc_context(&now, mock_get_time, max_time, tick_period);
            assert_not_null(ctx);
        }

        uint32_t tick_idle_count = 0;
        struct sched_task * task_idle = NULL;
        it("can allocate the idle task") {
            task_idle = sched_alloc_task(ctx, &tick_idle_count, mock_task, NULL, TASK_TICK_IDLE);
        }


        uint32_t tick_1_count = 0;
        struct sched_task * task_1 = NULL;
        it("can allocate the 1 tick task") {
            task_1 = sched_alloc_task(ctx, &tick_1_count, mock_task, NULL, TASK_TICK_1);
        }


        uint32_t tick_2_count = 0;
        struct sched_task * task_2 = NULL;
        it("can allocate the 2 tick task") {
            task_2 = sched_alloc_task(ctx, &tick_2_count, mock_task, NULL, TASK_TICK_2);
        }

        uint32_t tick_4_count = 0;
        struct sched_task * task_4 = NULL;
        it("can allocate the 4 tick task") {
            task_4 = sched_alloc_task(ctx, &tick_4_count, mock_task, NULL, TASK_TICK_4);
        }

        uint32_t tick_8_count = 0;
        struct sched_task * task_8 = NULL;
        it("can allocate the 8 tick task") {
            task_8 = sched_alloc_task(ctx, &tick_8_count, mock_task, NULL, TASK_TICK_8);
        }

        uint32_t tick_16_count = 0;
        struct sched_task * task_16 = NULL;
        it("can allocate the 16 tick task") {
            task_16 = sched_alloc_task(ctx, &tick_16_count, mock_task, NULL, TASK_TICK_16);
        }

        uint32_t tick_32_count = 0;
        struct sched_task * task_32 = NULL;
        it("can allocate the 32 tick task") {
            task_32 = sched_alloc_task(ctx, &tick_32_count, mock_task, NULL, TASK_TICK_32);
        }

        it("can execute the tasks") {
            assert_equal(0,     tick_idle_count);
            assert_equal(0,     tick_1_count);
            assert_equal(0,     tick_2_count);
            assert_equal(0,     tick_4_count);
            assert_equal(0,     tick_8_count);
            assert_equal(0,     tick_16_count);
            assert_equal(0,     tick_32_count);

            uint32_t i;
            for (i = 0; i < 32; ++i) {
                ++now;
                sched_run(ctx);
            }

            assert_equal(0,     tick_idle_count);
            assert_equal(32,    tick_1_count);
            assert_equal(16,    tick_2_count);
            assert_equal(8,     tick_4_count);
            assert_equal(4,     tick_8_count);
            assert_equal(2,     tick_16_count);
            assert_equal(1,     tick_32_count);
        }

        it("can execute the idle task") {
            assert_equal(0, tick_idle_count);
            sched_run(ctx);
            assert_equal(1, tick_idle_count);
        }

        it("context can be freed before the tasks") {
            sched_free_context(ctx);
        }

        it("can deallocate the tasks") {
            sched_free_task(task_idle);
            sched_free_task(task_1);
            sched_free_task(task_2);
            sched_free_task(task_4);
            sched_free_task(task_8);
            sched_free_task(task_16);
            sched_free_task(task_32);
        }
    }

    return assert_failures();
}

