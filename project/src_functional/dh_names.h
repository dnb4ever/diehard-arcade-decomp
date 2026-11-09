/* Shared semantic names for readability passes 1, 2 and 3. See notes/readability_pass2.md.
 * Macro aliases retain the original extern C symbols (including letter case).
 * Extern declarations keep their original per-TU types and volatility.
 * Address constants retain original literal suffixes; accessors retain width,
 * signedness and volatility. Never infer a layout from an offset alone.
 */
#ifndef DH_NAMES_H
#define DH_NAMES_H

/* HWRAM / resident-symbol aliases. _legacy preserves a second C spelling. */
#define g_matrix_stack_cursor                          g_06098878
#define g_ui_cursor_column                             g_060997A4
#define g_ui_cursor_column_legacy                      g_060997a4
#define g_ui_cursor_row                                g_060997A8
#define g_ui_cursor_row_legacy                         g_060997a8
#define g_actor_slot_registry                          g_06099584
#define g_sound_effects_enabled                        g_060e0ddc
#define g_sound_effects_enabled_legacy                 g_060E0DDC
#define g_view_task                                    g_0609882c
#define g_view_task_legacy                             g_0609882C
#define g_actor_slot_masks                             g_0604ebe4
#define g_actor_slot_masks_legacy                      g_0604EBE4
#define g_scene_event_flags                            g_06098580
#define g_actor_slot_tasks                             g_060995B0
#define g_actor_slot_tasks_legacy                      g_060995b0
#define g_transformed_vector                           g_06098b1c
#define g_transformed_vector_legacy                    g_06098B1C
#define g_frame_tick                                   g_060987BC
#define g_frame_tick_legacy                            g_060987bc
#define g_vdp2_register_shadow                         g_06098F08
#define g_vdp2_register_shadow_legacy                  g_06098f08
#define g_scene_descriptor                             g_0609866C
#define g_scene_descriptor_legacy                      g_0609866c
#define g_view_request_priority                        g_06098830
#define g_play_session_mode                            g_06098670
#define g_display_transfer_cursor                      g_06099510
#define g_scene_index                                  g_06098668
#define g_actor_slot_ids                               g_06099590
#define g_bios_status_buffer                           g_060987EC
#define g_bios_status_buffer_legacy                    g_060987ec
#define g_player_input_records                         g_06098734

/* Memory-mapped register extern aliases; preserve existing linker identities. */
#define reg_vdp1_tvmr                                  reg_25D00000
#define reg_vdp2_tvmd                                  reg_25F80000
#define reg_vdp2_ccctl                                 reg_25F800EC
#define reg_scu_t0c                                    reg_25FE0090
#define reg_smpc_sf                                    reg_20100063
#define reg_smpc_comreg                                reg_2010001F
#define reg_smpc_ireg0                                 reg_20100001
#define reg_smpc_ireg1                                 reg_20100003
#define reg_smpc_ireg2                                 reg_20100005
#define reg_smpc_oreg3                                 reg_20100027

/* Absolute addresses. _U variants preserve unsigned literal types. */
/* matrix stack cursor. */
#define DH_MATRIX_STACK_CURSOR_ADDR                    0x06098878
#define DH_MATRIX_STACK_CURSOR_ADDR_U                  0x06098878u

/* ui cursor column. */
#define DH_UI_CURSOR_COLUMN_ADDR                       0x060997a4

/* ui cursor row. */
#define DH_UI_CURSOR_ROW_ADDR                          0x060997a8

/* actor slot registry. */
#define DH_ACTOR_SLOT_REGISTRY_ADDR                    0x06099584
#define DH_ACTOR_SLOT_REGISTRY_ADDR_U                  0x06099584u

/* sound effects enabled. */
#define DH_SOUND_EFFECTS_ENABLED_ADDR                  0x060e0ddc
#define DH_SOUND_EFFECTS_ENABLED_ADDR_U                0x060e0ddcu

/* view task. */
#define DH_VIEW_TASK_ADDR                              0x0609882c
#define DH_VIEW_TASK_ADDR_U                            0x0609882cu

/* actor slot masks. */
#define DH_ACTOR_SLOT_MASKS_ADDR                       0x0604ebe4
#define DH_ACTOR_SLOT_MASKS_ADDR_U                     0x0604ebe4u

/* scene event flags. */
#define DH_SCENE_EVENT_FLAGS_ADDR                      0x06098580

/* actor slot tasks. */
#define DH_ACTOR_SLOT_TASKS_ADDR                       0x060995b0
#define DH_ACTOR_SLOT_TASKS_ADDR_U                     0x060995b0u

/* transformed vector. */
#define DH_TRANSFORMED_VECTOR_ADDR                     0x06098b1c
#define DH_TRANSFORMED_VECTOR_ADDR_U                   0x06098b1cu

/* frame tick. */
#define DH_FRAME_TICK_ADDR                             0x060987bc
#define DH_FRAME_TICK_ADDR_U                           0x060987bcu

/* vdp2 register shadow. */
#define DH_VDP2_REGISTER_SHADOW_ADDR                   0x06098f08
#define DH_VDP2_REGISTER_SHADOW_ADDR_U                 0x06098f08u

/* scene descriptor. */
#define DH_SCENE_DESCRIPTOR_ADDR                       0x0609866c

/* view request priority. */
#define DH_VIEW_REQUEST_PRIORITY_ADDR                  0x06098830
#define DH_VIEW_REQUEST_PRIORITY_ADDR_U                0x06098830u

/* play session mode. */
#define DH_PLAY_SESSION_MODE_ADDR                      0x06098670
#define DH_PLAY_SESSION_MODE_ADDR_U                    0x06098670u

/* display transfer cursor. */
#define DH_DISPLAY_TRANSFER_CURSOR_ADDR                0x06099510

/* scene index. */
#define DH_SCENE_INDEX_ADDR                            0x06098668

/* actor slot ids. */
#define DH_ACTOR_SLOT_IDS_ADDR                         0x06099590

/* bios status buffer. */
#define DH_BIOS_STATUS_BUFFER_ADDR                     0x060987ec

/* player input records. */
#define DH_PLAYER_INPUT_RECORDS_ADDR                   0x06098734
#define DH_PLAYER_INPUT_RECORDS_ADDR_U                 0x06098734u


/* Saturn register map: VDP1/VDP2/SCU/SMPC/SCSP manuals (see pass note). */
/* TV mode selection. */
#define VDP1_TVMR_ADDR                                 0x25d00000

/* framebuffer change control. */
#define VDP1_FBCR_ADDR                                 0x25d00002

/* plot trigger. */
#define VDP1_PTMR_ADDR                                 0x25d00004

/* erase/write data. */
#define VDP1_EWDR_ADDR                                 0x25d00006

/* erase/write upper-left coordinate. */
#define VDP1_EWLR_ADDR                                 0x25d00008

/* erase/write lower-right coordinate. */
#define VDP1_EWRR_ADDR                                 0x25d0000a

/* transfer end status (manual offset 0x100010). */
#define VDP1_EDSR_ADDR                                 0x25d00010

/* TV screen mode. */
#define VDP2_TVMD_ADDR                                 0x25f80000

/* color calculation control. */
#define VDP2_CCCTL_ADDR                                0x25f800ec

/* level-0 DMA read address. */
#define SCU_D0R_ADDR                                   0x25fe0000
#define SCU_D0R_ADDR_U                                 0x25fe0000u

/* level-0 DMA write address. */
#define SCU_D0W_ADDR                                   0x25fe0004
#define SCU_D0W_ADDR_U                                 0x25fe0004u

/* level-0 DMA count. */
#define SCU_D0C_ADDR                                   0x25fe0008
#define SCU_D0C_ADDR_U                                 0x25fe0008u

/* level-0 DMA address addition. */
#define SCU_D0AD_ADDR                                  0x25fe000c
#define SCU_D0AD_ADDR_U                                0x25fe000cu

/* level-0 DMA enable. */
#define SCU_D0EN_ADDR                                  0x25fe0010
#define SCU_D0EN_ADDR_U                                0x25fe0010u

/* level-0 DMA mode. */
#define SCU_D0MD_ADDR                                  0x25fe0014
#define SCU_D0MD_ADDR_U                                0x25fe0014u

/* DMA status. */
#define SCU_DSTA_ADDR                                  0x25fe007c
#define SCU_DSTA_ADDR_U                                0x25fe007cu

/* timer-0 compare. */
#define SCU_T0C_ADDR                                   0x25fe0090
#define SCU_T0C_ADDR_U                                 0x25fe0090u

/* timer mode. */
#define SCU_T1MD_ADDR                                  0x25fe0098
#define SCU_T1MD_ADDR_U                                0x25fe0098u

/* status flag. */
#define SMPC_SF_ADDR                                   0x20100063

/* command register. */
#define SMPC_COMREG_ADDR                               0x2010001f

/* input register 0. */
#define SMPC_IREG0_ADDR                                0x20100001

/* input register 1. */
#define SMPC_IREG1_ADDR                                0x20100003

/* input register 2. */
#define SMPC_IREG2_ADDR                                0x20100005

/* output register 3 (OREG0 is +0x21). */
#define SMPC_OREG3_ADDR                                0x20100027

/* port-2 data. */
#define SMPC_PDR2_ADDR                                 0x20100077

/* common control word: MEM4MB, DAC18B, version, master volume. */
#define SCSP_COMMON_CONTROL_ADDR                       0x25b00400

/* Byte offsets in the task node. These do not impose a host struct ABI. */
#define TASK_HANDLER                                   0x00
#define TASK_NEXT                                      0x04
#define TASK_WORK                                      0x08
#define TASK_STATE                                     0x10
#define TASK_PARENT                                    0x14
#define TASK_DEPTH                                     0x0E

