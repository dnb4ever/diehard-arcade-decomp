/* Recovered guest layouts. See notes/readability_pass2.md and notes/readability_pass3.md for scope.
 * Guest pointer words are uint32_t, never native host pointers. Unknown bytes
 * remain reserved. Do not apply the actor tail to camera/effect/hitbox work.
 */
#ifndef DH_WORK_H
#define DH_WORK_H
#include <stdint.h>

/* 0x20-byte task image: constructors 0220fc3c/0220fc82/0220fcc4.
 * +8 is work in handlers, and the allocation chain in 02214b70/0220fde4.
 * Pointer accessors below deliberately retain the existing MP host-width model.
 */
typedef struct DhTaskRecord {
    uint32_t handler;                 /* 00: callback, zero means cancelled */
    uint32_t next;                    /* 04: linked task */
    uint32_t work;                    /* 08: handler work / owned allocation chain */
    uint8_t reserved_0c[2];
    int8_t depth;                     /* 0e: parent depth + 1 */
    uint8_t reserved_0f;
    int8_t state;                     /* 10: handler-local state index */
    uint8_t reserved_11[3];
    uint32_t parent;                  /* 14: owning parent task */
    uint8_t reserved_18[8];
} DhTaskRecord;

/* Shared world-pose prefix only; camera and actor tails have different layouts.
 * Position proof: 0220d262. Angle proof: 0220b1c0, 02227704, 022225f6.
 */
typedef struct DhWorldPose {
    uint8_t reserved_00[8];
    int32_t position_x;                /* 08: 16.16 world X */
    int32_t position_y;                /* 0c: 16.16 height */
    int32_t position_z;                /* 10: 16.16 world Z */
    uint8_t reserved_14[16];
    int16_t angle_x;                   /* 24 */
    int16_t angle_y;                   /* 26 */
    int16_t angle_z;                   /* 28 */
} DhWorldPose;

/* 0x18c-byte actor allocation (0220ff0a, 02227bc4).
 * Movement accumulators: 022170ea integrates +14/+18/+1c then clears them.
 * Animation fields: 0222dc2c; category/state: 02223ee0; enemy state: 02244e2e.
 * Signed members match M8/M16/M32 sites; unsigned reads retain MU accessors.
 */
