/* results_tick_continue_countdown [0x02209814]: State-3 countdown, input and sound paths in results dispatcher family. */
#include "dh_work.h"
#include "dh.h"

/* 0x02209814 - func_02209814: results task state 3 (continue countdown).
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern s32   g_060997AC;                                 /* cursor font    */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern void (*g_060902A0)(s32 fmt, ...);                 /* ui_printf thunk */
extern s32   g_sound_effects_enabled_legacy;      /* sound-enable flag */
extern void sound_queue_push_entry(u32 cmd);             /* 0x215f6c sound_queue_push */
extern void audio_send_command_buffer(void *cmd, s32 len);  /* 0x215e20 sound_send_command */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))
#define RS16(a) (*(s16  *)(uintptr_t)(a))

/* ========================= state 3 (0x209814) ========================= */
/* "Continue?" countdown screen.  ent->unk4C = frame counter (0..0xBC, wraps 0),
 * ent->unk48 = selected voice/line index.  Per-frame actions drive a voice clip
 * (func_02215e20) and sound cues (func_02215f6c). */
void results_tick_continue_countdown(void *task)
{
    void *ent = TASK_PTR(task, work);

    results_draw_header(TASK_S8(task, state), ent);

    g_ui_cursor_column = 11;
    g_ui_cursor_row = 6;
    g_0609029C(RMP(0x0609053C));

    if (RU16(0x06098794 + 6) & 0x0408) {
        /* --- countdown running --- */
        s32 c = M32(ent, 0x4C);
        M32(ent, 0x4C) = (c < 0xBD) ? c + 1 : 0;

        /* per-frame voice/line selector (0x2098a0..0x209984): map the (just-
         * updated) frame counter ent->unk4C to a voice-line index ent->unk48 and
         * play that clip from the voice-id table g_0605136C[index].  Frame 0 arms
         * "no line" (unk48 = -1, no clip); counter values not listed leave unk48
         * unchanged and play nothing.  Frame->index decoded from the cmp/eq chain
         * and branch targets 0x2098f4..0x209984. */
        {
            s32 vidx = -2;                       /* -2 => no action this frame */
            switch (M32(ent, 0x4C)) {
            case 0:    M32(ent, 0x48) = -1; break;   /* clear line, no clip */
            case 1:    case 16:    vidx = 0; break;
            case 5:    case 112:   vidx = 1; break;
            case 7:    case 0x80:  vidx = 2; break;
            case 9:    case 0x87:  vidx = 3; break;
            case 13:   case 0x91:  vidx = 4; break;
            case 14:   case 0x9B:  vidx = 5; break;
            case 15:   case 0xAB:  vidx = 6; break;
            case 0xB4: case 0xBF:  vidx = 7; break;
            default:   break;
            }
            if (vidx >= 0) {
                M32(ent, 0x48) = vidx;
                audio_send_command_buffer(RMP(0x0605136C + vidx * 4), 0x6C000);
            }
        }
        sound_queue_push_entry(DH_SOUND_SCENE_END_COMMAND);
        if (M32(ent, 0x4C) != 0 && g_sound_effects_enabled_legacy != 0)
            sound_queue_push_entry((u32)((RS16(0x06053F04 + M32(ent, 0x4C) * 2) << 8) | DH_SOUND_CUE_BASE));
    } else if (RU16(0x06098794 + 6) & 0x0204) {
        /* --- player chose "yes": restart --- */
        sound_queue_push_entry(DH_SOUND_SCENE_END_COMMAND);
        TASK_S8(task, state) = 1;
        M32(ent, 0x40) = 3;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 6;
    g_ui_cursor_column = 0x12;
    g_ui_cursor_row = 6;
    g_060902A0(0x06051434, M32(ent, 0x4C));     /* countdown number */
    if (M32(ent, 0x4C) != 0) {
        g_ui_cursor_column = 0x17; g_ui_cursor_row = 6;
        g_060902A0(0x06051438, M32(ent, 0x48));
    } else {
        g_ui_cursor_column = 0x17; g_ui_cursor_row = 6;
        g_060902A0(0x0605143C);
    }
    g_ui_cursor_column = 0x19;
    g_ui_cursor_row = 6;
    g_060902A0(0x06051440, RS16(0x06053F04 + M32(ent, 0x4C) * 2));
    g_060997AC = 1;
}
/* fidelity: HIGH - header, event branches, countdown wrap, the per-frame voice
 * selector (frame->index map), and the printf tail all hand-traced vs fn.py
 * disassembly (0x209814..0x209a54). */
