/* game_run_scene_step [0x02205812]: Calls the currently selected scene-step function from the live game task. */
#include "dh.h"
/* 0x02205812 (scene_run_handler) - the LIVE-PLAY task handler (port unblock #3, the morph END).
 *
 * This is the handler that scene_director_build (0x02205b20) and round_controller_dual
 * (0x02205232) install into the game task (*g_06098678 / *task). Once installed, every frame
 * the task simply tail-calls the currently-selected scene step function pointer g_0609867c
 * (set up by func_022058a8 to the attract/demo stepper, or re-pointed by the scene director to
 * the active game-scene stepper). Installing this handler == "the game scene is now running as
 * the task", which is the state the port needs to reach. 4-instruction thunk; fidelity: EXACT. */

typedef void (*fn_v)(void);

void game_run_scene_step(void) {
    (*(fn_v *)(uintptr_t)0x0609867c)();   /* tail: step the current scene */
}