typedef struct DhActorWork {
    uint8_t reserved_00[8];
    int32_t position_x;                     /* 08 */
    int32_t position_y;                     /* 0c */
    int32_t position_z;                     /* 10 */
    int32_t movement_x;                     /* 14 */
    int32_t movement_y;                     /* 18 */
    int32_t movement_z;                     /* 1c */
    uint8_t reserved_20[4];
    int16_t angle_x;                        /* 24 */
    int16_t angle_y;                        /* 26 */
    int16_t angle_z;                        /* 28 */
    uint8_t reserved_2a[10];
    int8_t slot_id;                        /* 34 */
    uint8_t reserved_35[3];
    int32_t render_flags;                   /* 38 */
    uint8_t reserved_3c[4];
    int8_t interpolation_ticks;            /* 40 */
    int8_t animation_control;              /* 41 */
    uint8_t reserved_42[1];
    int8_t state_handler;                  /* 43 */
    int8_t facing_right;                   /* 44 */
    uint8_t reserved_45[3];
    int8_t action_pending; /* 48: pass 4, see layout_evidence.json */
    uint8_t reserved_49[1];
    int8_t hit_source_slot; /* 4a: pass 4, see layout_evidence.json */
    int8_t paired_slot; /* 4b: pass 4, see layout_evidence.json */
    int8_t hit_pending;                    /* 4c */
    int8_t direction_code;                 /* 4d */
    int8_t hit_shake_ticks;                /* 4e */
    int8_t model_variant;                  /* 4f */
    uint8_t reserved_50[2];
    int16_t interaction_latch; /* 52: pass 4, see layout_evidence.json */
    int16_t animation_events;               /* 54 */
    int16_t health; /* 56: pass 4, see layout_evidence.json */
    int16_t animation_frame;                /* 58 */
    int16_t animation_current;              /* 5a */
    int16_t animation_requested;            /* 5c */
    int16_t model_index;                    /* 5e */
    int16_t motion_pitch;                   /* 60 */
    int16_t motion_heading;                 /* 62 */
    uint8_t reserved_64[6];
    int8_t saved_primary_effect_type; /* 6a: pass 4, see layout_evidence.json */
    int8_t saved_primary_effect_slot; /* 6b: pass 4, see layout_evidence.json */
    int8_t primary_effect_type;            /* 6c */
    int8_t secondary_effect_type;          /* 6d */
    int8_t primary_effect_slot;            /* 6e */
    int8_t secondary_effect_slot;          /* 6f */
    int8_t primary_handoff_pending; /* 70: pass 4, see layout_evidence.json */
    int8_t primary_handoff_slot; /* 71: pass 4, see layout_evidence.json */
    uint8_t reserved_72[2];
    int32_t motion_speed; /* 74: pass 4, see layout_evidence.json */
    uint8_t reserved_78[12];
    int32_t action_category;                /* 84 */
    int32_t action_state;                   /* 88 */
    int32_t interaction_flags;              /* 8c */
    int32_t action_ticks;                   /* 90 */
    int32_t selected_effect_slot; /* 94: pass 4, see layout_evidence.json */
    uint8_t reserved_98[34];
    int16_t type_flags; /* ba: pass 4, see layout_evidence.json */
    uint8_t reserved_bc[20];
    uint32_t animation_script; /* d0: pass 4, see layout_evidence.json */
    int32_t behavior_state;                 /* d4 */
    int32_t behavior_phase;                 /* d8 */
    uint8_t reserved_dc[4];
    int32_t state_flags;                    /* e0 */
    int32_t state_index;                    /* e4 */
    uint8_t reserved_e8[24];
    int32_t queued_attack_kind; /* 100: pass 4, see layout_evidence.json */
    int32_t queued_attack_slot; /* 104: pass 4, see layout_evidence.json */
    int32_t action_phase;                   /* 108 */
    int32_t event_count;                    /* 10c */
    int32_t event_tick;                     /* 110 */
    int32_t action_start_tick; /* 114: pass 4, see layout_evidence.json */
    uint8_t reserved_118[8];
    int32_t target_slot;                    /* 120 */
    uint8_t reserved_124[4];
    int32_t player_target_slot;             /* 128 */
    int32_t effect_target_slot;             /* 12c */
    int32_t target_offset_x;                /* 130 */
    int32_t target_offset_z;                /* 134 */
    uint8_t reserved_138[16];
    int16_t idle_animation;                 /* 148 */
    int16_t approach_animation;             /* 14a */
    int16_t state2_animation;               /* 14c */
    int16_t state8_animation;               /* 14e */
    uint8_t reserved_150[4];
    uint32_t direction_animation_table; /* 154: pass 4, see layout_evidence.json */
    uint32_t action_config_tables; /* 158: pass 4, see layout_evidence.json */
    uint32_t reaction_config_tables; /* 15c: pass 4, see layout_evidence.json */
    uint8_t reserved_160[4];
    uint32_t response_config_tables; /* 164: pass 4, see layout_evidence.json */
    uint32_t distance_attack_table; /* 168: pass 4, see layout_evidence.json */
    uint32_t motion_action_tables; /* 16c: pass 4, see layout_evidence.json */
    uint8_t reserved_170[16];
    uint32_t reaction_descriptor; /* 180: pass 4, see layout_evidence.json */
    uint32_t state_animation_descriptor; /* 184: pass 4, see layout_evidence.json */
    uint32_t variant_animation_table; /* 188: pass 4, see layout_evidence.json */
} DhActorWork;

/* Five records, stride 0x18, starting at g_player_input_records (06098734).
 * Every named field is established by input_poll_records (0220f9d4).
 * +8/+a/+14 remain unknown; some consumers also copy timer words to UI output.
 */