/* World-object payload prefix, only at confirmed actor/camera sites. */
#define OBJ_POS_X                                      0x08
#define OBJ_POS_Y                                      0x0C
#define OBJ_POS_Z                                      0x10
#define OBJ_ANGLE_X                                    0x24
#define OBJ_ANGLE_Y                                    0x26
#define OBJ_ANGLE_Z                                    0x28

/* Actor payload fields, only at confirmed actor-handler sites. */
#define ACTOR_SLOT_ID                                  0x34
#define ACTOR_ANIM_REQUEST                             0x5C


/* Readability pass 2 function spellings. Preserve original linker symbols.
 * Each address remains in the alias and in the function header. */
#define game_boot_and_run                              func_02201020
#define game_reset_globals                             func_02201958
#define scene_advance_task                             func_02205b20
#define scene_vdp2_bringup_task                        func_02202138
#define scene_palette_fade_task                        func_02201c9c
#define scene_display_setup_task                       func_02201f20
#define task_pool_reset                                func_0220fc18
#define task_insert_after_current                      func_0220fc3c
#define task_insert_after_parent                       func_0220fc82
#define task_cancel_below_depth                        func_0220fdc8
#define task_run_frame                                 func_0220fde4
#define task_spawn_child_work88                        func_0220feec
#define task_spawn_current_work6c                      func_0220fece
#define task_spawn_list_work7c                         func_0220ff28
#define task_spawn_current_work40                      func_0220ff54
#define task_spawn_child_work40                        func_0220ff72
#define task_spawn_actor_work                          func_0220ff0a
#define task_alloc_zeroed_array                        func_02214bb8
#define task_run_work_handler                          func_0220b354
#define input_reset_records                            func_0220f990
#define input_poll_records                             func_0220f9d4
#define input_update_direction_tap                     func_0220fb98
#define player_update_combat_input                     func_02223ee0
#define actor_resolve_facing                           func_02227704
#define actor_dispatch_pose_and_type                   func_0222adaa
#define actor_queue_pose_sound                         func_0222aadc
#define actor_load_pose_animation                      func_0223a6dc
#define actor_update_task                              func_0222863e
#define enemy_reset_idle_or_walk                       func_022469f2
#define actor_enter_hit_reaction                       func_022489d0
#define actor_register_movement_slot                   func_0221707c
#define actor_allocate_effect_slot                     func_02228106
#define scene_select_player_parameter                  func_0222a878
#define actor_spawn_fixed_slot                         func_02227bc4
#define camera_update_slot_orbit                       func_0220cdf6
#define camera_update_player_follow                    func_0220cfac
#define actor_update_orbit_task                        func_0220d262
#define camera_compute_lookat_angles                   func_0221055c
#define view_build_display_transform                   func_022225f6
#define collision_reset_frame                          func_0222ecc4
#define collision_sweep_grid                           func_0222ece4
#define collision_queue_actor                          func_0222eea2
#define collision_insert_grid                          func_0222eee4
#define collision_test_pose_hitboxes                   func_0222ef38
#define collision_test_swept_hitbox                    func_0222f1d0
#define model_submit_culled                            func_02215a2c
#define sprite_draw_child                              func_022153f6
#define sprite_draw_billboard                          func_022154fc
#define vdp1_upload_texture_groups                     func_022150d4
#define vdp1_begin_frame                               func_02214cdc
#define vdp1_trigger_draw                              func_02214d26
#define vdp1_upload_command_lists                      func_02214f90
#define audio_load_driver                              func_02215da8
#define audio_send_command_buffer                      func_02215e20
#define audio_queue_scene_sound                        func_02215fac
#define game_random_next                               func_02210020
#define fixed_mul16                                    func_022131c8
#define fixed_sqrt16                                   func_02213164
#define memory_clear_vec4_blocks                       func_02213290
#define shc_shift_left                                 func_0224a360
#define shc_shift_right_signed                         func_0224a420


/* Pass 2 globals. Keep per-TU extern types and original C linkage. */
#define g_random_seed                                  g_0609957C
#define g_random_seed_legacy                           g_0609957c
#define g_task_list_control                            g_06092238
#define g_active_task_count                            g_06096244
#define g_collision_node_count                         g_06097550
#define g_collision_active_head                        g_06097554
#define g_collision_grid_heads                         g_06097558
#define g_collision_node_pool                          g_06097450
#define g_effect_slot_occupied                         g_060996D4
#define g_effect_slot_occupied_legacy                  g_060996d4
#define g_player_direction_taps                        g_060987AC
#define g_player_direction_taps_legacy                 g_060987ac
#define g_ready_player_mask                            g_06098564
#define g_player_join_states                           g_06098674
#define g_actor_state_handlers                         g_06091100
#define g_enemy_state_handlers                         g_0609192c
#define g_actor_pose_handlers                          g_060911BC
#define g_actor_pose_handlers_legacy                   g_060911bc
#define g_actor_type_handlers                          g_060911C4
#define g_actor_pose_set                               g_0609979A
#define g_actor_pose_set_legacy                        g_0609979a
#define g_audio_latched_sound_id                       g_060E0DE0
#define g_audio_latched_sound_id_legacy                g_060e0de0
#define g_audio_command_ready                          g_060E0DE4
#define g_actor_movement_records                       g_060B6D9C
#define g_actor_movement_records_legacy                g_060b6d9c

/* Exact guest addresses, with the original literal signedness/suffix. */
#define DH_ACTOR_MOVEMENT_RECORDS_ADDR                 0x060b6d9c
#define DH_ACTOR_POSE_HANDLERS_ADDR                    0x060911bc
#define DH_ACTOR_POSE_HANDLERS_ADDR_U                  0x060911bcu
#define DH_ACTOR_POSE_SET_ADDR                         0x0609979a
#define DH_ACTOR_POSE_SET_ADDR_U                       0x0609979au
#define DH_AUDIO_LATCHED_SOUND_ID_ADDR                 0x060e0de0
#define DH_COLLISION_ACTIVE_HEAD_ADDR                  0x06097554
#define DH_COLLISION_GRID_HEADS_ADDR                   0x06097558
#define DH_EFFECT_SLOT_OCCUPIED_ADDR                   0x060996d4
#define DH_PLAYER_DIRECTION_TAPS_ADDR                  0x060987ac
#define DH_PLAYER_DIRECTION_TAPS_ADDR_U                0x060987acu
#define DH_PLAYER_JOIN_STATES_ADDR                     0x06098674
#define DH_RANDOM_SEED_ADDR                            0x0609957c
#define DH_READY_PLAYER_MASK_ADDR                      0x06098564
#define DH_SOUND_QUEUE_COMMANDS_ADDR                   0x060e0df0
#define DH_SOUND_QUEUE_READ_INDEX_ADDR                 0x060e0de8
#define DH_SOUND_QUEUE_WRITE_INDEX_ADDR                0x060e0dec
#define DH_TASK_LIST_CONTROL_ADDR                      0x06092238

#define DH_ACTIVE_TASK_COUNT_ADDR                      0x06096244
#define DH_COLLISION_NODE_COUNT_ADDR                   0x06097550
#define DH_COLLISION_NODE_POOL_ADDR                    0x06097450
#define DH_ACTOR_STATE_HANDLERS_ADDR                   0x06091100
#define DH_ENEMY_STATE_HANDLERS_ADDR                   0x0609192c
#define DH_ACTOR_TYPE_HANDLERS_ADDR                    0x060911c4
#define DH_AUDIO_COMMAND_READY_ADDR                    0x060e0de4

/* Input masks: axes follow world-coordinate debug movement in 0220e1e2.
 * Action bits remain numbered until physical button labels are proven. */
#define DH_PAD_ACTION1                                 0x1
#define DH_PAD_ACTION2                                 0x2
#define DH_PAD_ACTION3                                 0x4
#define DH_PAD_ACTIONS                                 0x7
#define DH_PAD_Z_NEG                                   0x40
#define DH_PAD_Z_POS                                   0x80
#define DH_PAD_DEPTH                                   0xc0
#define DH_PAD_X_NEG                                   0x100
#define DH_PAD_X_POS                                   0x200
#define DH_PAD_HORIZONTAL                              0x300
#define DH_PAD_DIRECTIONS                              0x3c0
#define DH_SYSTEM_START_P1                             0x10
#define DH_SYSTEM_START_P2                             0x20
#define DH_SYSTEM_START_MASK                           0x30
#define DH_SCENE_STOP                                  81
#define DH_SCENE_NONPLAY_FIRST                         76
#define DH_TAP_WAIT_IDLE                               0
#define DH_TAP_CONFIRM_IDLE                            1
#define DH_TAP_WAIT_PRESS                              2
#define DH_TAP_WAIT_RELEASE                            3
#define DH_DISPLAY_CONFIGURE                           0
#define DH_DISPLAY_WAIT                                1
#define DH_DISPLAY_NOTIFY_PARENT                       2

/* Pass 3: source names retain exact original C linker spellings.
 * _entry preserves raw externs distinct from an existing descriptive definition. */
