/* shc_rotate_left2_signfill [0x0224a504]: Two left rotations followed by OR with -2 in runtime shift sled. */
#include "dh.h"

/*
 * Function: func_0224a504
 * Address: 0x0224a504
 * 
 * Analysis:
 * The assembly shows:
 * 1. rotl r0 (rotate left r0 by 1)
 * 2. rotl r0 (rotate left r0 by 1 again)
 * 3. mov #-2, r1 (load -2 into r1)
 * 4. or r1, r0 (r0 = r0 | -2)
 * 5. mov.l @r15+, r2 (restore r2 from stack, standard epilogue for return)
 * 6. rts (return)
 * 
 * The m2c draft incorrectly assumes r0 is uninitialized. However, the 
 * "rotl r0" instructions at the very start without prior assignment 
 * indicate that r0 is an INPUT parameter to this function.
 * 
 * Logic:
 * - Input: r0 (u32)
 * - Rotate left by 2 bits (equivalent to (val << 2) | (val >> 30))
 * - OR with -2 (0xFFFFFFFE), which sets all bits except bit 0 to 1.
 * - Return result in r0.
 */

u32 shc_rotate_left2_signfill(u32 r0) {
    u32 temp;

    // rotl r0 (first time)
    temp = (r0 << 1) | (r0 >> 31);
    
    // rotl r0 (second time)
    temp = (temp << 1) | (temp >> 31);
    
    // or r1, r0 where r1 is -2
    temp = temp | (u32)-2;
    
    return temp;
}

/* fidelity: High - Correctly identified r0 as input parameter based on immediate read, implemented double rotate and OR with -2 */