typedef struct DhPlayerInput {
    int16_t buttons;                   /* 00: raw OR injected */
    int16_t injected_buttons;          /* 02 */
    int16_t pressed;                   /* 04: ~previous & current */
    int16_t released;                  /* 06: ~current & previous */
    uint8_t reserved_08[4];
    int16_t direction_hold_frames;     /* 0c: same latched direction */
    int16_t direction_change_frames;   /* 0e: resets on any direction change */
    int16_t button1_hold_frames;       /* 10: mask 1 */
    int16_t button2_hold_frames;       /* 12: mask 2 */
    uint8_t reserved_14[2];
    int16_t latched_direction;         /* 16 */
} DhPlayerInput;

/* Two records, stride 6, at 060987ac. 0220fb98 proves phase/counter/fired.
 * +0 remains unknown, even though input_reset_records clears it.
 */
typedef struct DhDirectionTap {
    uint8_t reserved_00[2];
    uint16_t phase;                    /* 02: four-phase release/press detector */
    int8_t hold_frames;                /* 04: reset after two held frames */
    int8_t fired;                      /* 05: one-update pulse */
} DhDirectionTap;

#define ACTOR_WORK(p) ((DhActorWork *)(uintptr_t)(p))
#define WORLD_POSE(p) ((DhWorldPose *)(uintptr_t)(p))
#define PLAYER_INPUT(p) ((DhPlayerInput *)(uintptr_t)(p))
#define DIRECTION_TAP(p) ((DhDirectionTap *)(uintptr_t)(p))

/* Keep exact old pointer widths and signed/unsigned variants. Offsets are
 * checked against DhTaskRecord by work/readability2/check_layouts.c.
 */
#define TASK_PTR(p, field) MP(p, DH_TASK_##field##_OFFSET)
#define TASK_S32(p, field) M32(p, DH_TASK_##field##_OFFSET)
#define TASK_S8(p, field) M8(p, DH_TASK_##field##_OFFSET)
#define TASK_U8(p, field) MU8(p, DH_TASK_##field##_OFFSET)
#define DH_TASK_handler_OFFSET 0x00
#define DH_TASK_next_OFFSET    0x04
#define DH_TASK_work_OFFSET    0x08
#define DH_TASK_depth_OFFSET   0x0e
#define DH_TASK_state_OFFSET   0x10
#define DH_TASK_parent_OFFSET  0x14

/* Unsigned reads of actor words cannot use the signed struct member directly. */
#define ACTOR_U16(p, field) MU16(p, DH_ACTOR_##field##_OFFSET)
#define DH_ACTOR_angle_x_OFFSET             0x24
#define DH_ACTOR_angle_y_OFFSET             0x26
#define DH_ACTOR_angle_z_OFFSET             0x28
#define DH_ACTOR_animation_events_OFFSET    0x54
#define DH_ACTOR_animation_frame_OFFSET     0x58
#define DH_ACTOR_animation_current_OFFSET   0x5a
#define DH_ACTOR_animation_requested_OFFSET 0x5c


/* Camera prefix through +c9, padded to 0xcc; this is not an allocation-size claim.
 * Init: 0220b1c0/0220bc20. Targets/easing: 0220b8d4/0220e0aa.
 * Orbit focus/offset: 0220cdf6/0220cfac. +ae is a slot in orbit handlers,
 * a countdown in definition stepping; those alternative roles remain reserved. */
typedef struct DhCameraWork {
    uint8_t reserved_00[8];
    int32_t position_x;                     /* 08 */
    int32_t position_y;                     /* 0c */
    int32_t position_z;                     /* 10 */
    uint8_t reserved_14[16];
    int16_t angle_x;                        /* 24 */
    int16_t angle_y;                        /* 26 */
    int16_t angle_z;                        /* 28 */
    uint8_t reserved_2a[58];
    uint32_t update_handler; /* 64: pass 4, see layout_evidence.json */
    int32_t horizontal_range; /* 68: pass 4, see layout_evidence.json */
    uint8_t reserved_6c[24];
    int32_t stage_origin_x;                 /* 84 */
    int32_t stage_origin_y;                 /* 88 */
    int32_t stage_origin_z;                 /* 8c */
    int32_t focus_x;                        /* 90 */
    int32_t focus_y;                        /* 94 */
    int32_t focus_z;                        /* 98 */
    int32_t orbit_offset_x;                 /* 9c */
    int32_t orbit_offset_y;                 /* a0 */
    int32_t orbit_offset_z;                 /* a4 */
    int16_t request_hold_ticks;             /* a8 */
    int16_t shake_ticks;                    /* aa */
    uint8_t reserved_ac[12];
    int32_t target_x;                       /* b8 */
    int32_t target_y;                       /* bc */
    int32_t target_z;                       /* c0 */
    int16_t target_angle_x;                 /* c4 */
    int16_t target_angle_y;                 /* c6 */
    int16_t target_angle_z;                 /* c8 */
    uint8_t reserved_ca[2];
} DhCameraWork;