#define actor_commit_effect_handoffs                     func_0223a9d0
#define actor_dispatch_action_and_finish                 func_0223ffcc
#define vdp2_queue_attribute                             func_0221632e
#define effect_pool_spawn                                func_0223cbb0
#define matrix_translate_current                         func_02213ca4
#define collision_sweep_pose                             func_022173f4
#define vdp2_pack_layer_addresses                        func_0222156c
#define actor_mark_slots_for_refresh                     func_0222a708
#define actor_select_directional_animation               func_0224124e
#define actor_draw_transformed                           func_02246282
#define results_draw_header                              func_022093f8
#define sh2_dma0_copy_words                              func_022145d0
#define actor_reset_pose_history                         func_02223e70
#define enemy_attack_is_armed                            func_02245dcc
#define enemy_classify_attack_timing                     func_02245fc4
#define vdp2_upload_pattern_groups                       func_02215228
#define sound_queue_push_entry                           func_02215f6c
#define video_reset_buffers                              func_02216154
#define draw_actor_hier_shadowed_entry                   func_0222ca98
#define effect_pool_reset_scene                          func_0223c95c
#define effect_pool_make_room                            func_0223e4a0
#define model_load_scene_records                         func_02223ac0
#define effect_pool_update_pose                          func_0223a924
#define enemy_load_action_descriptor                     func_02245c88
#define matrix_scale_columns                             func_02213ce4
#define tilemap_copy_wrap_wide                           func_0221675e
#define stage_initialize_grids                           func_02216c38
#define player_clear_saved_action                        func_0223dcc2
#define actor_resolve_movement                           func_022170ea
#define grid_find_overlap_type_zero                      func_02217ae4
#define vdp2_apply_layer1_commands                       func_02220ade
#define vdp2_set_layer_scroll                            func_022213a8
#define actor_reset_slot_registry                        func_02227a84
#define actor_load_part_ids                              func_0222ca54
#define actor_publish_linked_slot_value                  func_0222f3c0
#define actor_consume_linked_effect_count                func_0223c74a
#define enemy_enter_clash                                func_02249a82
#define camera_tick_shake                                func_0220d98e
#define camera_ease_to_target                            func_0220e0aa
#define display_ramp_to_limit                            func_022101ce
#define matrix_transform_to_scratch                      func_02213538
#define vdp2_apply_sprite_commands                       func_02216b14
#define stage_draw_visible_cells                         func_02216d58
#define player_refresh_ready_mask                        func_0222a8d8
#define actor_compute_motion_components                  func_0222acf8
#define display_accumulate_channel                       func_022386fc
#define display_mark_channel_bits                        func_02238dde
#define effect_pool_release                              func_0223cd0c
#define enemy_enter_guard                                func_02247d6e
#define enemy_enter_optional_pose                        func_022499a6
#define scene_record_bootstrap_task                      func_02218038
#define player_update_aim_limits                         func_0221af30
#define process_spawn_list_entry                         func_0222a5d0
#define actor_debounce_frame_event                       func_02241556
#define enemy_sweep_toward_target                        func_02243fe0
#define collision_probe_between_actors                   func_022452be
#define enemy_test_player_pair_lock                      func_02245eb8
#define enemy_enter_descriptor_state18                   func_022470a2
#define enemy_enter_timed_reaction                       func_02249240
#define enemy_enter_state19                              func_02249fb0
#define hud_install_status_task                          func_0220340c
#define audio_queue_scene_gated_id                       func_0221603e
#define scene_reload_graphics                            func_02217d6c
#define scene_reset_stage                                func_02217f04
#define model_draw_composite_parts                       func_0221b330
#define hud_setup_small_font                             func_02222d3c
#define hud_setup_large_font                             func_02222e44
#define actor_fill_pose_history                          func_022260c8
#define actor_decode_pose_direction                      func_0222ad7a
#define player_try_special_move                          func_0222c220
#define actor_load_pose_buffers                          func_0222db44
#define actor_anim_frame_update_entry                    func_0222dc2c
#define scene_spawn_record_list                          func_02233388
#define actor_load_part_offset                           func_0223b254
#define actor_draw_proximity_pose                        func_0223c6bc
#define actor_check_expiry_or_lock                       func_022402c0
#define actor_draw_by_model_mode                         func_022462bc
#define enemy_enter_dash_recovery_a                      func_02247a16
#define enemy_enter_dash_recovery_b                      func_02247a3e
#define hud_copy_status_template                         func_02203de8
#define hud_encode_status_item                           func_02203e0a
#define display_ramp_to_zero                             func_022100f8
#define mtx_load_abc_entry                               func_022149aa
#define mtx_build_fwd_inv_entry                          func_02214a7a
#define grid_find_overlap_any                            func_02217888
#define vdp2_apply_layer0_commands                       func_02220828
#define actor_spawn_dynamic_slot                         func_02227b60
#define actor_release_slot                               func_02227c22
#define effect_follow_parent_task                        func_0223194a
#define scene_spawn_effect_list                          func_022332d4
#define hud_draw_counter_icon                            func_022380c0
#define hud_upload_shared_assets                         func_0223820c
#define actor_apply_effect_action                        func_02239fc6
#define actor_spawn_special_debris                       func_0223ac9c
#define hud_rebuild_player_counters                      func_0223e17c
#define actor_enter_directional_launch                   func_02241324
#define actor_enter_random_launch                        func_022413e6
#define actor_follow_target_depth                        func_0224174e
#define enemy_find_latched_effect_target                 func_02245d1c
#define enemy_classify_linked_effect                     func_022460c8
#define enemy_begin_scripted_dash                        func_022473ee
#define actor_set_state_7_entry                          func_0224840a
#define enemy_spawn_clash_task                           func_022490a0
#define enemy_begin_combo                                func_02249836
#define enemy_enter_despawn                              func_02249cda
#define shc_copy_word_block                              func_0224a280
#define attract_service_credit_join                      func_022045f8
#define camera_tick_request_hold                         func_0220d8d0
#define camera_compute_xy_lookat                         func_02210622
#define quad_build_projected_corners                     func_02213d24
#define quad_test_visible                                func_02213e0c
#define cache_clear_line_heads                           func_022148ac
#define mtx_apply_vec_at8_entry                          func_02214a40
#define model_build_quad_list                            func_02215cb4
#define audio_queue_actor_cue                            func_022160a0
#define audio_queue_actor_slot_cue                       func_02216110
#define tilemap_copy_wrap_narrow                         func_02216858
#define stage_build_render_grid                          func_02216c88
#define grid_find_overlap_static                         func_022179b0
#define hud_select_player_sources                        func_0221a5f6
#define vdp2_set_layer_zoom                              func_0222142c
#define vdp2_pack_rotation_addresses                     func_02221bd4
#define vdp2_setup_rotation_pair                         func_02221f68
#define vdp2_apply_rotation_commands                     func_022220be
#define vdp2_apply_colour_commands                       func_022223c8
#define actor_enter_pose_mode                            func_0222bd54
#define actor_emit_spark_children                        func_0222fe14
#define effect_spawn_actor_clone                         func_022301c4
#define effect_orient_from_parent                        func_02231a80
#define model_emit_mesh_part                             func_02235dc0
#define hud_draw_status_panel                            func_02238134
#define hud_draw_status_digit                            func_02238176
#define actor_restore_primary_channel                    func_02239f78
#define effect_mark_actor_slot_ready                     func_0223a394
#define actor_load_recovery_pose                         func_0223ac50
#define actor_load_transition_pose                       func_0223d0c6
#define player_restore_saved_action                      func_0223dd96
#define actor_enter_lateral_reaction                     func_022414ee
#define actor_apply_direction_code                       func_022417a4
#define actor_finish_arrival                             func_022418fa
#define actor_latch_action_completion                    func_02241b2c
#define actor_test_target_rectangle                      func_0224492c
#define enemy_find_player_in_strike_range                func_022453de
#define enemy_select_player_side                         func_02245a6a
#define enemy_begin_approach                             func_02246cac
#define enemy_dispatch_script                            func_02247036
#define hud_encode_config_nibble                         func_02203e5e
#define scene_bootstrap                                  func_022058a8
#define scene81_stage_setup_entry                        func_02206e2e
#define scene_bg_layer_build_entry                       func_02207360
#define scene_display_initialize_entry                   func_022082be
#define resident_add_session_value                       func_0220a962
#define resident_increment_counter104                    func_0220ac5a
#define resident_increment_counter108                    func_0220aca8
#define smpc_read_intback                                func_0220acf6
#define debug_draw_camera_pose                           func_0220ad58
#define camera_count_slots_in_rectangle                  func_0220e000
#define camera_step_scene_definition                     func_0220f6f6
#define camera_bind_scene_definition                     func_0220f816
#define task_spawn_child_work6c                          func_0220feb0
#define task_spawn_current_workdc                        func_0220ff90
#define camera_angles_from_delta                         func_022105ca
#define animation_bind_channel                           func_02210858
#define animation_sample_absolute                        func_022109d4
#define sin_lookup_entry                                 func_02213190
#define cos_lookup_entry                                 func_022131a8
#define vector_dot_fixed16                               func_022132c4
#define vertex_transform_project_raw_entry               func_022135a0
#define smpc_mask_port_shadow                            func_02214970
#define smpc_clear_port_shadow_bit                       func_02214980
#define mtx_rotate_zyx_at24_entry                        func_02214a06
#define heap_allocate_owned                              func_02214b70
#define heap_free_coalesce                               func_02214c8a
#define vdp1_reset_sort_arenas                           func_02214dae
#define vdp2_register_attribute_pair                     func_0221636c
#define tilemap_fill_narrow                              func_02216a26
#define collision_resolve_sweep                          func_0221746c
#define grid_remove_node                                 func_02217d22
#define hud_recoil_sprite_task                           func_0221a6a8
#define scene_spawn_matching_behaviors                   func_0221ad58
#define model_draw_title_composite                       func_0221c368
#define vdp2_apply_text_layer_commands                   func_0222109e
#define vdp2_disable_layer_bit                           func_02221384
#define vdp2_configure_layer1_interpolation              func_022214d2
#define vdp2_setup_coefficient_pair                      func_02222052
#define view_initialize_transform                        func_02222574
#define text_write_small                                 func_02222fe0
#define text_write_large                                 func_02223080
#define format_decimal                                   func_02223862
#define format_padded_string                             func_02223a52
#define effect_handoff_sprite_task                       func_02227906
#define actor_clear_slot_without_count                   func_02227c5c
#define actor_spawn_arm_task                             func_0222a300
#define task_clear_handler                               func_0222a5ca
#define actor_select_scene_variant                       func_0222a91a
#define player_select_motion_animation                   func_0222b7e8
#define actor_draw_skeletal_mesh                         func_0222d450
#define actor_load_current_pose_buffers                  func_0222da90
#define actor_store_part_transform                       func_0222ec98
#define projectile_sustained_fire_task                   func_022300e8
#define scene_spawn_display_and_effects                  func_02233250
#define hud_update_player_meters                         func_02237828
#define hud_draw_segmented_meter                         func_02237be4
#define hud_draw_meter_icon                              func_02237e0e
#define hud_draw_meter_label                             func_02237e6c
#define effect_spark_fall_back_task                      func_02238ba4
#define effect_spark_fall_forward_task                   func_02238c4e
#define effect_flash_expand_task                         func_02238cd2
#define effect_trail_fall_task                           func_02238d5e
#define display_pulse_decay_task                         func_02238df4
#define debug_reset_model_preview                        func_0223964e
#define actor_select_effect_action                       func_02239ce8
#define actor_aim_at_target_record                       func_0223a806
#define actor_spawn_secondary_effect                     func_0223ab5c
#define actor_spawn_counted_effect                       func_0223abd0
#define actor_load_hit_pose                              func_0223afc4
#define actor_begin_throw_effect                         func_0223b01e
#define actor_begin_hold_effect                          func_0223b1c0
#define player_consume_linked_effect_count               func_0223c7ba
#define actor_begin_effect_action                        func_0223cfe4
#define actor_begin_effect_action_one                    func_0223d0fe
#define player_save_action                               func_0223dd28
#define hud_update_player_counters                       func_0223df30
#define hud_queue_action_label                           func_0223e104
#define actor_select_walk_or_random_pose                 func_02241164
#define actor_debounce_long_event                        func_0224158c
#define actor_clamp_target_heading                       func_022416c4
#define actor_dispatch_type_completion                   func_022418ce
#define enemy_resolve_team_pairs                         func_02243628
#define actor_compare_xz_quadrant                        func_02243fba
#define enemy_sweep_to_anchor                            func_022444b6
#define enemy_test_dodge_side                            func_02244820
#define actor_state_dispatch_entry                       func_02244e2e
#define actor_decode_enemy_direction                     func_02245e58
#define actor_face_from_yaw_sign                         func_02245e80
#define combat_reset_stage_bounds                        func_02246052
#define actor_flag_rng_1in4_entry                        func_02246208
#define enemy_approach_entry_task                        func_0224680c
#define enemy_enter_idle_state18                         func_02247064
#define enemy_enter_parent_link                          func_0224910e
#define enemy_enter_stage_pose18                         func_02249f08
#define display_reset_command_stream                     func_02201102
#define display_stage_clear_work                         func_0220121e
#define display_stage_clear_sprite_tables                func_0220123c
#define display_stage_clear_sprite_scratch               func_0220125a
#define display_stage_reset_sort                         func_02201278
#define display_stage_reset_cursors                      func_022012e0
#define display_stage_build_quads                        func_02201320
#define display_stage_submit_frame                       func_0220134c
#define boot_display_mode_task                           func_02201590
#define boot_scene_director_task                         func_022016ac
#define game_install_director_task                       func_02201b6c
#define task_noop                                        func_02201b8e
#define boot_read_region_input                           func_02201b92
#define scene_bootstrap_task                             func_022027e4
#define scene_zoom_overlay_task                          func_022029a4
#define scene_prop_keyframe_task                         func_02202a28
#define scene_prop_gravity_task                          func_02202ba8
#define scene_initialize_hud_task                        func_02202c5c
#define scene_countdown_task                             func_02202d14
#define scene_prepare_record_bootstrap                   func_02202d7e
#define scene_bootstrap_record_dispatch                  func_02202dda
#define scene_bootstrap_record_select                    func_02202dfa
#define scene_run_task                                   func_02202e84
#define hud_blink_status_task                            func_02203500
#define hud_status_two_players_task                      func_02203576
#define hud_status_one_player_task                       func_02203afc
#define hud_status_config_task                           func_02203c28
#define hud_dispatch_status_buffer                       func_02203ec2
#define scene_sprite_acceleration_task                   func_022041d4
#define scene_sprite_spin_task                           func_02204258
#define scene_sprite_ease_task                           func_02204308
#define hud_blink_join_notice                            func_022044cc
#define attract_update_task                              func_02204868
#define round_join_countdown_task                        func_02204afa
#define round_dual_countdown_task                        func_02205232
#define game_run_scene_step                              func_02205812
#define player_reset_join_slot                           func_0220581a
#define scene_intro_view_task                            func_02206c88
#define camera_frame_near_task                           func_0220b3cc
#define camera_approach_near_task                        func_0220b544
#define camera_seed_template_task                        func_0220b6b2
#define camera_run_mode1_handler                         func_0220b85c
#define camera_frame_medium_task                         func_0220b8d4
#define camera_approach_medium_task                      func_0220ba48
#define camera_run_mode2_handler                         func_0220bdb4
#define camera_frame_far_task                            func_0220be2c
#define camera_approach_far_task                         func_0220bfa4
#define camera_track_two_slots_task                      func_0220c3e0
#define camera_test_zone_turn                            func_0220c858
#define camera_test_zone_bounds                          func_0220cab0
#define camera_acquire_leap_target                       func_0220cb5c
#define camera_acquire_target                            func_0220cca0
#define camera_reset_orbit_offsets                       func_0220d188
#define camera_tick_linear_offset                        func_0220d204
#define camera_patrol_zone_task                          func_0220d310
#define camera_patrol_x_task                             func_0220d424
#define camera_pendulum_task                             func_0220d58c
#define camera_follow_slot_task                          func_0220d70c
#define debug_move_world_pose_task                       func_0220e1e2
#define debug_initialize_world_pose_task                 func_0220e29e
#define debug_orbit_camera_task                          func_0220e2d8
#define camera_integrate_seeded_pose                     func_0220e5bc
#define camera_integrate_velocity_pose                   func_0220e6d4
#define camera_attach_to_source                          func_0220e978
#define camera_ease_source_pose                          func_0220ea06
#define camera_integrate_local_velocity                  func_0220ebde
#define camera_integrate_source_follow                   func_0220ef02
#define camera_copy_source_part_pose                     func_0220f0ec
#define camera_copy_selected_part_pose                   func_0220f0ee
#define camera_copy_player_part_pose                     func_0220f15e
#define camera_copy_pose_spawn_child                     func_0220f19e
#define camera_copy_pose_spawn_flagged_child             func_0220f202
#define camera_spawn_initial_child                       func_0220f26a
#define camera_update_rotated_source                     func_0220f28e
#define camera_update_translated_source                  func_0220f454
#define debug_free_fly_camera                            func_0220f5b8
#define camera_step_current_definition                   func_0220f880
#define task_spawn_root_list                             func_0220fe48
#define actor_clear_frame_pair_slots                     func_022148ca
#define heap_release_owned                               func_02214c66
#define display_radix_sort_records                       func_02214e2c
#define display_radix_sort_entities                      func_02214e2e
#define vdp1_append_frame_headers                        func_02215000
#define sprite_clear_character_tables                    func_022151d8
#define sprite_clear_scratch_heads                       func_02215386
#define model_draw_by_matrix_mode                        func_022154fa
#define vdp1_build_sprite_commands                       func_022156da
#define vdp1_build_sprite_commands_reverse               func_0221582a
#define sprite_emit_or_queue                             func_02215964
#define model_emit_or_queue                              func_02215b34
#define model_build_pending_quad_list                    func_02215bf4
#define audio_queue_indexed_cue                          func_02215f08
#define audio_queue_and_latch                            func_02216018
#define audio_reemit_latched                             func_02216066
#define vdp2_clear_vram                                  func_022161c8
#define view_reset_transform_records                     func_022161dc
#define display_clear_work_heads                         func_02223c20
#define frame_measure_pulse_timing                       func_02223c74
#define isr_vblank_in                                    func_02223cf0
#define display_commit_channel_accumulator               func_022386c6
#define enemy_wave_initialize_task                       func_02246308
#define enemy_wave_spawn_task                            func_022463b8
#define scene_transition_timeline_task                   func_022463c8
#define enemy_wave_test_complete                         func_022465fc
#define enemy_initialize_spawn_task                      func_02246678
#define enemy_update_state_task                          func_022469a0
#define enemy_update_interrupts                          func_02246ab8
#define enemy_update_variant_script                      func_02246d0c
#define enemy_choose_full_response                       func_02246dbc
#define enemy_choose_simple_response                     func_02246f32
#define enemy_enter_state2                               func_022470da
#define enemy_update_state2                              func_02247106
#define enemy_dash_from_second_list                      func_02247530
#define enemy_dash_from_third_list                       func_02247580
#define enemy_update_dash                                func_022475a8
#define enemy_choose_dash_recovery                       func_02247924
#define enemy_update_recovery                            func_02247a8c
#define enemy_choose_engagement                          func_02247e14
#define enemy_dispatch_distance_response                 func_0224806a
#define enemy_update_combat_think                        func_02248104
#define enemy_begin_scripted_response                    func_022482dc
#define enemy_update_scripted_response                   func_02248318
#define enemy_update_turn                                func_02248436
#define enemy_update_grab_probe                          func_0224852c
#define enemy_update_grapple                             func_022485b8
#define enemy_update_carry_throw                         func_0224886e
#define enemy_update_action_commands                     func_02248bb8
#define enemy_update_held_commands_entry                 func_02248f86
#define enemy_update_held_commands                       func_02248f88
#define enemy_try_grab                                   func_022490e4
#define enemy_enter_state8                               func_02249214
#define enemy_update_combo                               func_022492f8
#define enemy_update_grab_lock                           func_02249898
#define enemy_update_wait                                func_02249a04
#define enemy_pick_optional_animation                    func_02249b80
#define enemy_update_clash                               func_02249bba
#define enemy_retract_task                               func_02249da8
#define enemy_enter_random_idle18                        func_02249ea0
#define enemy_update_hit_or_clash                        func_02249f52
#define enemy_update_script_completion                   func_0224a000

