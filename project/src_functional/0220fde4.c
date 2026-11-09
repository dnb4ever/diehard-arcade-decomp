/* task_run_frame [0x0220fde4]: Walks handlers, reclaims dead nodes and their work sublists, and publishes the active count. */
#include "dh.h"
/* 0x0220fde4 (task_list_run) - per-frame executive: walk the active-task list held
 * in the control block g_06092238 (freelist @0, prev-iterator @4, current @8,
 * embedded sentinel node @0xC). For each node: if it has a handler (@0) call
 * handler(node) and count it; otherwise the node is dead - free each node of its
 * sub-list (@8, chained via @0) through func_02214c8a, unlink it, and push it onto
 * the freelist. Stores the active count to g_06096244.
 * fidelity: HIGH - all mov.l (pointer/word); sub-list nodes reuse field @0 as their
 * next link (a union in the original). */
typedef struct TaskNode {
    void (*handler)(struct TaskNode *);   /* +0 (also sub-list next link) */
    struct TaskNode *next;                /* +4 */
    struct TaskNode *sublist;             /* +8 */
} TaskNode;

typedef struct {
    TaskNode *freelist;   /* +0  */
    TaskNode *iter;       /* +4  (prev pointer) */
    TaskNode *cur;        /* +8  */
    TaskNode  head;       /* +0C embedded sentinel */
} TaskListCtl;

extern TaskListCtl g_task_list_control;
extern void        heap_free_coalesce(void *node);   /* free a task node */
extern s32         g_active_task_count;                 /* active task count */

void task_run_frame(void) {
    s32 count = 0;
    TaskNode *node = &g_task_list_control.head;

    g_task_list_control.iter = node;
    for (;;) {
        node = node->next;
        if (node == (TaskNode *)0)
            break;
        if (node->handler != (void (*)(TaskNode *))0) {
            g_task_list_control.cur = node;
            node->handler(node);
            count++;
            g_task_list_control.iter = node;
        } else {
            TaskNode *sub = node->sublist;
            while (sub != (TaskNode *)0) {
                TaskNode *nextsub = *(TaskNode **)sub;   /* field @0 = next */
                heap_free_coalesce(sub);
                sub = nextsub;
            }
            g_task_list_control.iter->next = node->next;   /* unlink */
            node->next = g_task_list_control.freelist;     /* push to freelist */
            g_task_list_control.freelist = node;
            node = g_task_list_control.iter;               /* resume from prev */
        }
    }
    g_active_task_count = count;
}
