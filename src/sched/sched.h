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

#ifndef SCHED_H_
#define SCHED_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// ---------------------------------------------------------- Scheduler context


// ------- Internal scheduler structure
struct sched_ctx;


/**
 * @brief Function pointer prototype for getting the current time
 * @details The returned time must be an incrementing type, starting at 0,
 *          incrementing to its maximum value, and then rolling back to 0.
 *          The exact units of time do not matter to the scheduler. It is
 *          recommended that the time function is at least faster than the task
 *          tick rate such that task execution time can be measured with
 *          meaningful resolution.
 *          If the implementation references a count down time source, then it
 *          should invert the time source value, i.e.
 *              return (TIMER_MAX_VALUE - timer_value);
 *
 * @param hint Optional hint parameter. This pointer may be used for whatever
 *          the implementation wants, such as a this pointer. It can be set to
 *          NULL if it isn't used.
 * @return Current time
 */
typedef uint32_t (*sched_get_time_fn)(void * hint);


/**
 * @brief Allocates the scheduler context on the heap
 *
 * @param hint Optional hint parameter for the get_time_fn function. The
 *          get_time_fn will always be called with this parameter. If unused,
 *          set to NULL
 * @param get_time_fn Get time function pointer. This will be used by the
 *          scheduler to time task execution, as well as to determine when to
 *          run the next task tick
 * @param max_time Maximum value that the get_time_fn will return
 * @param tick_period Number of counts returned by the get_time_fn per task
 *          tick
 * @return Scheduler context, or NULL on failure
 */
struct sched_ctx *
sched_alloc_context(void * hint,
                    sched_get_time_fn get_time_fn,
                    uint32_t max_time,
                    uint32_t tick_period);


/**
 * @brief Deallocates a scheduler
 * @details This will free all resources used by the scheduler an unregister
 *          all tasks.
 *
 * @param sched_ctx Scheduler context to free
 */
void
sched_free_context(struct sched_ctx * ctx);


/**
 * @brief Periodic call to drive the scheduler
 * @details This needs to be called continuously to drive task execution. This
 *          will return to allow for top level code outside of the scheduler to
 *          run. To run the scheduler forever, use the sched_run_noret wrapper.
 *          Example:
 *          for (;;) {
 *              sched_run(sched_ctx);
 *              if (can_sleep) {
 *                  enter_lpm();
 *                  sched_reset(sched_ctx);
 *              }
 *          }
 *
 * @param sched_ctx Scheduler context
 */
void
sched_run(struct sched_ctx * ctx);


/**
 * @brief Executes the scheduler and never returns
 *
 * @param sched_ctx Scheduler context
 */
static inline void
sched_run_noret(struct sched_ctx * ctx)
{
    for(;;) {
        sched_run(ctx);
    }
}


/**
 * @brief Resets the current task tick and next tick time
 * @details This is useful for reseting the scheduler after coming out of a
 *          sleep mode. This will allow tasks to execute right.
 *
 * @param sched_ctx Scheduler context
 */
void
sched_reset(struct sched_ctx * ctx);


// ---------------------------------------------------------------------- Tasks

// ------------ Internal task structure
struct sched_task;


/**
 * @brief Tasks function pointer prototype
 *
 * @param hint Optional hint parameter. Implementation may use this for
 *          whatever the implementation wants, such as a this pointer
 */
typedef void (*sched_task_fn)(void * hint);


/**
 * @brief Allocates a new task on the heap and registers it with the scheduler
 *
 * @param sched_ctx Scheduler context to register with
 * @param hint Optional hint parameter for the task_fn function. This will be
 *          passed to the task_fn function every time it is called. If not
 *          used, set to NULL
 * @param task_fn Task function callback
 * @param name Human readable name for the task. If unused, set to NULL. If not
 *          NULL, then the string will be copied into the returned task handle
 * @param tick_mask Tick mask to execute the task on. The scheduler implements
 *          ticks as a 32 bit number with only one bit set. The scheduler
 *          increments the tick by rotating the tick number right by one. If
 *          (tick_mask & tick) is true, then the task is executed. If the tick
 *          mask is 0, then the task is executed as an idle task.
 *          Users may use this tick strategy to hard code exact tick slots
 *          that tasks execute in. To use the scheduler as a "standard" power
 *          of 2 type scheduler, use the "Standard task tick masks" included in
 *          this header
 * @return Scheduler tasks handle or NULL on failure
 */
struct sched_task *
sched_alloc_task(struct sched_ctx * ctx,
                 void * hint,
                 sched_task_fn task_fn,
                 const char * name,
                 uint32_t tick_mask);


/**
 * @brief Deallocates a task handle
 * @details This will unregister the task from the scheduler and free its
 *          resources
 *
 * @param sched_task Scheduler task handle
 */
void
sched_free_task(struct sched_task * task);


// --------------------------------------------------- Standard task tick masks

#define TASK_TICK_IDLE                  0b00000000000000000000000000000000
#define TASK_TICK_1                     0b11111111111111111111111111111111
#define TASK_TICK_2                     0b10101010101010101010101010101010
#define TASK_TICK_4                     0b01000100010001000100010001000100
#define TASK_TICK_8                     0b00010000000100000001000000010000
#define TASK_TICK_16                    0b00000001000000000000000100000000
#define TASK_TICK_32                    0b00000000000000010000000000000000


// ------------------------------------------------------------- Task debugging

/**
 * The task debug functions may be used to pull run time stats about
 * each task registered with the scheduler. This can be used to figure out if
 * tasks are overflowing their tick.
 *
 * bool new_info;
 * struct sched_task_info info;
 * for (new_info = sched_get_first_task_info(sched_ctx, &info);
 *      false != new_info;
 *      new_info = sched_get_next_task_info(&info))
 * {
 *     // Print out tasks stats to debug console
 * }
 *
 * sched_reset_stats(sched_ctx);
 */
struct sched_task_info {
    // Pointer to the next task. For internal use only
    void * _iter;

    // Task name string
    const char * name;

    // Average execution time of the task. Units are implementation specific
    // and will be the same resolution as the get_time_fn function
    uint32_t average_time;

    // Maximum execution time of the task, same units as average_time
    uint32_t max_time;
};


/**
 * @brief Initializes a task iterator for the first task in the scheduler
 *
 * @param sched_ctx Scheduler context
 * @param sched_task_info Task info structure to initialize
 * @return true on success, else false
 */
bool
sched_get_first_task_info(struct sched_ctx * ctx,
                          struct sched_task_info * info);


/**
 * @brief Updates the task info structure to the next task
 * @details This follows the iterator embedded in the task info structure to
 *          get at the next tasks information. See the section example for
 *          detailed usage
 *
 * @param sched_task_info Task info structure to update with the next tasks
 *          info
 * @return true on success, else false
 */
bool
sched_get_next_task_info(struct sched_task_info * info);


/**
 * @brief Resets all task timing statistics
 *
 * @param sched_ctx Scheduler context
 * @return true on success, else false
 */
void
sched_reset_stats(struct sched_ctx * ctx);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SCHED_H_ */