/* Pass 3 command/state constants. Names describe observed roles, not sample,
 * enemy species or stage titles. Keep original integer literal types. */
/* 02205c96/02205b20 establish selector zero and play entry two; 0222a5d0/0222a300 suppress saved action restore at scene fifty-two. */
#define DH_SCENE_SELECTOR                                0
/* 02205b20: queued when the scene director reaches its stop entry. */
#define DH_SOUND_SCENE_END_COMMAND                       0xA00001FF
/* 02205c96/02205b20 establish selector zero and play entry two; 0222a5d0/0222a300 suppress saved action restore at scene fifty-two. */
#define DH_SCENE_PLAY_ENTRY                              2
/* 02216018/02216066: latched sound ID shifted by eight, OR class A010. */
#define DH_SOUND_LATCH_BASE                              0xa01000ff
/* 02202ba8: queued once when the gravity prop is initialized. */
#define DH_SOUND_PROP_LAUNCH_COMMAND                     0xa01400ff
/* 022027e4/02218038: queued on scene transition completion. */
#define DH_SOUND_TRANSITION_DONE_COMMAND                 0xa00003ff
/* 0223ed94: queued when its effect child is spawned. */
#define DH_SOUND_EFFECT_SPAWN_COMMAND                    0xA01402FF
/* 02215f08/022160a0/0222aadc: sound ID shifted by eight, OR command class A000. */
#define DH_SOUND_CUE_BASE                                0xA00000FF
/* 02215f24 clears sixteen command words; 02215f6c wraps with mask fifteen. */
#define DH_SOUND_QUEUE_CAPACITY                          16
/* 02215f6c advances the ring write index modulo sixteen. */
#define DH_SOUND_QUEUE_INDEX_MASK                        15
/* 0221603e: scene-gated ID queue uses class A003. */
#define DH_SOUND_SCENE_CUE_BASE                          0xA00300FF
/* 0222adaa/022160a0/0223ffcc: low six bits index actor-type dispatch tables. */
#define DH_ACTOR_TYPE_MASK                               0x3F
/* 0223edce: queued when its parameterized effect child is spawned. */
#define DH_SOUND_EFFECT_VARIANT_COMMAND                  0xA01406FF
/* 02205c96/02205b20 establish selector zero and play entry two; 0222a5d0/0222a300 suppress saved action restore at scene fifty-two. */
#define DH_SCENE_SAVED_ACTION_BYPASS                     52
/* 02215f08/022160a0/0222aadc: sound ID shifted by eight, OR command class A000. */
#define DH_SOUND_CUE_BASE_U                              0xA00000FFU
/* 022301c4: queued after spawning the actor clone. */
#define DH_SOUND_CLONE_COMMAND_U                         0xA0130BFFU
/* 022301c4: queued after spawning the actor clone. */
#define DH_SOUND_CLONE_COMMAND                           0xA0130BFF
/* 0223cbb0: this type shares the seven-live and per-player four-count capacity gates. */
#define DH_EFFECT_TYPE_LIMITED_A                         13
/* 0223cbb0: this type shares the seven-live and per-player four-count capacity gates. */
#define DH_EFFECT_TYPE_LIMITED_B                         14
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_REACTION                       0x11
/* 0223e96c: requested when the actor already faces its target and waits in behavior eighteen. */
#define DH_ANIM_TARGET_WAIT                              0x247
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_POSE                           0x12
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_GUARD                          4
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_DASH                           3
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_SCRIPTED_RESPONSE              6
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_RECOVERY                       5
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_TURN                           7
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_STOP                           0x13
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_GRAPPLE                        10
/* 02248f86/02248f88: requested on the animation-ready transition to held-command phase one. */
#define DH_ANIM_HELD_READY                               0xdd
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_PARENT_LINK                    9
/* 02249836: optional initial pose when beginning combo state thirteen. */
#define DH_ANIM_COMBO_START                              0x162
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_COMBO                          13
/* 022499a6/02249b80: 0xffff means optional animation entry absent. */
#define DH_ANIMATION_ABSENT                              0xFFFF
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_OPTIONAL_POSE                  14
/* Enemy state setters and task dispatchers agree on actor+0xd4; see the correspondingly named enter/update functions. */
#define DH_ENEMY_BEHAVIOR_CLASH                          0xF