/* Scene table stride 0x50: 02205b20; resource users 02202138/02217d6c/02217f04.
 * +28 word and +29 byte intentionally overlap: the original guest bytes are
 * accessed independently. No host-endian interpretation of the union is used.
 * The unconverted task/wave/transition tail awaits more independent users.
 */
typedef struct DhSceneDescriptor {
    uint32_t texture_groups;          /* 00: texture upload list */
    uint32_t pattern_groups;          /* 04: pattern upload list */
    uint32_t model_list;              /* 08: model ID list */
    uint32_t sound_resource;          /* 0c: optional sound transfer source */
    uint32_t camera_definition;       /* 10: default camera step definition */
    uint8_t reserved_14[12];
    int32_t stage_index;              /* 20: 0x2c-byte stage record index */
    int32_t view_radius;              /* 24: visible grid radius */
    union {
        int16_t pose_index;          /* 28: position/bootstrap table selector */
        struct {
            uint8_t reserved_28;
            int8_t angle_variant;    /* 29: angle table selector */
        } bytes;
    } pose;
    uint8_t reserved_2a[38];
} DhSceneDescriptor;

/* Source descriptor copied by 02245c88 into actor+148 and later requested by
 * enemy_reset_idle_or_walk, enemy_begin_approach, enemy_enter_state2/state8.
 * Only these four animation words have independently agreed uses. */
typedef struct DhEnemyActionDescriptor {
    int16_t idle_animation;           /* 00 -> actor+148 */
    int16_t approach_animation;       /* 02 -> actor+14a */
    int16_t state2_animation;         /* 04 -> actor+14c */
    int16_t state8_animation;         /* 06 -> actor+14e */
    uint8_t reserved_08[60];
} DhEnemyActionDescriptor;

/* Software command storage is 0x24, including a depth key beyond the 0x20
 * VDP1 hardware packet. Builders: 022153f6/022154fc/022156da/0221582a.
 * Packed vertex words retain their original 32-bit access and byte order.
 * Clip headers also use individual 16-bit coordinates (02215000).
 */
typedef struct DhVdp1CommandEntry {
    int16_t control;                  /* 00 */
    uint8_t reserved_02[2];
    int16_t draw_mode;                /* 04 */
    int16_t colour;                   /* 06 */
    int16_t character_address;        /* 08 */
    int16_t character_size;           /* 0a */
    int32_t vertex_a;                 /* 0c: packed coordinate pair */
    int32_t vertex_b;                 /* 10 */
    int32_t vertex_c;                 /* 14 */
    int32_t vertex_d;                 /* 18 */
    uint8_t reserved_1c[4];
    int32_t sort_key;                 /* 20: software-only */
} DhVdp1CommandEntry;

/* Deferred record (0x30 stride): both sprite command builders agree on these
 * fields. +20 == -1 terminates the list; this is not a hardware VDP1 packet. */
typedef struct DhSpriteDrawRecord {
    int32_t vertex_a;                 /* 00 */
    int32_t vertex_b;                 /* 04 */
    int32_t vertex_c;                 /* 08 */
    int32_t vertex_d;                 /* 0c */
    uint8_t reserved_10[16];
    int32_t depth;                    /* 20 */
    uint8_t reserved_24[4];
    uint32_t descriptor;              /* 28: guest pointer */
    int16_t mode_bits;                /* 2c */
    uint8_t reserved_2e[2];
} DhSpriteDrawRecord;

/* 0x10-byte character records; sprite_draw_child/billboard and both deferred
 * builders agree on address/size/mode. Width/height agree in both immediate sprite builders. */