/* Pass 3: remaining executive, interrupt and category continuations. */
#define input_settle_injected_buttons                    func_0220fb58
#define smpc_stop_slave_cpu                              func_02214918
#define smpc_start_slave_cpu                             func_02214944
#define actor_build_proximity_tables                     func_02227ca8
#define scene_select_index_task_entry                    func_02205c96
#define scene_credit_join_task_entry                     func_0220600e
#define scene_game_control_task_entry                    func_02206876
#define isr_vblank_out_entry                             func_02223d98
#define isr_timer0_entry                                 func_02223dca
#define effect_project_source_motion                     func_0223a888
#define enemy_select_farther_player_side                 func_02245aec
#define enemy_assign_minimum_cost_requests               func_0223e530
#define enemy_update_category_a_task                     func_0223e5e0
#define enemy_animate_category_a                         func_0223e632
#define enemy_route_category_a_response                  func_0223e690
#define enemy_update_skeletal_category_task              func_0223e878
#define enemy_animate_skeletal_category                  func_0223e8ca
#define enemy_wait_or_turn                               func_0223e96c
#define enemy_ground_slam_task                           func_0223e9d2
#define enemy_update_category_b_task                     func_0223f10e
#define enemy_update_category_c_task                     func_0223f198
#define enemy_update_category_d_task                     func_0223f1ea
#define enemy_animate_category_b                         func_0223f23c
#define enemy_route_category_b_response                  func_0223f29a
#define enemy_arm_distance_band                          func_0223f494
#define scene_waypoint_actor_task                        func_0223f54e
#define scene_actor_script_task                          func_0223f95c
#define enemy_update_category_e_task                     func_0223fc06
#define enemy_animate_category_e                         func_0223fc58
#define enemy_route_collision_response                   func_0223fcb6
#define effect_spawn_template_children                   func_0223fec4
#define actor_dispatch_action_with_effects               func_02240054
#define actor_dispatch_action_preserve_motion            func_022401d6

/* Pass 4: source names retain exact original C linker spellings.
 * _entry preserves raw externs distinct from an existing descriptive definition. */
#define boot_default_noop                                func_02001b8e
#define crc16_buffer                                     func_0220074c
#define task_step_work_x_left_half                       func_0220686a
#define scene_load_background_bank0                      func_02207224
#define background_set_view_projection                   func_022072ce
#define scene_load_descriptor_layer                      func_02207604
#define scene_load_streamed_layer                        func_02207772
#define background_tick_view_transform                   func_02207884
#define scene_load_tiled_layer_with_task                 func_022078aa
#define scene_load_layer_bank1                           func_02207b3e
#define scene_load_line_scroll_layer                     func_02208030
#define scene_load_layer_bank2                           func_0220825c
#define background_tick_perspective_floor                func_022083a0
#define scene_load_shutter_layer                         func_022089d0
#define scene_load_perspective_strip_layer               func_02208b98
#define background_tick_perspective_strips               func_02208c44
#define background_cycle_palette                         func_0220903e
#define background_emit_grayscale_ramp                   func_02209100
#define results_task_dispatch                            func_02209188
#define results_draw_state1_labels                       func_022094c4
#define results_draw_state2_markers                      func_022095cc
#define results_tick_continue_countdown                  func_02209814
#define results_edit_options                             func_02209a8c
#define results_draw_final_stats                         func_02209d70
#define results_draw_state8_stats                        func_0220a0b0
#define results_draw_state9_stats                        func_0220a21c
#define stats_add_counter60                              func_0220a992
#define stats_add_duration_histogram                     func_0220a9c2
#define stats_increment_pair_counters70                  func_0220aa70
#define stats_increment_pair_counters74                  func_0220ab34
#define stats_increment_indexed_counter                  func_0220ac0c
#define debug_print_input_records                        func_0220ade6
#define debug_print_panel_records                        func_0220ae90
#define debug_print_table_record                         func_0220af94
#define task_pool_init_source_alias                      func_0220fc16
#define task_alloc_into_list                             func_0220fcc4
#define task_spawn_numbered_children                     func_0220fd52
#define task_spawn_definition_list                       func_0220fd90
#define task_cancel_word                                 func_0220fe98
#define task_spawn_list_work6c                           func_0220ffae
#define text_write_attributed_cells                      func_022100a2
#define text_write_attributed_block                      func_022100c8
#define display_tick_intensity_ramp                      func_0221039a
#define vec2_magnitude_scaled                            func_0221047c
#define direction_from_yaw_pitch                         func_022104f6
#define orientation_compose_angles                       func_02210690
#define animation_sample_channel                         func_02210876
#define actor_tick_pose_sequence                         func_02210b58
#define actor_tick_thrown_body                           func_02211108
#define actor_tick_debris_emitter                        func_022114b0
#define actor_tick_spin_popup                            func_0221185c
#define actor_tick_animation_sequence                    func_022119a4
#define actor_tick_bouncing_explosive                    func_02211b04
#define actor_tick_orbit_spawner                         func_0221214a
#define actor_tick_scripted_spawner                      func_02212350
#define actor_tick_scattered_projectile                  func_0221279c
#define actor_tick_sway                                  func_02212b20
#define actor_tick_breakable                             func_02212c80
#define actor_tick_rising_effect                         func_02212e00
#define distance_squared_xz                              func_022131d4
#define distance_squared_xyz                             func_022131fc
#define matrix_transform_translation                     func_02213b5c
#define quad_select_depth                                func_02213d90
#define vertex_blend_four                                func_0221400c
#define vector3_blend_four                               func_02214194
#define vector2_blend_four                               func_02214284
#define lz_decompress_buffer                             func_02214328
#define decompressor_noop                                func_022143b8
#define sh2_dmac_copy_unchecked                          func_022144b8
#define sh2_dmac_copy_halfwords                          func_02214510
#define sh2_dmac_copy_aligned_blocks                     func_02214678
#define scu_dma0_transfer                                func_0221478a
#define scu_dma_stack_reset                              func_022147cc
#define scu_dma_stack_push                               func_02214818
#define scu_dma_stack_submit                             func_0221482e
#define heap_resize_allocation                           func_02214bec
#define display_flush_and_toggle_bank                    func_02216530
#define vdp2_copy_shadow_bank                            func_0221654a
#define vdp2_commit_shadow_bank                          func_02216582
#define vdp2_flush_shadow_banks                          func_022165c6
#define display_flush_transfer_banks                     func_0221660c
#define tilemap_blit_biased                              func_02216714
#define tilemap_fill_paged80                             func_0221694e
#define stage_draw_geometry_list                         func_02216ecc
#define stage_build_actor_grid                           func_02216f7a
#define collision_grid_insert_live                       func_02217c10
#define collision_grid_insert_cached                     func_02217ca2
#define prop_tick_triggered_shutter                      func_022182c6
#define prop_tick_spinning_pickup                        func_022186ec
#define prop_tick_scrolling_pair                         func_0221885c
#define prop_tick_swing_pair                             func_0221898c
#define prop_tick_eased_panel                            func_02218bec
#define prop_tick_lateral_sequence                       func_02218eac
#define prop_tick_banked_path                            func_022191ec
#define event_actor_draw_task                            func_02219916
#define camera_tick_sine_sweep                           func_022199da
#define camera_tick_delayed_heading_turn                 func_02219ad2
#define camera_tick_heading_turn                         func_02219ba4
#define prompt_tick_random_accept                        func_02219c44
#define prompt_tick_rotating_buttons                     func_02219f26
#define prompt_tick_blinking_buttons                     func_0221a1a0
#define hud_tick_timer_icon                              func_0221a452
#define prop_tick_triggered_sine_arm                     func_0221a80c
#define prop_tick_pitch_cycle                            func_0221a9e0
#define prop_tick_axis_gate                              func_0221ab1c
#define sound_tick_timeline_cues                         func_0221ad06
#define scene_bootstrap_dispatch_4550                    func_0221b080
#define evt_actor_anim_physics_step                      func_0221b0a2
#define prop_fade_in_sweep                               func_0221b450
#define setpiece_director                                func_0221bf72
#define prop_open_close_sweep                            func_0221c4ac
#define boss_intro_director                              func_0221c690
#define debris_ballistic_update                          func_0221c8f2
#define evt_actor_spawn_4state                           func_0221ca1e
#define evt_actor_spawn_wait100                          func_0221ccf8
#define scene_bootstrap_dispatch_4b54                    func_0221ce1c
#define evt_actor_spawn_flag1_once                       func_0221ce3e
#define evt_actor_spawn_sfx66                            func_0221ce6e
#define evt_actor_spawn_sfx40                            func_0221cee8
#define obj_retract_step                                 func_0221d176
#define evt_actor_spawn_respawn                          func_0221d4fc
#define launch_arc_segment                               func_0221d6b6
#define coop_lever_director                              func_0221e256
#define evt_actor_fall_proximity_step                    func_0221e8c8
#define evt_proximity_spawn                              func_0221e9c6
#define scene_bootstrap_dispatch_51c0                    func_0221eac0
#define evt_actor_spawn_notify_sfx                       func_0221eae2
#define evt_actor_spawn_notify                           func_0221eb62
#define evt_actor_spawn_child                            func_0221f004
#define evt_sequence_step                                func_0221f088
#define evt_actor_spawn_wait11                           func_0221f1d6
#define scene_bootstrap_dispatch_52b0                    func_0221f240
#define evt_actor_spawn_respawn_b                        func_0221f262
#define draw_model_list                                  func_0221fd7a
#define cam_aim_update                                   func_0221fed4
#define spawn_entry_list                                 func_02220190
#define vdp2_shadow_setbit                               func_02221354
#define vdp2_shadow_pack6_args                           func_0222184c
#define actor_combo_reset                                func_022275e4
#define players_entities_ready                           func_02228134
#define actor_substate_init                              func_02228536
#define actor_step_launch                                func_022289b8
#define actor_step_gravity                               func_02228a0e
#define actor_state_1A_enter                             func_02228d44
#define actor_step_grab_follow                           func_02228e0c
#define actor_hit_state_step                             func_02229488
#define player_join_arm                                  func_02229b60
#define player_join_commit_fsm                           func_02229c00
#define player_join_next                                 func_02229df4
#define player_actor_anim_step                           func_0222a418
#define actor_anim_init_task                             func_0222a970
#define actor_anim_step_task                             func_0222a9fe
#define actor_hit_countdown                              func_0222ac60
#define actor_facing_anim_select                         func_0222ae40
#define actor_jump_anim_select                           func_0222baf8
#define actor_state4_enter                               func_0222bc80
#define player_input_anim_select                         func_0222bdf2
#define actor_fall_setup                                 func_0222c322
#define rescale_active_slots                             func_0222ea94
#define actor_anim_step_task_source_alias                func_0222f4e8
#define projectile_track_init_task                       func_0222f640
#define projectile_phase_track_task                      func_0222f6a8
#define slide_projectile_fire_task                       func_0222fa20
#define spark_projectile_slide_task                      func_0222fb90
#define spark_projectile_fire_task                       func_0222fc02
#define projectile_spark_arc_task                        func_0222fd54
#define projectile_seed_table_task                       func_0222ff40
#define projectile_fire_seed_task                        func_02230006
#define projectile_arc_task                              func_0223026a
#define projectile_seed_task                             func_02230434
#define projectile_emit_task                             func_022304b4
#define effect_scatter_task                              func_02230f74
#define effect_prescatter_task                           func_0223101c
#define obj_expand_draw4                                 func_0223115c
#define projectile_track_task                            func_0223144a
#define projectile_launch_dual_task                      func_0223152c
#define projectile_launch_task                           func_022317c0
#define projectile_fly_task                              func_02231864
#define attach_follow_task                               func_02231a1c
#define launched_actor_task                              func_02231b14
#define projectile_emitter_task                          func_02231c30
#define rising_split_projectile_task                     func_02231db8
#define effect_spawner_task                              func_02231fd4
#define particle_fall_task                               func_022320d4
#define muzzle_spark_task                                func_02232178
#define trail_spark_task                                 func_02232294
#define homing_follow_task                               func_0223236c
#define debris_arc_draw_task                             func_02232526
#define stage_intro_task_b                               func_02232bc4
#define evt_timed_dispatch                               func_02233122
#define actor_task_setup_dispatch                        func_0223346c
#define video_menu_bootstrap_sm                          func_0223358c
#define vdp2_stage_loader                                func_02234984
#define gfx_submit_box8                                  func_02234aae
#define actor_draw_11seg                                 func_02235f0c
#define cam_projectile_task                              func_02236246
#define debug_obj_inspector                              func_022363e4
#define video_menu_bootstrap_sm2                         func_0223673a
#define debug_cursor_editor                              func_02236af0
#define video_menu_bootstrap_sm3                         func_02236e66
#define obj_anim_step                                    func_02238e94
#define effect_flash_scale_task                          func_02238f46
#define effect_pulse_scale_task                          func_02238fdc
#define spin_projectile_handler                          func_02239082
#define projectile_toss_handler                          func_0223938c
#define draw_rot_sprite_source_alias                     func_0223967a
#define projectile_arc_handler                           func_022397c2
#define spread_projectile_handler                        func_02239868
#define actor_aim_transform_draw                         func_022399bc
#define actor_state_release_gen                          func_0223a3a2
#define actor_state_gen_hitreact                         func_0223a740
#define obj_state_machine_8                              func_0223bf86
#define pickup_slot_task                                 func_0223c650
#define effect_fall_task                                 func_0223d1b6
#define effect_tumble_task                               func_0223d2a0
#define both_players_special_trigger                     func_0223de20
#define hud_score_slots_init                             func_0223e2ea
#define aim_effect_spawn                                 func_0223e3d0
#define effect_spin_rise_task                            func_0223eef0
#define chain_hit_task                                   func_0223f054
#define actor_motion_init_source_alias                   func_022415ee
#define actor_sound_emit_dir                             func_02241822
#define actor_sound_emit_fixed                           func_02241868
#define actor_deactivate_if_active_source_alias          func_022418a0
#define actor_pose_hold_state                            func_02242272
#define aabb_overlap_box                                 func_02244874
#define aabb_overlap_box_b                               func_022448d0
#define slot_record_reset                                func_02244d40
#define actor_scan_pair_slots_simple                     func_02245f48
#define shc_signfill_rotl7                               func_0224a4aa
#define shc_signfill_rotl5                               func_0224a4d4
#define shc_signfill_rotl4                               func_0224a4e6
#define shc_signfill_rotl3                               func_0224a4f6
#define scene_tick_staged_transfers                      func_0221b5a0
#define scene_tick_spawn_sound_intro                     func_0221b730
#define scene_tick_branch_drain                          func_0221b8e4
#define scene_tick_timed_child_sequence                  func_0221bb40
#define scene_tick_two_trigger_sequence                  func_0221bd08
#define scene_spawn_list_once                            func_0221bdce
#define prop_tick_decelerating_descent                   func_0221c2aa
#define event_spawn_matching_frame                       func_0221c8a0
#define debris_ballistic_update_source_alias             func_0221c8f4
#define scene_tick_double_branch_sequence                func_0221cad4
#define scene_tick_spawn_count20                         func_0221cd58
#define event_spawn_sequence_draft                       func_0221cfa4
#define scene_tick_teardown_timer                        func_0221d302
#define scene_tick_selected_enemy_gate                   func_0221d59c
#define prop_tick_cued_animation                         func_0221d910
#define scene_tick_view_selected_branch                  func_0221da60
#define prop_tick_attached_marker                        func_0221db6e
#define scene_tick_thirteen_phase_setup                  func_0221dc04
#define scene_tick_sound_pair62                          func_0221debc
#define scene_tick_escalating_spawn_cycles               func_0221df80
#define camera_tick_dual_sine_orbit                      func_0221e7d6
#define scene_tick_periodic_random_spawn                 func_0221e844
#define event_spawn_dispatch_draft                       func_0221ec08
#define event_gate_branch_draft                          func_0221edec
#define scene_tick_waypoint_model_intro                      func_0221f9c4
#define model_draw_segment_list_task                     func_0221fe00
#define camera_tick_waypoint_path                        func_0221ffb0
#define display_tick_strip_upload                        func_022201d8
#define prop_tick_preset_launch                          func_022202ca
#define camera_tick_sound_timeline                       func_022203ce
#define prop_tick_weighted_path_launch                   func_022205fc
#define prop_tick_phased_rotation                        func_022206ae
#define effect_tick_ballistic_sprite_source_alias        func_0222077e
#define effect_tick_ballistic_sprite                     func_0222078c
#define vdp2_set_transform_slot0                         func_02221460
#define vdp2_set_plane_nibble                            func_02221800
#define vdp2_reset_transform_block                       func_02221ffa
#define clear_word_region                                func_022220b0
#define vdp2_enable_shadow_bit5                          func_022223b2
#define vdp2_apply_packed_commands                       func_02222494
#define display_set_priority_origin                      func_022227aa
#define perspective_build_edge_samples                   func_02222c08
#define text_clear_work_buffer                           func_02222ecc
#define text_upload_work_buffers                         func_02222efc
#define text_flush_small_line                            func_02222f16
#define text_flush_large_line                            func_02222f34
#define text_printf_small_draft                          func_02223142
#define text_printf_large_draft                          func_02223460
#define text_format_context_init                         func_02223780
#define interaction_append_slot_records                  func_02225fac
#define callback_publish_noop                            func_02226134
#define published_callback_noop                          func_02226144
#define interaction_advance_frame_pair                   func_02227602
#define actor_system_construct                           func_02227790
#define player_initialize_task                           func_02228508
#define player_tick_grab_commands                        func_022286cc
#define player_tick_event_to_state9                      func_02228738
#define player_tick_fall_settle                          func_02228768
#define player_tick_event_to_state6                      func_0222881a
#define player_tick_damage_recovery                      func_02228874
#define player_tick_landing_recovery                     func_022288e8
#define player_tick_hit_recovery                         func_02228aa6
#define player_enter_input_recovery                      func_02228b38
#define player_tick_input_recovery                       func_02228b68
#define player_tick_restore_idle                         func_02228bb6
#define player_tick_timed_hit_reaction                   func_02228c70
#define player_tick_loop_to_idle                         func_02228ca2
#define player_tick_loop_to_idle_variant                 func_02228ca4
#define player_tick_grab_pull_x                          func_02228cd6
#define player_tick_turn_recovery                        func_02228d9c
#define player_tick_grab_home_contact                    func_02228e60
#define player_tick_pose_loop_to_idle                    func_02228fe2
#define player_tick_loop_to_state17                      func_02229054
#define player_tick_hit_grab_body                        func_022294b4
#define player_state_noop                                func_02229640
#define player_tick_timed_action_response                func_02229644
#define player_tick_animation_idle_reset                 func_02229688
#define player_tick_timed_action_response_b              func_02229714
#define player_tick_timed_action_response_c              func_02229758
#define player_tick_timed_release                        func_0222979c
#define player_tick_timed_pair_response                  func_022297d8
#define player_tick_timed_effect_response                func_02229816
#define player_tick_timed_action_finish                  func_02229854
#define player_tick_loop_special_pose                    func_0222988e
#define player_tick_join_commit                          func_02229bb0
#define player_enter_join_countdown_source_alias         func_02229bce
#define players_select_health_leader                     func_0222a04e
#define players_publish_health_order                     func_0222a0ec
#define player_select_grounded_animation                 func_0222b0fe
#define player_select_block_animation                    func_0222b1e2
#define player_select_launch_animation                   func_0222b2a8
#define player_select_hitstun_animation                  func_0222b42c
#define player_select_knockback_animation                func_0222b630
#define player_init_thrown_physics                       func_0222bd90
#define player_scan_grab_target                          func_0222bfc2
#define player_handle_grab_throw_commands                func_0222c39c
#define pose_blend_rotation_matrices                     func_0222e3b0
#define animation_cell_reset                             func_0222eb72
#define animation_cell_init                              func_0222eb84
#define animation_cell_tick                              func_0222eba8
#define animation_cell_upload_frame                      func_0222ec3c
#define effect_tick_lifetime_or_owner_flags43800         func_0222f3ea
#define effect_tick_lifetime_or_owner_flagsb800          func_0222f40e
#define effect_tick_lifetime_or_owner_flags3800          func_0222f432
#define effect_tick_lifetime                             func_0222f456
#define effect_init_owner_frame_track                    func_0222f470
#define effect_init_owner_frame_variant                  func_0222f84e
#define effect_cancel_on_timer5a                         func_022301ac
#define projectile_tick_owner_phase                      func_0223033e
#define projectile_tick_owner_phase_finish               func_022303a6
#define spark_tick_child_spawn                           func_022305e2
#define projectile_init_attack_variant_a                 func_022306c4
#define projectile_tick_attack_variant_a                 func_022308a0
#define projectile_init_attack_variant_b                 func_02230ae0
#define projectile_tick_arc_to_burst                     func_02230cdc
#define effect_tick_explosion_debris                     func_02230e34
#define effect_tick_expanding_frames                     func_02231336
#define projectile_init_tracking_launch                  func_02231382
#define effect_arm_follow_without_copy                   func_022319b2
#define effect_cancel_task                               func_02231b0e
#define projectile_tick_linear_lifetime                  func_02231d72
#define effect_tick_source_attachment                    func_022324da
#define scene_tick_round_intro                           func_02232610
#define prop_tick_short_rotation                         func_02233178
#define vdp2_init_menu_graphics                          func_022348cc
#define hud_tick_eight_meter_slots                       func_022371c0
#define hud_tick_player_popups                           func_02237ea6
#define hud_draw_indexed_label                           func_022380fa
#define hud_load_indexed_panel                           func_0223828c
#define hud_draw_level_record                            func_02238364
#define sound_emit_owner_animation_cue                   func_022383f8
#define sound_step_command_cursor                        func_0223845c
#define sound_tick_slot_sequence                         func_022384e4
#define effect_tick_depth_clip                           func_0223875c
#define effect_finish_owner_action                       func_022387e0
#define effect_spawn_owner_variant_a                     func_022387fc
#define effect_spawn_owner_variant_b                     func_022388b8
#define effect_spawn_five_table_children                 func_02238960
#define effect_spawn_three_table_children                func_02238a9c
#define effect_tick_reference_debris                     func_02239194
#define effect_tick_frame_relative_sprite                func_02239462
#define projectile_tick_directional_arc                  func_022396e8
#define actor_arm_effect_with_motion                     func_0223ab04
#define actor_enter_retarget_aim                         func_0223af0c
#define effect_tick_launched_slot                        func_0223b28a
#define effect_tick_spinning_slot                        func_0223b5d8
#define effect_tick_bouncing_slot                        func_0223b9b8
#define effect_tick_tumbling_slot                        func_0223bcd4
#define actor_step_effect_transition                     func_0223c7f6
#define effect_retire_finished_slots                     func_0223cd98
#define actor_attach_effect_slot                         func_0223cf70
#define effect_tick_ballistic_collision                  func_0223d3d0
#define effect_tick_landing_sound                        func_0223d558
#define projectile_tick_grenade_fuse                     func_0223d9dc
#define effect_tick_scaled_tumbling                      func_0223db2a
#define both_players_special_trigger_source_alias        func_0223de0e
#define aim_effect_spawn_source_alias                    func_0223e3ba
#define angle_difference_nonnegative_halfturn            func_0223e5bc
#define scu_timer0_set_short_interval                    func_0223e5cc
#define effect_spawn_jittered_spark                      func_0223eb3c
#define effect_tick_jittered_emitter                     func_0223ec44
#define effect_tick_short_sprite                         func_0223ed14
#define effect_spawn_eight_frame_flash                   func_0223ed94
#define effect_spawn_rising_flash                        func_0223edce
#define effect_tick_even_frame_emitter                   func_0223ee1a
#define effect_tick_twelve_frame_arc                     func_0223ef5a
#define effect_tick_eight_frame_owner_flash              func_0223efa0
#define effect_tick_eight_frame_flash                    func_0223efec
#define enemy_action_init_motion                         func_0224043c
#define enemy_action_init_phase                          func_02240492
#define enemy_action_set_special_or_expire               func_022404ce
#define enemy_action_face_recorded_motion                func_02240588
#define enemy_action_signal_and_expire                   func_02240612
#define enemy_action_select_descriptor_motion            func_02240622
#define enemy_action_init_boosted_motion                 func_022407b0
#define enemy_action_dispatch_special_state              func_0224081e
#define enemy_action_resolve_descriptor_facing           func_022408a0
#define enemy_action_walk_or_face3                       func_022409a0
#define enemy_action_update_then_face                    func_02240a4a
#define enemy_action_force_face                          func_02240a7a
#define enemy_action_face_recorded_angle                 func_02240ae0
#define enemy_action_signal_then_face                    func_02240b2e
#define enemy_action_update_random_face                  func_02240b5a
#define enemy_action_force_random_face                   func_02240b8a
#define enemy_action_recorded_random_face                func_02240bba
#define enemy_action_signal_random_face                  func_02240c3e
#define enemy_action_update_direction_face               func_02240c6a
#define enemy_action_force_direction_face                func_02240c9a
#define enemy_action_signal_direction_face               func_02240cca
#define enemy_action_select_hit_kind                     func_02240d1c
#define enemy_action_walk_or_face_variant                func_02240e54
#define enemy_action_walk_or_face_alternate              func_02240efe
#define enemy_action_enter_statewalk                     func_02240fd8
#define enemy_action_walk_or_face6                       func_02241034
#define enemy_action_seed_motion_vector                  func_0224110e
#define actor_face_direction_code                        func_022417ea
#define actor_spawn_aimed_shot                           func_02241a10
#define enemy_tick_state_table_a                         func_02241b9c
#define enemy_tick_state_table_b                         func_02241bee
#define enemy_tick_state_table_c                         func_02241c40
#define enemy_enter_configured_step                      func_02241d18
#define enemy_enter_timed_state16                        func_02241d6e
#define enemy_enter_descriptor_state16                   func_02241e06
#define enemy_tick_chase_phases                          func_02241eb0
#define enemy_restore_saved_pose                         func_02242226
#define enemy_tick_launch_wrapper                        func_02242410
#define enemy_tick_launched_body                         func_022424c4
#define projectile_tick_armed_bomb                       func_022425d0
#define effect_tick_armed_puff                           func_022428bc
#define actor_tick_motion_modes                          func_02242a10
#define actor_dispatch_motion_mode                       func_02242cb8
#define actor_request_animation47e                       func_02242f9c
#define prop_release_to_panel_handler                    func_02242fb0
#define enemy_can_lunge_target                           func_02242fde
#define enemy_tick_grab_throw                            func_02243066
#define enemy_tick_grabbed_overlay                       func_022433e8
#define projectile_spawn_velocity_burst                  func_022434ae
#define actor_spawn_falling_pickup                       func_02243590
#define enemy_commit_target_approach                     func_02243be0
#define enemy_init_jump_lane                             func_02243d78
#define enemy_commit_dodge_lane                          func_02243db4
#define enemy_setup_lateral_approach                     func_02243e22
#define enemy_dispatch_counter_reaction                  func_02244670
#define enemy_classify_facing_agreement                  func_022447d4
#define actor_slot_set_mode7                             func_02244d6a
#define actor_slot_set_mode6                             func_02244dc8
#define enemy_select_latched_attack                      func_02244e54
#define enemy_select_mode_attack                         func_02244fdc
#define enemy_try_latched_attack                         func_0224519e
#define actors_test_directional_range                    func_02245382
#define enemy_choose_and_start_motion                    func_022454e4
#define enemy_choose_action_and_side                     func_02245632
#define enemy_select_pending_reaction                    func_022458ec
#define enemy_select_mode_reference                      func_02245b48
#define enemy_test_reference_facing                      func_02245bcc
#define enemy_load_action_descriptor_source_alias        func_02245c76
#define enemy_elapsed_action_threshold                   func_02246026
#define enemy_selector_zero                              func_0224617a
#define enemy_choose_random_direction                    func_02246180
#define player_get_active_work                           func_022461d8
#define model_task_init_packed_id                        func_02246254
#define model_task_draw_work                             func_0224627e
#define effect_tick_blink_countdown                      func_022462d0
#define shc_shift_left15                                 func_0224a39c
#define shc_shift_left23                                 func_0224a3b0
#define shc_shift_left27                                 func_0224a3c4
#define shc_mask_rotate_right4                           func_0224a3d2
#define shc_mask_rotate_right3                                func_0224a3e0
#define shc_mask_rotate_right2                                func_0224a3ec
#define shc_mask_rotate_right1                           func_0224a3f6
#define shc_swap_and_signfill                            func_0224a49e
#define shc_rotate_left6_signfill                        func_0224a4c0
#define shc_rotate_left2_signfill                        func_0224a504
#define shc_shift_sled_negative_one                      func_0224a510
#define shc_double_subtract                              func_0224a518
#define enemy_action_init_boosted_motion_signalled       func_0224080e
#define enemy_action_dispatch_special_state_signalled    func_0224086c
#define enemy_action_walk_or_face3_signalled             func_02240a3a
#define enemy_action_select_hit_kind_signalled           func_02240e0e
#define enemy_action_walk_or_face_variant_signalled      func_02240eee
#define enemy_action_walk_or_face_alternate_signalled    func_02240fc8
#define enemy_action_enter_statewalk_signalled           func_02241024
#define enemy_action_walk_or_face6_signalled             func_022410fe