typedef struct DhSpriteCharacter {
    int16_t character_address;        /* 00 */
    int16_t character_size;           /* 02 */
    int16_t width;                    /* 04 */
    int16_t height;                   /* 06 */
    int16_t draw_mode;                /* 08 */
    uint8_t reserved_0a[6];
} DhSpriteCharacter;

/* Ring at 060e0de8: reset 02215f24, push 02215f6c; consumer 02215f36
 * remains absent from the C tree. The read-index role retains MED confidence.
 * Commands are whole words; no speculative byte field or host endian split. */
typedef struct DhSoundCommandQueue {
    int32_t read_index;               /* 00 */
    int32_t write_index;              /* 04 */
    int32_t commands[16];             /* 08 */
} DhSoundCommandQueue;

/* Named fields with original access width, qualifiers and pointer model.
 * Every offset is independently asserted in work/readability3/check_layouts.c. */
#define ACTOR_M16(p, field) M16(p, DH_ACTOR_##field##_OFFSET)
#define ACTOR_M32(p, field) M32(p, DH_ACTOR_##field##_OFFSET)
#define ACTOR_M8(p, field) M8(p, DH_ACTOR_##field##_OFFSET)
#define ACTOR_MU16(p, field) MU16(p, DH_ACTOR_##field##_OFFSET)
#define ACTOR_MU32(p, field) MU32(p, DH_ACTOR_##field##_OFFSET)
#define ACTOR_MU8(p, field) MU8(p, DH_ACTOR_##field##_OFFSET)
#define DH_ACTOR_render_flags_OFFSET 0x38
#define DH_ACTOR_hit_pending_OFFSET 0x4c
#define DH_ACTOR_direction_code_OFFSET 0x4d
#define DH_ACTOR_model_variant_OFFSET 0x4f
#define DH_ACTOR_model_index_OFFSET 0x5e
#define DH_ACTOR_motion_pitch_OFFSET 0x60
#define DH_ACTOR_motion_heading_OFFSET 0x62
#define DH_ACTOR_primary_effect_type_OFFSET 0x6c
#define DH_ACTOR_secondary_effect_type_OFFSET 0x6d
#define DH_ACTOR_primary_effect_slot_OFFSET 0x6e
#define DH_ACTOR_secondary_effect_slot_OFFSET 0x6f
#define DH_ACTOR_interaction_flags_OFFSET 0x8c
#define DH_ACTOR_action_ticks_OFFSET 0x90
#define DH_ACTOR_behavior_state_OFFSET 0xd4
#define DH_ACTOR_behavior_phase_OFFSET 0xd8
#define DH_ACTOR_action_phase_OFFSET 0x108
#define DH_ACTOR_event_count_OFFSET 0x10c
#define DH_ACTOR_event_tick_OFFSET 0x110
#define DH_ACTOR_target_slot_OFFSET 0x120
#define DH_ACTOR_player_target_slot_OFFSET 0x128
#define DH_ACTOR_effect_target_slot_OFFSET 0x12c
#define DH_ACTOR_target_offset_x_OFFSET 0x130
#define DH_ACTOR_target_offset_z_OFFSET 0x134
#define DH_ACTOR_idle_animation_OFFSET 0x148
#define DH_ACTOR_approach_animation_OFFSET 0x14a
#define DH_ACTOR_state2_animation_OFFSET 0x14c
#define DH_ACTOR_state8_animation_OFFSET 0x14e
#define CAMERA_M16(p, field) M16(p, DH_CAMERA_##field##_OFFSET)
#define CAMERA_M32(p, field) M32(p, DH_CAMERA_##field##_OFFSET)
#define DH_CAMERA_position_x_OFFSET 0x08
#define DH_CAMERA_position_y_OFFSET 0x0c
#define DH_CAMERA_position_z_OFFSET 0x10
#define DH_CAMERA_angle_x_OFFSET 0x24
#define DH_CAMERA_angle_y_OFFSET 0x26
#define DH_CAMERA_angle_z_OFFSET 0x28
#define DH_CAMERA_stage_origin_x_OFFSET 0x84
#define DH_CAMERA_stage_origin_y_OFFSET 0x88
#define DH_CAMERA_stage_origin_z_OFFSET 0x8c
#define DH_CAMERA_focus_x_OFFSET 0x90
#define DH_CAMERA_focus_y_OFFSET 0x94
#define DH_CAMERA_focus_z_OFFSET 0x98
#define DH_CAMERA_orbit_offset_x_OFFSET 0x9c
#define DH_CAMERA_orbit_offset_y_OFFSET 0xa0
#define DH_CAMERA_orbit_offset_z_OFFSET 0xa4
#define DH_CAMERA_request_hold_ticks_OFFSET 0xa8
#define DH_CAMERA_shake_ticks_OFFSET 0xaa
#define DH_CAMERA_target_x_OFFSET 0xb8
#define DH_CAMERA_target_y_OFFSET 0xbc
#define DH_CAMERA_target_z_OFFSET 0xc0
#define DH_CAMERA_target_angle_x_OFFSET 0xc4
#define DH_CAMERA_target_angle_y_OFFSET 0xc6
#define DH_CAMERA_target_angle_z_OFFSET 0xc8
#define SCENE_M16(p, field) M16(p, DH_SCENE_##field##_OFFSET)
#define SCENE_M32(p, field) M32(p, DH_SCENE_##field##_OFFSET)
#define SCENE_M8(p, field) M8(p, DH_SCENE_##field##_OFFSET)
#define SCENE_MEM32(p, field) MEM32((p) + DH_SCENE_##field##_OFFSET)
#define SCENE_MEM8(p, field) MEM8((p) + DH_SCENE_##field##_OFFSET)
#define DH_SCENE_texture_groups_OFFSET 0x00
#define DH_SCENE_pattern_groups_OFFSET 0x04
#define DH_SCENE_model_list_OFFSET 0x08
#define DH_SCENE_sound_resource_OFFSET 0x0c
#define DH_SCENE_camera_definition_OFFSET 0x10
#define DH_SCENE_stage_index_OFFSET 0x20
#define DH_SCENE_view_radius_OFFSET 0x24
#define DH_SCENE_pose_index_OFFSET 0x28
#define DH_SCENE_angle_variant_OFFSET 0x29
#define VDP1_COMMAND_M16(p, field) M16(p, DH_VDP1_COMMAND_##field##_OFFSET)
#define VDP1_COMMAND_M32(p, field) M32(p, DH_VDP1_COMMAND_##field##_OFFSET)
#define VDP1_COMMAND_MEM16(p, field) MEM16((p) + DH_VDP1_COMMAND_##field##_OFFSET)
#define VDP1_COMMAND_MEM32(p, field) MEM32((p) + DH_VDP1_COMMAND_##field##_OFFSET)
#define DH_VDP1_COMMAND_control_OFFSET 0x00
#define DH_VDP1_COMMAND_draw_mode_OFFSET 0x04
#define DH_VDP1_COMMAND_colour_OFFSET 0x06
#define DH_VDP1_COMMAND_character_address_OFFSET 0x08
#define DH_VDP1_COMMAND_character_size_OFFSET 0x0a
#define DH_VDP1_COMMAND_vertex_a_OFFSET 0x0c
#define DH_VDP1_COMMAND_vertex_b_OFFSET 0x10
#define DH_VDP1_COMMAND_vertex_c_OFFSET 0x14
#define DH_VDP1_COMMAND_vertex_d_OFFSET 0x18
#define DH_VDP1_COMMAND_sort_key_OFFSET 0x20
#define SPRITE_DRAW_MEM32(p, field) MEM32((p) + DH_SPRITE_DRAW_##field##_OFFSET)
#define SPRITE_DRAW_MEMU16(p, field) MEMU16((p) + DH_SPRITE_DRAW_##field##_OFFSET)
#define DH_SPRITE_DRAW_vertex_a_OFFSET 0x00
#define DH_SPRITE_DRAW_vertex_b_OFFSET 0x04
#define DH_SPRITE_DRAW_vertex_c_OFFSET 0x08
#define DH_SPRITE_DRAW_vertex_d_OFFSET 0x0c
#define DH_SPRITE_DRAW_depth_OFFSET 0x20
#define DH_SPRITE_DRAW_descriptor_OFFSET 0x28
#define DH_SPRITE_DRAW_mode_bits_OFFSET 0x2c
#define SPRITE_CHARACTER_M16(p, field) M16(p, DH_SPRITE_CHARACTER_##field##_OFFSET)
#define SPRITE_CHARACTER_MEM16(p, field) MEM16((p) + DH_SPRITE_CHARACTER_##field##_OFFSET)
#define SPRITE_CHARACTER_MEMU16(p, field) MEMU16((p) + DH_SPRITE_CHARACTER_##field##_OFFSET)
#define DH_SPRITE_CHARACTER_character_address_OFFSET 0x00
#define DH_SPRITE_CHARACTER_character_size_OFFSET 0x02
#define DH_SPRITE_CHARACTER_width_OFFSET 0x04
#define DH_SPRITE_CHARACTER_height_OFFSET 0x06
#define DH_SPRITE_CHARACTER_draw_mode_OFFSET 0x08