/* Pass 4: state/command constants retain original integer literal types. */
#define DH_PLAYER_HANDLER_IDLE                                 1
#define DH_PLAYER_HANDLER_HIT_RESPONSE                         9
#define DH_PLAYER_HANDLER_SPECIAL_POSE                         13
#define DH_PLAYER_HANDLER_TIMED_RECOVERY                       0xA
#define DH_PLAYER_HANDLER_DEFEATED                             0x1F
#define DH_PLAYER_HANDLER_EVENT_FOLLOWUP                       6
#define DH_PLAYER_HANDLER_JOIN_COUNTDOWN                       0x20
#define DH_PLAYER_HANDLER_LANDED                               5
#define DH_PLAYER_HANDLER_HIT_REACTION                         14
#define DH_ANIMATION_CONTROL_RESTART                           1
#define DH_ANIMATION_CONTROL_BLENDING                          2
#define DH_ANIMATION_CONTROL_ADVANCE_LOOP                      4
#define DH_EFFECT_ACTION_ENTER_SECONDARY                       0x4F
#define DH_EFFECT_ACTION_REFRESH_SECONDARY                     0x50
#define DH_EFFECT_ACTION_SWAP_CHANNELS                         0x51
#define DH_EFFECT_ACTION_SET_PRIMARY                           0x52
#define DH_EFFECT_ACTION_SET_SECONDARY                         0x53
#define DH_PAD_ACTIONS_DEC                                     7
#define DH_SOUND_EVENT_PAIR_BEGIN_COMMAND                      0xA01206FFU
#define DH_SOUND_EVENT_PAIR_END_COMMAND                        0xA01207FFU
#define DH_SOUND_EVENT_SECOND_PAIR_BEGIN_COMMAND               0xA01208FFU
#define DH_SOUND_EVENT_SECOND_PAIR_END_COMMAND                 0xA01209FFU
#define DH_SOUND_ANIMATED_PROP_BEGIN_A_COMMAND                 0xa0120aff
#define DH_SOUND_ANIMATED_PROP_BEGIN_B_COMMAND                 0xa0120cff
#define DH_SOUND_ANIMATED_PROP_VARIANT_COMMAND                 0xa0120eff
#define DH_SOUND_ACTOR_FIXED_CUE_COMMAND                       0xA01318FF
#define DH_SOUND_CAMERA_TIMELINE_A_COMMAND                     0xA01407FF
#define DH_SOUND_CAMERA_TIMELINE_B_COMMAND                     0xA01404FF
#define DH_SOUND_CAMERA_TIMELINE_C_COMMAND                     0xA01405FF
#define DH_SOUND_CAMERA_TIMELINE_D_COMMAND                     0xA01401FF
#define DH_SOUND_EXPLOSION_BURST_COMMAND                       0xA01403FF
#define DH_SCENE_SEGMENTED_MODEL_INTRO                         13
#define DH_MODEL_DEFINITION_STRIDE                             0x14