#define SOUND_QUEUE_INDEX(p, field) MEM32((p) + DH_SOUND_QUEUE_##field##_OFFSET)
#define SOUND_QUEUE_INDEX_PTR(p, field) PTR(s32, (p) + DH_SOUND_QUEUE_##field##_OFFSET)
#define SOUND_QUEUE_COMMANDS(p) PTR(s32, (p) + DH_SOUND_QUEUE_commands_OFFSET)
#define DH_SOUND_QUEUE_read_index_OFFSET 0x00
#define DH_SOUND_QUEUE_write_index_OFFSET 0x04
#define DH_SOUND_QUEUE_commands_OFFSET 0x08

/* Pass 4: wrappers preserve the exact original accessor, including MP. */

/* Model source/runtime record, 0x14 stride: loader 02223ac0 and independent
 * immediate/pending mesh builders 02215cb4/02215bf4/02235dc0 agree. */
typedef struct DhModelDefinition {
    uint8_t vertex_count; /* 00: 16-byte vertex records */
    uint8_t quad_count;   /* 01: 16-byte quad descriptors */
    uint8_t reserved_02[2];
    uint32_t vertices;    /* 04: guest pointer word */
    uint32_t quads;       /* 08: guest pointer word */
    uint8_t reserved_0c[4];
    int8_t draw_handler_index;  /* 10: ORed with submission mode before dispatch */
    uint8_t reserved_11[3];
} DhModelDefinition;