/* Pass 4: raw externs and local reset helper retain original symbols. */
#define view_bounds_from_actors_raw_entry                      func_0220d9ec
#define store_angle_between_raw_entry                          func_022107dc
#define matrix_rotate_transform_vector_raw_entry               func_0221080a
#define copy_longs_raw_entry                                   func_0221326c
#define zero_longs_raw_entry                                   func_0221327c
#define fill_vec4_array_raw_entry                              func_022132ac
#define matrix_copy_rowswap_raw_entry                          func_02213434
#define matrix_copy_reverse_raw_entry                          func_02213474
#define matrix_copy_reverse_to_raw_entry                       func_022134d8
#define vertex_project_buffered_source_entry                   func_022137a0
#define project_quad_corners_raw_entry                         func_02213a70
#define load_matrix_default_raw_entry                          func_02213fb4
#define matrix_rotate_zyx_raw_entry                            func_022149e4
#define matrix_push_duplicate_raw_entry                        func_02214a2e
#define matrix_transform_vector_raw_entry                      func_02214a50
#define matrix_transform_angle_vector_raw_entry                func_02214a60
#define matrix_clip_test_raw_entry                             func_02215630
#define view_reset_one_transform_record                        func_02216258
#define collision_release_slot_raw_entry                       func_022170b4
#define event_select_spawn_record_interior                     func_0221dfaa
#define event_select_branch_pair_interior                      func_0221fdca
#define actor_tieup_task_raw_entry                             func_02226148
#define actor_spawn_list_tick_raw_entry                        func_0222a120
#define effect_owner_frame_continuation                        func_0222f8d0
#define actor_init_from_config_raw_entry                       func_02244988
#define shc_signed_divide_raw_entry                            func_0224a06c
#define shc_double_to_int_raw_entry                            func_0224a7a8
#define shc_int_to_double_raw_entry                            func_0224a830
#define shc_double_multiply_shared_entry                       func_0224a928

#endif /* DH_NAMES_H */