/* Scene sequence work prefix, not actor work: timed list interpreters
 * 0221b8e4/0221bf72/0221c690/0221cad4 and process_spawn_list agree.
 * main_ticks can count upward in some sequences; names describe its clock. */
typedef struct DhSceneSequenceWork {
    uint8_t reserved_00[0x18];
    int32_t main_ticks;      /* 18 */
    int32_t phase_ticks;     /* 1c */
    uint8_t reserved_20[0x10];
    uint32_t spawn_cursor;   /* 30: next 24-byte spawn entry */
    uint8_t reserved_34[8];
    uint32_t branch_cursor;  /* 3c: saved branch list */
} DhSceneSequenceWork;
#define DH_ACTOR_position_x_OFFSET 0x08
#define DH_ACTOR_position_y_OFFSET 0x0c
#define DH_ACTOR_position_z_OFFSET 0x10
#define DH_ACTOR_movement_x_OFFSET 0x14
#define DH_ACTOR_movement_y_OFFSET 0x18
#define DH_ACTOR_movement_z_OFFSET 0x1c
#define DH_ACTOR_slot_id_OFFSET 0x34
#define DH_ACTOR_interpolation_ticks_OFFSET 0x40
#define DH_ACTOR_animation_control_OFFSET 0x41
#define DH_ACTOR_state_handler_OFFSET 0x43
#define DH_ACTOR_facing_right_OFFSET 0x44
#define DH_ACTOR_hit_shake_ticks_OFFSET 0x4e
#define DH_ACTOR_action_category_OFFSET 0x84
#define DH_ACTOR_action_state_OFFSET 0x88
#define DH_ACTOR_state_flags_OFFSET 0xe0
#define DH_ACTOR_state_index_OFFSET 0xe4
#define DH_ACTOR_action_pending_OFFSET 0x48
#define DH_ACTOR_hit_source_slot_OFFSET 0x4a
#define DH_ACTOR_paired_slot_OFFSET 0x4b
#define DH_ACTOR_interaction_latch_OFFSET 0x52
#define DH_ACTOR_health_OFFSET 0x56
#define DH_ACTOR_saved_primary_effect_type_OFFSET 0x6a
#define DH_ACTOR_saved_primary_effect_slot_OFFSET 0x6b
#define DH_ACTOR_primary_handoff_pending_OFFSET 0x70
#define DH_ACTOR_primary_handoff_slot_OFFSET 0x71
#define DH_ACTOR_motion_speed_OFFSET 0x74
#define DH_ACTOR_selected_effect_slot_OFFSET 0x94
#define DH_ACTOR_type_flags_OFFSET 0xba
#define DH_ACTOR_animation_script_OFFSET 0xd0
#define DH_ACTOR_queued_attack_kind_OFFSET 0x100
#define DH_ACTOR_queued_attack_slot_OFFSET 0x104
#define DH_ACTOR_action_start_tick_OFFSET 0x114
#define DH_ACTOR_direction_animation_table_OFFSET 0x154
#define DH_ACTOR_action_config_tables_OFFSET 0x158
#define DH_ACTOR_reaction_config_tables_OFFSET 0x15c
#define DH_ACTOR_response_config_tables_OFFSET 0x164
#define DH_ACTOR_distance_attack_table_OFFSET 0x168
#define DH_ACTOR_motion_action_tables_OFFSET 0x16c
#define DH_ACTOR_reaction_descriptor_OFFSET 0x180
#define DH_ACTOR_state_animation_descriptor_OFFSET 0x184
#define DH_ACTOR_variant_animation_table_OFFSET 0x188
#define ACTOR_MP(p, field) MP(p, DH_ACTOR_##field##_OFFSET)
#define DH_CAMERA_update_handler_OFFSET 0x64
#define DH_CAMERA_horizontal_range_OFFSET 0x68
#define CAMERA_MP(p, field) MP(p, DH_CAMERA_##field##_OFFSET)
#define DH_MODEL_vertex_count_OFFSET 0x00
#define DH_MODEL_quad_count_OFFSET 0x01
#define DH_MODEL_vertices_OFFSET 0x04
#define DH_MODEL_quads_OFFSET 0x08
#define DH_MODEL_draw_handler_index_OFFSET 0x10
#define MODEL_M8(p, field) M8(p, DH_MODEL_##field##_OFFSET)
#define MODEL_MEM32(p, field) MEM32((p) + DH_MODEL_##field##_OFFSET)
#define MODEL_MEM8(p, field) MEM8((p) + DH_MODEL_##field##_OFFSET)
#define MODEL_MEMU32(p, field) MEMU32((p) + DH_MODEL_##field##_OFFSET)
#define MODEL_MP(p, field) MP(p, DH_MODEL_##field##_OFFSET)
#define MODEL_MU32(p, field) MU32(p, DH_MODEL_##field##_OFFSET)
#define MODEL_MU8(p, field) MU8(p, DH_MODEL_##field##_OFFSET)
#define DH_SEQUENCE_main_ticks_OFFSET 0x18
#define DH_SEQUENCE_phase_ticks_OFFSET 0x1c
#define DH_SEQUENCE_spawn_cursor_OFFSET 0x30
#define DH_SEQUENCE_branch_cursor_OFFSET 0x3c
#define SEQUENCE_M32(p, field) M32(p, DH_SEQUENCE_##field##_OFFSET)
#define SEQUENCE_MEM32(p, field) MEM32((p) + DH_SEQUENCE_##field##_OFFSET)
#define SEQUENCE_MEMU32(p, field) MEMU32((p) + DH_SEQUENCE_##field##_OFFSET)
#define SEQUENCE_MP(p, field) MP(p, DH_SEQUENCE_##field##_OFFSET)
#define SEQUENCE_MU32(p, field) MU32(p, DH_SEQUENCE_##field##_OFFSET)

#endif /* DH_WORK_H */
