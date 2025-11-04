#include "main/console.hpp"

#include "cseries/cseries_events.hpp"
#include "game/game_time.hpp"
#include "hs/hs_compile.hpp"
#include "input/input_abstraction.hpp"
#include "interface/c_controller.hpp"
#include "interface/debug_menu/debug_menu_main.hpp"
#include "interface/gui_custom_bitmap_storage.hpp"
#include "interface/terminal.hpp"
#include "main/debug_keys.hpp"
#include "main/main.hpp"
#include "main/main_render.hpp"
#include "memory/module.hpp"
#include "multithreading/threads.hpp"
#include "networking/tools/remote_command.hpp"
#include "profiler/profiler.hpp"
#include "render/render_cameras.hpp"
#include "shell/shell.hpp"
#include "sound/sound_manager.hpp"
#include "text/draw_string.hpp"
#include "xbox/xbox.hpp"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

HOOK_DECLARE(0x00605E10, console_execute_initial_commands);

s_console_globals console_globals;

bool console_dump_to_debug_display = false;

c_static_string<256> console_token_buffer;
int32 suggestion_current_index;

s_status_line* g_status_line_head = NULL;
s_status_line* g_status_line_tail = NULL;

s_status_string g_status_strings[20];

s_status_line::s_status_line() :
	text(),
	flags()
{

}

s_status_string::s_status_string() :
	format_string(),
	line()
{
}

s_string_cache::s_string_cache() :
	string()
{
}

FILE* console_open_init_by_application_type(e_shell_application_type application_type)
{
	FILE* file = NULL;
	c_static_string<256> file_name;

	if (application_type == _shell_application_tool)
	{
		e_shell_tool_type tool_type = shell_tool_type();
		if (tool_type == _shell_tool_interactive)
		{
			file_name.print("guerilla_init.txt");
		}
		else if (tool_type == _shell_tool_command_line)
		{
			file_name.print("tool_init.txt");
		}
	}
	else if (application_type == _shell_application_editor)
	{
		file_name.print("editor_init.txt");
	}
	else
	{
		file_name.print("init.txt");
	}

	fopen_s(&file, file_name.get_string(), "r");
	return file;
}

FILE* console_open_init()
{
	return console_open_init_by_application_type(shell_application_type());
}

// regex for identifying undefined hs globals
// `([\w\d_]+)(?=,\n\t_hs.*,\n\t(?:NULL,\s*|\s*) \/\/ \$TODO)`
// not perfect, but gets most of them

const int32 k_hs_undefined_global_count = 809;
const char* k_undefined_hs_global_names[k_hs_undefined_global_count] =
{
	"display_pulse_rates",
	"display_throttle_rates",
	"display_lag_times",
	"display_frame_deltas",
	"unknownA",
	"framerate_infinite",
	"framerate_debug",
	"framerate_use_system_time",
	"framerate_stabilization",
	"unknown11",
	"unknown12",
	"debug_physical_memory",
	"events_debug_spam_render",
	"flying_camera_has_collision",
	"flying_camera_use_old_controls",
	"editor_director_mouse_wheel_speed_enabled",
	"async_display_statistics",
	"suppress_upload_debug",
	"lightmap_pointsample",
	"debug_no_frustum_clip",
	"debug_bink",
	"recover_saved_games_hack",
	"game_state_verify_on_write",
	"game_state_verify_on_read",
	"output_bad_pda_model_info",
	"debug_globals_empty",
	"dont_load_material_effects",
	"dont_load_lightmaps",
	"editor_strip_dialogue_sounds",
	"scenario_load_fast",
	"scenario_load_fast_and_playable",
	"prune_global_use_empty",
	"prune_scenario_material_effects",
	"prune_scenario_lightmaps",
	"prune_scenario_add_single_bsp_zones",
	"prune_scenario_force_single_bsp_zone_set",
	"prune_scenario_for_environment_editing",
	"prune_scenario_force_solo_mode",
	"prune_scenario_for_environment_editing_keep_cinematics",
	"prune_scenario_for_environment_editing_keep_scenery",
	"prune_scenario_use_gray_shader",
	"prune_global_dialog_sounds",
	"prune_global",
	"prune_global_keep_playable",
	"prune_error_geometry",
	"debug_structure_sampling",
	"display_precache_progress",
	"log_precache_progress",
	"fake_precache_percentage",
	"force_aligned_utility_drive",
	"debug_object_garbage_collection",
	"debug_object_dump_log",
	"allow_all_sounds_on_player",
	"disable_player_rotation",
	"player_rotation_scale",
	"debug_player_respawn",
	"g_synchronous_client_maximum_catchup_chunk_size",
	"g_editor_maximum_edited_object_speed",
	"g_editor_edited_object_spring_constant",
	"g_editor_maximum_rotation_speed",
	"chud_cortana_debug",
	"debug_unit_all_animations",
	"debug_unit_animations",
	"debug_unit_illumination",
	"debug_unit_active_camo_frequency_modulator",
	"debug_unit_active_camo_frequency_modulator_bias",
	"debug_player_melee_attack",
	"debug_boarding_force_enemy",
	"enable_animation_influenced_flight",
	"enable_flight_noise",
	"enable_player_transitions",
	"disable_node_interpolation",
	"disable_analog_movement",
	"disable_transition_animations",
	"unknown74",
	"unknown75",
	"debug_effects_nonviolent",
	"debug_effects_locations",
	"effects_enable",
	"debug_effects_allocation",
	"debug_effects_play_distances",
	"debug_effects_lightprobe_sampling",
	"player_effects_enabled",
	"unknown7E",
	"sound_direct_path_gain",
	"unknown82",
	"unknown83",
	"unknown84",
	"unknown85",
	"unknown86",
	"unknown87",
	"unknown88",
	"unknown89",
	"unknown8A",
	"unknown8B",
	"unknown8C",
	"unknown8F",
	"unknown90",
	"unknown91",
	"unknown92",
	"unknown93",
	"unknown94",
	"unknown95",
	"unknown97",
	"unknown98",
	"unknown99",
	"unknown9A",
	"unknown9B",
	"unknown9C",
	"unknown9D",
	"collision_log_time",
	"debug_object_scheduler",
	"render_debug_cache_state",
	"render_environment",
	"render_objects",
	"visibility_debug_use_old_sphere_test",
	"render_lightmap_shadows_apply",
	"render_lights",
	"render_first_person",
	"render_debug_mode",
	"render_debug_safe_frame_bounds",
	"render_debug_colorbars",
	"render_debug_transparents",
	"render_debug_slow_transparents",
	"render_transparents",
	"render_debug_transparent_cull",
	"render_debug_transparent_cull_flip",
	"render_debug_lens_flares",
	"render_instanced_geometry",
	"render_sky",
	"render_lens_flares",
	"render_decorators",
	"light_decorators",
	"render_decorator_bounds",
	"render_decorator_spheres",
	"render_decorator_bsp_test_offset_scale_parameter",
	"render_muffins",
	"render_debug_muffins",
	"render_debug_force_cinematic_lights",
	"render_atmosphere_cluster_blend_data",
	"render_debug_display_command_buffer_data",
	"render_debug_screen_shaders",
	"render_debug_screen_effects",
	"render_screen_effects",
	"render_debug_save_surface",
	"render_disable_screen_effects_not_first_person",
	"render_exposure_stops",
	"display_exposure",
	"render_disable_prt",
	"force_render_lightmap_mesh",
	"screenshot_anisotropic_level",
	"screenshot_gamma",
	"render_light_intensity",
	"render_light_clip_planes",
	"render_light_opaque",
	"render_debug_cloth",
	"render_debug_antennas",
	"render_debug_leaf_systems",
	"render_debug_object_lighting",
	"render_debug_object_lighting_refresh",
	"render_debug_use_cholocate_mountain",
	"render_debug_object_lighting_use_scenery_probe",
	"render_debug_object_lighting_use_device_probe",
	"render_debug_object_lighting_use_air_probe",
	"render_debug_infinite_framerate",
	"render_debug_show_4x3_bounds",
	"render_weather_bounds",
	"render_debug_cinematic_clip",
	"render_buffer_gamma",
	"render_screen_gamma",
	"render_buffer_gamma_curve",
	"render_color_balance_red",
	"render_color_balance_green",
	"render_color_balance_blue",
	"render_black_level_red",
	"render_black_level_green",
	"render_black_level_blue",
	"debug_volume_samples",
	"decal_create",
	"decal_frame_advance",
	"decal_render",
	"decal_render_debug",
	"decal_render_latest_debug",
	"decal_cull",
	"decal_fade",
	"decal_dump",
	"decal_z_bias",
	"decal_slope_z_bias",
	"particle_create",
	"particle_frame_advance",
	"particle_render",
	"particle_render_debug",
	"particle_render_debug_spheres",
	"particle_render_debug_emitters",
	"particle_cull",
	"particle_dump",
	"particle_force_cpu",
	"particle_force_gpu",
	"unknown156",
	"unknown157",
	"unknown158",
	"effect_priority_cutoff",
	"weather_occlusion_max_height",
	"render_method_debug",
	"render_debug_viewport_scale",
	"render_debug_light_probes",
	"effect_property_culling",
	"contrail_create",
	"contrail_pulse",
	"contrail_frame_advance",
	"contrail_submit",
	"contrail_dump",
	"light_volume_create",
	"light_volume_frame_advance",
	"light_volume_submit",
	"light_volume_dump",
	"beam_create",
	"beam_frame_advance",
	"beam_submit",
	"beam_dump",
	"debug_inactive_objects",
	"pvs_building_disabled",
	"visibility_debug_portals",
	"visibility_debug_audio_clusters",
	"visibility_debug_portals_structure_bsp_index",
	"visibility_debug_portals_cluster_index",
	"error_geometry_draw_names",
	"error_geometry_tangent_space",
	"error_geometry_lightmap_lights",
	"debug_objects_functions_all",
	"debug_objects_unit_lipsync",
	"debug_objects_unit_lipsync_verbose",
	"debug_objects_unit_emotion",
	"debug_objects_biped_melee_in_range",
	"debug_objects_devices",
	"debug_objects_machines",
	"debug_objects_spawn_timers",
	"debug_objects_freeze_ragdolls",
	"debug_objects_disable_relaxation",
	"debug_objects_sentinel_physics_ignore_lag",
	"debug_objects_ignore_node_masks",
	"debug_objects_force_awake",
	"debug_objects_disable_node_animation",
	"debug_objects_dump_memory_stats",
	"debug_objects_object",
	"debug_objects_by_index",
	"debug_objects_vehicle_suspension",
	"debug_objects_cluster_counts",
	"debug_objects_cluster_count_threshold",
	"render_model_nodes",
	"render_model_point_counts",
	"render_model_vertex_counts",
	"render_model_names",
	"render_model_triangle_counts",
	"render_model_collision_vertex_counts",
	"render_model_collision_surface_counts",
	"render_model_texture_usage",
	"render_model_geometry_usage",
	"render_model_cost",
	"render_model_markers",
	"render_model_no_geometry",
	"render_model_skinning_disable",
	"debug_player_damage",
	"debug_damage_radius",
	"breakpoints_enabled",
	"debug_trigger_volume_triangulation",
	"debug_point_physics",
	"debug_obstacle_path",
	"debug_obstacle_path_on_failure",
	"debug_obstacle_path_start_point_x",
	"debug_obstacle_path_start_point_y",
	"debug_obstacle_path_goal_point_x",
	"debug_obstacle_path_goal_point_y",
	"suppress_pathfinding_generation",
	"enable_pathfinding_generation_xbox",
	"ai_generate_flood_sector_wrl",
	"ai_pathfinding_generate_stats",
	"debug_player_control_autoaim_always_active",
	"debug_structure_soft_ceilings_biped",
	"debug_structure_soft_ceilings_vehicle",
	"debug_structure_soft_ceilings_huge_vehicle",
	"debug_structure_soft_ceilings_camera",
	"debug_structure_soft_ceilings_test_observer",
	"debug_structure_seams",
	"debug_structure_seam_triangles",
	"debug_bsp",
	"debug_plane_index",
	"debug_surface_index",
	"debug_leaf0_index",
	"debug_leaf1_index",
	"debug_leaf_connection_index",
	"debug_cluster_index",
	"debug_first_person_weapons",
	"debug_first_person_models",
	"debug_lights",
	"debug_light_passes",
	"debug_biped_landing",
	"debug_biped_relaxation_pose",
	"debug_biped_node_velocities",
	"debug_collision_skip_instanced_geometry",
	"debug_collision_skip_objects",
	"debug_collision_skip_vectors",
	"debug_collision_object_payload_collision",
	"debug_material_effects",
	"debug_material_default_effects",
	"player_training_debug",
	"player_training_disable",
	"game_engine_debug_statborg",
	"jaime_control_hack",
	"bertone_control_hack",
	"motor_system_debug",
	"ai_render_all_actors",
	"ai_render_inactive_actors",
	"ai_render_lineoffire_crouching",
	"ai_render_lineoffire",
	"ai_render_lineofsight",
	"ai_render_ballistic_lineoffire",
	"ai_render_vision_cones",
	"ai_render_current_state",
	"ai_render_detailed_state",
	"ai_render_props",
	"ai_render_props_web",
	"ai_render_props_no_friends",
	"ai_render_props_unreachable",
	"ai_render_props_unopposable",
	"ai_render_props_stimulus",
	"ai_render_props_dialogue",
	"ai_render_props_salience",
	"ai_render_props_update",
	"ai_render_idle_look",
	"ai_render_support_surfaces",
	"ai_render_recent_damage",
	"ai_render_current_damage",
	"ai_render_threats",
	"ai_render_emotions",
	"ai_render_audibility",
	"ai_render_aiming_vectors",
	"ai_render_secondary_looking",
	"ai_render_targets",
	"ai_render_targets_last_visible",
	"ai_render_states",
	"ai_render_vitality",
	"ai_render_evaluations",
	"ai_render_evaluations_detailed",
	"ai_render_evaluations_text",
	"ai_render_evaluations_shading",
	"ai_render_evaluations_shading_type",
	"ai_render_pursuit",
	"ai_render_shooting",
	"ai_render_trigger",
	"ai_render_projectile_aiming",
	"ai_render_aiming_validity",
	"ai_render_speech",
	"ai_render_leadership",
	"ai_render_status_flags",
	"ai_render_goal_state",
	"ai_render_behavior_debug",
	"ai_render_active_camo",
	"ai_render_vehicle_attachment",
	"ai_render_actor_blinddeaf",
	"ai_render_morphing",
	"ai_render_look_orders",
	"ai_render_behavior_failure",
	"ai_render_dialogue",
	"ai_render_dialogue_queue",
	"ai_render_dialogue_records",
	"ai_dialogue_test_mode",
	"ai_dialogue_datamine_enable",
	"ai_render_teams",
	"ai_render_player_ratings",
	"ai_render_spatial_effects",
	"ai_render_firing_positions",
	"ai_render_firing_position_statistics",
	"ai_render_firing_position_obstacles",
	"ai_render_mission_critical",
	"ai_render_gun_positions",
	"ai_render_aiming_positions",
	"ai_render_burst_geometry",
	"ai_render_vehicle_avoidance",
	"ai_render_vehicles_enterable",
	"ai_render_melee_check",
	"ai_render_grenades",
	"ai_render_danger_zones",
	"ai_render_control",
	"ai_render_activation",
	"ai_render_paths",
	"ai_render_paths_text",
	"ai_render_paths_selected_only",
	"ai_render_paths_destination",
	"ai_render_paths_raw",
	"ai_render_paths_current",
	"ai_render_paths_failed",
	"ai_render_paths_smoothed",
	"ai_render_paths_avoided",
	"ai_render_paths_error_thresholds",
	"ai_render_paths_avoidance_segment",
	"ai_render_paths_avoidance_obstacles",
	"ai_render_paths_avoidance_search",
	"ai_render_paths_nodes",
	"ai_render_paths_nodes_all",
	"ai_render_paths_nodes_polygons",
	"ai_render_paths_nodes_costs",
	"ai_render_paths_nodes_closest",
	"ai_render_paths_distance",
	"ai_render_player_aiming_blocked",
	"ai_render_squad_patrol_state",
	"ai_render_deceleration_obstacles",
	"ai_render_recent_obstacles",
	"ai_render_combat_range",
	"ai_render_clumps",
	"ai_render_clump_props",
	"ai_render_clump_props_all",
	"ai_render_clump_dialogue",
	"ai_render_sector_bsps",
	"ai_render_giant_sector_bsps",
	"ai_render_sector_link_errors",
	"ai_render_non_walkable_sectors",
	"ai_render_sector_geometry_errors",
	"ai_pathfinding_generation_verbose",
	"ai_render_sectors_range_max",
	"ai_render_sectors_range_min",
	"ai_render_link_specific",
	"ai_render_links",
	"ai_render_user_hints",
	"ai_render_area_flight_hints",
	"ai_render_hints",
	"ai_render_hints_detailed",
	"ai_render_object_hints",
	"ai_render_object_hints_all",
	"ai_render_hints_movement",
	"ai_orders_print_entries",
	"ai_orders_print_entries_verbose",
	"ai_render_orders",
	"ai_render_suppress_combat",
	"ai_render_squad_patrol",
	"ai_render_formations",
	"ai_render_squad_fronts",
	"ai_render_squad_fronts_detailed",
	"ai_render_ai_iterator",
	"ai_render_child_stack",
	"ai_render_behavior_stack",
	"ai_render_combat_status",
	"ai_render_decisions",
	"ai_render_decisions_all",
	"ai_render_script_data",
	"ai_hide_actor_errors",
	"ai_render_tracked_props",
	"ai_debug_vignettes",
	"ai_render_joint_behaviors",
	"ai_render_swarm",
	"ai_render_flocks",
	"ai_debug_prop_refresh",
	"ai_debug_all_disposable",
	"ai_render_vehicle_turns",
	"ai_render_discarded_firing_positions",
	"ai_render_firing_positions_all",
	"ai_render_firing_position_info",
	"ai_inspect_avoidance_failure",
	"ai_render_action_selection_failure",
	"ai_print_major_upgrade",
	"ai_print_evaluation_statistics",
	"ai_print_communication",
	"ai_print_communication_player",
	"ai_print_vocalizations",
	"ai_print_placement",
	"ai_print_speech",
	"ai_print_allegiance",
	"ai_print_lost_speech",
	"ai_print_migration",
	"ai_print_scripting",
	"ai_print_disposal",
	"ai_print_killing_sprees",
	"ai_naimad_spew",
	"ai_maxd_spew",
	"ai_debug_fast_los",
	"ai_debug_evaluate_all_positions",
	"ai_debug_path",
	"ai_debug_path_start_freeze",
	"ai_debug_path_end_freeze",
	"ai_debug_path_flood",
	"ai_debug_path_maximum_radius",
	"ai_debug_path_attractor",
	"ai_debug_path_attractor_radius",
	"ai_debug_path_attractor_weight",
	"ai_debug_path_accept_radius",
	"ai_debug_path_radius",
	"ai_debug_path_destructible",
	"ai_debug_path_giant",
	"ai_debug_ballistic_lineoffire_freeze",
	"ai_debug_path_naive_estimate",
	"ai_debug_blind",
	"ai_debug_deaf",
	"ai_debug_invisible_player",
	"ai_debug_ignore_player",
	"ai_debug_force_all_active",
	"ai_debug_path_disable_smoothing",
	"ai_debug_path_disable_obstacle_avoidance",
	"net_bitstream_debug",
	"net_bitstream_display_errors",
	"net_bitstream_capture_structure",
	"net_never_timeout",
	"net_use_local_time",
	"net_traffic_warnings",
	"net_traffic_print",
	"net_messages_print",
	"net_replication_requests_print",
	"net_packet_print_mask",
	"net_rate_unlimited",
	"net_rate_server",
	"net_rate_client",
	"net_window_unlimited",
	"net_window_size",
	"net_bandwidth_unlimited",
	"net_bandwidth_per_channel",
	"net_streams_disable",
	"net_disable_flooding",
	"net_ignore_version",
	"net_ignore_join_checking",
	"net_ignore_migration_checking",
	"net_maximum_machine_count",
	"net_maximum_player_count",
	"net_debug_random_seeds",
	"net_allow_out_of_sync",
	"net_distributed_always",
	"net_distributed_never",
	"net_matchmaking_force_gather",
	"net_matchmaking_force_search",
	"net_matchmaking_fail_arbitration",
	"net_connectivity_model_enabled",
	"net_nat_override",
	"net_matchmaking_nat_check_enabled",
	"net_matchmaking_hopper_id_adjustment",
	"net_matchmaking_use_last_map_and_game",
	"net_matchmaking_allow_early_start",
	"net_matchmaking_skip_host_migration",
	"net_matchmaking_force_disband",
	"net_enable_host_migration_loop",
	"net_matchmaking_fake_progress",
	"net_matchmaking_force_no_joining",
	"net_matchmaking_allow_idle_controllers",
	"net_simulation_set_stream_bandwidth",
	"net_set_channel_disconnect_interval",
	"net_enable_block_detection",
	"net_override_base_xp",
	"net_override_ranked_games_played",
	"net_matchmaking_mask_maps",
	"net_status_memory",
	"net_status_link",
	"net_status_sim",
	"net_status_channels",
	"net_status_connections",
	"net_status_message_queues",
	"net_status_observer",
	"net_status_sessions",
	"net_status_leaderboard",
	"net_status_leaderboard_mask",
	"net_test",
	"net_test_rate",
	"net_test_update_server",
	"net_test_update_client",
	"net_test_replication_scheduler",
	"net_test_debug_spheres",
	"net_test_matchmaking_playlist_index",
	"net_voice_diagnostics",
	"net_http_failure_ratio",
	"sim_status_world",
	"sim_status_views",
	"sim_entity_validate",
	"sim_disable_aim_assist",
	"sim_bandwidth_eater",
	"debug_player_teleport",
	"debug_players",
	"debug_player_input",
	"debug_survival_mode",
	"display_rumble_status_lines",
	"texture_cache_show_mipmap_bias",
	"texture_cache_graph",
	"texture_cache_list",
	"texture_cache_force_low_detail",
	"texture_cache_force_medium_detail",
	"texture_cache_force_high_detail",
	"texture_cache_status",
	"texture_cache_usage",
	"texture_cache_block_warning",
	"texture_cache_lod_bias",
	"texture_cache_dynamic_low_detail_texture",
	"render_debug_low_res_textures",
	"geometry_cache_graph",
	"geometry_cache_list",
	"geometry_cache_status",
	"geometry_cache_block_warning",
	"geometry_cache_never_block",
	"geometry_cache_debug_display",
	"director_camera_switch_fast",
	"director_camera_switch_disable",
	"director_disable_first_person",
	"director_use_dt",
	"observer_collision_enabled",
	"observer_collision_anticipation_enabled",
	"observer_collision_water_flags",
	"g_observer_wave_height",
	"debug_recording",
	"debug_recording_newlines",
	"cinematic_letterbox_style",
	"vehicle_status_display",
	"vehicle_disable_suspension_animations",
	"vehicle_disable_acceleration_screens",
	"biped_meter_display",
	"display_verbose_disk_usage",
	"display_disk_usage",
	"default_scenario_ai_type",
	"debug_menu_enabled",
	"catch_exceptions",
	"debug_first_person_hide_base",
	"debug_first_person_hide_movement",
	"debug_first_person_hide_jitter",
	"debug_first_person_hide_overlay",
	"debug_first_person_hide_pitch_turn",
	"debug_first_person_hide_ammo",
	"debug_first_person_hide_ik",
	"global_playtest_mode",
	"g_override_logon_task",
	"g_logon_task_override_result_code",
	"ui_display_memory",
	"ui_memory_verify",
	"xov_display_memory",
	"render_comment_flags",
	"render_comment_flags_text",
	"render_comment_flags_look_at",
	"enable_controller_flag_drop",
	"override_player_representation_index",
	"debug_tag_dependencies",
	"disable_network_hopper_download",
	"disable_network_configuration_download",
	"check_system_heap",
	"data_mine_player_update_interval",
	"data_mine_mp_player_update_interval",
	"data_mine_debug_menu_interval",
	"data_mine_spam_enabled",
	"webstats_file_zip_writes_per_frame",
	"debug_projectiles",
	"debug_damage_effects",
	"debug_damage_effect_obstacles",
	"force_player_walking",
	"unit_animation_report_missing_animations",
	"font_cache_status",
	"font_cache_debug_texture",
	"font_cache_debug_graph",
	"font_cache_debug_list",
	"halt_on_stack_overflow",
	"disable_progress_screen",
	"character_force_physics",
	"enable_new_ik_method",
	"animation_throttle_dampening_scale",
	"animation_blend_change_scale",
	"biped_fitting_enable",
	"biped_fitting_root_offset_enable",
	"biped_pivot_enable",
	"biped_pivot_allow_player",
	"giant_hunt_player",
	"giant_hunting_min_radius",
	"giant_hunting_max_radius",
	"giant_hunting_throttle_scale",
	"giant_weapon_wait_time",
	"giant_weapon_trigger_time",
	"giant_ik",
	"giant_foot_ik",
	"giant_ankle_ik",
	"giant_elevation_control",
	"giant_force_buckle",
	"giant_force_crouch",
	"giant_force_death",
	"giant_buckle_rotation",
	"debug_objects_giant_feet",
	"debug_objects_giant_buckle",
	"enable_xsync_timings",
	"allow_restricted_tag_groups_to_load",
	"xsync_restricted_tag_groups",
	"enable_cache_build_resources",
	"xma_compression_level_default",
	"enable_console_window",
	"display_colors_in_banded_gamma",
	"use_tool_command_legacy",
	"maximum_tool_command_history",
	"disable_unit_aim_screens",
	"disable_unit_look_screens",
	"disable_unit_eye_tracking",
	"enable_tag_resource_xsync",
	"dont_recompile_shaders",
	"scenario_load_all_tags",
	"synchronization_debug",
	"profiler_pulse_rates",
	"profiler_collection_interval",
	"debug_objects_scenery",
	"disable_switch_zone_sets",
	"facial_animation_testing_enabled",
	"profiler_datamine_uploads_enabled",
	"debug_object_recycling",
	"enable_sound_over_network",
	"lsp_allow_lsp_connections",
	"lsp_allow_raw_connections",
	"lsp_service_id_override",
	"shared_files_enable",
	"sound_manager_debug_suppression",
	"serialize_update_and_render",
	"minidump_use_retail_provider",
	"scenario_use_non_bsp_zones",
	"allow_restricted_active_zone_reloads",
	"debug_cinematic_controls_enable",
	"debug_campaign_metagame",
	"debug_campaign_metagame_verbose",
	"aiming_interpolation_stop_delta",
	"aiming_interpolation_start_delta",
	"aiming_interpolation_rate",
	"airborne_arc_enabled",
	"airborne_descent_test_duration",
	"airborne_descent_test_count",
	"disable_main_loop_throttle",
	"force_unit_walking",
	"leap_force_start_rotation",
	"leap_force_end_rotation",
	"leap_force_flight_start_rotation",
	"leap_force_flight_end_rotation",
	"leap_flight_path_scale",
	"leap_departure_power",
	"leap_departure_scale",
	"leap_arrival_power",
	"leap_arrival_scale",
	"leap_twist_power",
	"leap_cannonball_power",
	"leap_cannonball_scale",
	"leap_idle_power",
	"leap_idle_scale",
	"leap_overlay_power",
	"leap_overlay_scale",
	"leap_interpolation_limit",
	"biped_fake_soft_landing",
	"biped_fake_hard_landing",
	"biped_soft_landing_recovery_scale",
	"biped_hard_landing_recovery_scale",
	"biped_landing_absorbtion",
	"debug_player_network_aiming",
	"aim_assist_disable_target_lead_vector",
	"enable_tag_edit_sync",
	"render_debug_dont_flash_low_res_textures",
	"ui_alpha",
	"ui_alpha_lockdown",
	"ui_alpha_eula_accepted",
	"ui_alpha_custom_games_enabled",
	"net_config_client_badness_rating_threshold_override",
	"net_config_disable_bad_client_anticheating_override",
	"net_config_disable_bad_connectivity_anticheating_override",
	"net_config_disable_bad_bandwidth_anticheating_override",
	"net_config_maximum_multiplayer_split_screen_override",
	"net_config_crash_handling_minidump_type_override",
	"net_config_crash_handling_ui_display_override",
	"online_files_slowdown",
	"debug_trace_main_events",
	"force_xsync_memory_buyback",
	"bitmaps_trim_unused_pixels",
	"bitmaps_interleave_compressed_bitmaps",
	"enable_structure_audibility",
	"debug_sound_transmission",
	"minidump_force_regular_minidump_with_ui",
	"giant_custom_anim_recovery_time",
	"facial_animation_enable_lipsync",
	"facial_animation_enable_gestures",
	"facial_animation_enable_noise",
	"rasterizer_disable_vsync",
	"scale_ui_to_maintain_aspect_ratio",
	"maximum_aspect_ratio_scale_percentage",
	"disable_audibility_generation",
	"unicode_warn_on_truncation",
	"debug_determinism_version",
	"debug_determinism_compatible_version",
	"error_geometry_environment_enabled",
	"error_geometry_lightmap_enabled",
	"error_geometry_seam_enabled",
	"error_geometry_instance_enabled",
	"error_geometry_object_enabled",
	"debug_objects_unit_melee",
	"utility_drive_enabled",
	"debug_objects_root_node_print",
	"require_secure_cache_files",
	"debug_aim_assist_targets_enabled",
	"render_debug_cortana_ticks",
	"unknown50A",
	"unknown50B",
	"unknown50C",
	"unknown50D",
	"unknown50E",
	"unknown50F",
	"unknown510",
	"unknown511",
	"unknown512",
	"unknown513",
	"unknown514",
	"unknown515",
	"unknown516",
	"unknown517",
	"unknown518",
	"unknown519",
	"unknown51B",
	"unknown51C",
	"unknown51D",
	"load_time_multiplier",
	"g_enable_debug_animation_solving",
	"display_prefetch_progress",
	"survival_mode_allow_flying_camera",
};

static bool is_undefined_hs_global_name(const char* name)
{
	// return true if name in k_undefined_hs_global_names
	for (int32 i = 0; i < k_hs_undefined_global_count; i++)
	{
		if (csstricmp(name, k_undefined_hs_global_names[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

char* __cdecl console_get_token()
{
	char* input_text = strrchr(console_globals.input_state.result, ' ') + 1;
	char* v1 = strrchr(console_globals.input_state.result, '(') + 1;
	char* result = strrchr(console_globals.input_state.result, '"') + 1;

	if (console_globals.input_state.result > input_text)
		input_text = console_globals.input_state.result;

	if (input_text > v1)
		v1 = input_text;

	if (v1 > result)
		return v1;

	return result;
}

void __cdecl console_clear()
{
	terminal_clear();
}

void __cdecl console_close()
{
	if (console_is_active())
	{
		terminal_gets_end(&console_globals.input_state);
		console_globals.open_timeout_seconds = 0.1f;
		console_globals.active = false;
	}
	else
	{
		debug_menu_close();
	}
}

void __cdecl console_complete()
{

	if (console_globals.input_state.result[0] == 0)
		return;

	char initial_input[256];
	strcpy_s(initial_input, console_globals.input_state.result);

	char current_input[256];

	// Handle line-start match '^'
	bool match_line_start = console_globals.input_state.result[0] == '^';
	if (match_line_start) { // get input after '^'
		strcpy_s(current_input, console_globals.input_state.result + 1);
	}
	else { // get entire current input
		strcpy_s(current_input, console_globals.input_state.result);
	}

	// A valid input for console completion has the following form:
	// '(?:\^)?(\w+)(?: -(\w+))*'
	// Where:
	//  (?:\^)?         : an optional '^' at the start of the line to indicate line-start matching
	//  (\w+)           : the token to complete
	//  (?: -(\w+))*   : zero or more filter phrases, each starting with ' -' followed by a word

	// At this point, we've determined whether the input starts with '^' and have extracted the current input accordingly.

	// We now need to parse out the token to complete and any filter phrases from current_input.

	size_t input_len = strlen(current_input);
	size_t token_start_idx = 0;
	size_t token_len = 0;

	// Skip leading spaces (robustness)
	while (token_start_idx < input_len && current_input[token_start_idx] == ' ')
		token_start_idx++;

	// Find token [\w+]
	size_t p = token_start_idx;
	while (p < input_len && (isalnum(current_input[p]) || current_input[p] == '_'))
	{
		p++;
	}
	token_len = (p > token_start_idx) ? (p - token_start_idx) : 0;

	// Store parsed token
	c_static_string<256> parsed_token;
	parsed_token.clear();
	if (token_len > 0)
		parsed_token.append_print("%.*s", int(token_len), current_input + token_start_idx);

	// Extract filters matching " -(\w+)"
	c_static_string<64> filters[16];
	int32 filter_count = 0;
	const char* scan = current_input;
	while (const char* hit = strstr(scan, " -"))
	{
		const char* fstart = hit + 2; // after space and dash
		const char* fend = fstart;

		while (*fend && (isalnum(*fend) || *fend == '_'))
		{
			fend++;
		}

		if (fend > fstart && filter_count < int32(NUMBEROF(filters)))
		{
			filters[filter_count].set("");
			filters[filter_count].append_print("%.*s", int(fend - fstart), fstart);
			filter_count++;
		}

		scan = fend;
	}

	// Clear existing input
	console_globals.input_state.result[0] = 0;
	// Set new text
	csnzappendf(console_globals.input_state.result, NUMBEROF(console_globals.input_state.result), current_input);
	// Reset the edit cursor position
	edit_text_selection_reset(&console_globals.input_state.edit);

	bool present_list = false;
	if (console_token_buffer.is_empty())
	{
		// Use the parsed token (not the last space-delimited token)
		console_token_buffer.set(parsed_token.get_string());
		suggestion_current_index = NONE;
		present_list = true;
	}

	// Make 'token' point to the actual token region in the input buffer, so we can replace it in-place
	char* token = console_globals.input_state.result + token_start_idx;

	const char* matching_items[256]{};
	int16 matching_item_count = hs_tokens_enumerate(console_token_buffer.get_string(), NONE, matching_items, NUMBEROF(matching_items));
	if (matching_item_count)
	{
		ASSERT(matching_items[0]);

		int16 last_similar_character_index = SHRT_MAX;
		bool columnize = matching_item_count > 12;

		c_static_string<1024> print_buffer;
		print_buffer.set("");

		if (present_list)
		{
			// Filter out undefined globals from matches
			bool filtered_out[256]{};
			int16 display_count = 0;
			for (int16 i = 0; i < matching_item_count; i++)
			{
				const char* item = matching_items[i];
				filtered_out[i] = false;
				if (!item || is_undefined_hs_global_name(item)) {
					filtered_out[i] = true;
				}
				if (match_line_start && !filtered_out[i])
				{
					// Filter the item out if it does not start with the current input in its entirety
					if (csstrnicmp(item, console_token_buffer.get_string(), console_token_buffer.length()) != 0) {
						filtered_out[i] = true;
					}
				}
				if (!filtered_out[i] && filter_count > 0)
				{
					// Check the current item against all filters (case-insensitive comparison)
					// If the item contains the filter, the item is filtered out
					for (int32 f = 0; f < filter_count; f++)
					{
						if (strstr(item, filters[f].get_string())) {
							filtered_out[i] = true;
							break;
						}
					}
				}
				if (!filtered_out[i]) {
					display_count++;
				}
			}

			const int16 columns = 3; // fixed column count
			const int16 width = 42;  // fixed width

			// Print the (filtered) match count and the string being matched, e.g.: "(23 matches) 'debug_':"
			console_printf("(%d matches) '%s':", display_count, initial_input);

			// Render (space-padded, no tabs), track printed columns only when an item is actually printed
			if (columnize)
			{
				int16 printed_in_row = 0;
				for (int16 i = 0; i < matching_item_count; i++)
				{

					const char* item = matching_items[i];
					if (filtered_out[i])
						continue;

					int16 item_len = int16(strlen(item));
					if (item_len > width - 1)
					{
						// truncate item to width using '~' to indicate truncation
						print_buffer.append_print("%.*s~ ", width - 1, item);
					}
					else
					{
						// pad item to width
						print_buffer.append_print("%-*s ", width, item);
					}

					printed_in_row++;

					if (printed_in_row % columns == 0)
					{
						console_printf("%s", print_buffer.get_string());
						print_buffer.clear();
					}
				}

				// Flush any remainder
				if (!print_buffer.is_empty())
				{
					console_printf("%s", print_buffer.get_string());
					print_buffer.clear();
				}
			}
			else
			{
				for (int16 i = 0; i < matching_item_count; i++)
				{
					const char* item = matching_items[i];
					if (filtered_out[i])
						continue;

					console_printf("%s", item);
				}
			}

			suggestion_current_index = 0;
			console_token_buffer.copy_to(token, 256);
			console_globals.input_state.edit.insertion_point_index = uns16(console_token_buffer.length() + 1);

		}
		else if (suggestion_current_index == matching_item_count)
		{
			suggestion_current_index = 0;
			console_token_buffer.copy_to(token, 256);
			console_globals.input_state.edit.insertion_point_index = uns16(console_token_buffer.length() + 1);
		}
		else
		{
			csmemcpy(token, matching_items[suggestion_current_index], strlen(matching_items[suggestion_current_index]));

			int16 suggestion_length = int16(strlen(matching_items[suggestion_current_index++]));
			token[suggestion_length] = 0;

			console_globals.input_state.edit.insertion_point_index = int16(token - console_globals.input_state.result) + suggestion_length;
		}
	}
}

void __cdecl console_dispose()
{
	static bool x_once = true;
	if (x_once)
	{
		console_close();
		debug_keys_dispose();
		terminal_dispose();
		x_once = false;
	}
}

void __cdecl console_execute_initial_commands()
{
	if (FILE* file = console_open_init())
	{
		char buffer[200]{};
		while (fgets(buffer, NUMBEROF(buffer), file))
		{
			string_terminate_at_first_delimiter(buffer, "\r\n");
			console_process_command(buffer, false);
		}
		fclose(file);
	}
}

void __cdecl console_initialize()
{
	static bool x_once = true;
	if (x_once)
	{
		terminal_initialize();

		console_globals.status_render = true;
		console_globals.input_state.color = { 1.0f, 1.0f, 0.3f, 1.0f };
		csstrnzcpy(console_globals.input_state.prompt, "donkey( ", NUMBEROF(console_globals.input_state.prompt));
		console_globals.input_state.result[0] = 0;
		console_globals.previous_command_count = 0;
		console_globals.newest_previous_command_index = NONE;
		console_globals.selected_previous_command_index = NONE;

		debug_keys_initialize();

		for (int32 index = 0; index < NUMBEROF(g_status_strings); index++)
		{
			s_status_line* line = &g_status_strings[index].line;
			status_lines_initialize(line, NULL, 1);
			line->flags.set(_status_line_left_justify_bit, true);
		}

		x_once = false;
	}
}

bool __cdecl console_is_active()
{
	return console_globals.active;
}

bool __cdecl console_is_empty()
{
	return console_globals.active && !console_globals.input_state.result[0];
}

void __cdecl console_open(bool debug_menu)
{
	if (!console_is_active() && !debug_menu_get_active())
	{
		if (debug_menu)
		{
			debug_menu_open();
		}
		else
		{
			console_globals.input_state.result[0] = 0;
			console_globals.active = terminal_gets_begin(&console_globals.input_state);
		}
	}
}

void __cdecl console_printf(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	if (is_main_thread())
	{
		char buffer[255];
		cvsnzprintf(buffer, sizeof(buffer), format, arglist);
		terminal_printf(NULL, buffer);
		if (console_dump_to_debug_display)
		{
			display_debug_string(buffer);
		}
	}
	va_end(arglist);
}

void __cdecl console_printf_color(const real_argb_color* color, const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	if (is_main_thread())
	{
		char buffer[255];
		cvsnzprintf(buffer, sizeof(buffer), format, arglist);
		terminal_printf(color, buffer);
		if (console_dump_to_debug_display)
		{
			display_debug_string(buffer);
		}
	}
	va_end(arglist);
}

void __cdecl console_update(real32 shell_seconds_elapsed)
{
	PROFILER(console_update)
	{
		if (console_is_active())
		{
			for (int32 key_index = 0; key_index < console_globals.input_state.key_count; key_index++)
			{
				s_key_state* key = &console_globals.input_state.keys[key_index];
				ASSERT(key->ascii_code != _key_not_a_key);

				if (key->key_type == _key_type_down && (key->ascii_code == _key_backquote || key->ascii_code == _key_f1))
				{
					console_close();
					break;
				}
				else if (key->key_type == _key_type_up && key->ascii_code == _key_tab)
				{
					console_complete();
				}
				else if (TEST_BIT(key->modifier_flags, _key_modifier_flag_control_key_bit) && key->key_type == _key_type_up && key->ascii_code == _key_v)
				{
					char buffer[256]{};
					get_clipboard_as_text(buffer, NUMBEROF(buffer));
					csnzappendf(console_globals.input_state.result, NUMBEROF(console_globals.input_state.result), buffer);

					suggestion_current_index = 0;
					console_token_buffer.clear();
					break;
				}
				else if (key->key_type == _key_type_up && (key->ascii_code == _key_return || key->ascii_code == _keypad_enter))
				{
					if (console_globals.input_state.result[0])
					{
						console_process_command(console_globals.input_state.result, true);
						console_globals.input_state.result[0] = 0;
						edit_text_selection_reset(&console_globals.input_state.edit);
					}
					break;
				}
				else if (key->key_type == _key_type_up && (key->ascii_code == _key_up_arrow || key->ascii_code == _key_down_arrow))
				{
					if (key->ascii_code == _key_up_arrow)
					{
						console_globals.selected_previous_command_index += 2;
					}

					int16 v4 = console_globals.selected_previous_command_index - 1;
					console_globals.selected_previous_command_index = v4;

					if (v4 <= 0)
					{
						console_globals.selected_previous_command_index = 0;
					}

					if (v4 <= 0)
					{
						v4 = 0;
					}

					if (v4 > console_globals.previous_command_count - 1)
					{
						v4 = console_globals.previous_command_count - 1;
						console_globals.selected_previous_command_index = console_globals.previous_command_count - 1;
					}

					if (v4 != NONE)
					{
						decltype(console_globals.input_state.result)& input_text = console_globals.input_state.result;
						decltype(console_globals.previous_commands)& previous_commands = console_globals.previous_commands;
						decltype(console_globals.newest_previous_command_index)& newest_previous_command_index = console_globals.newest_previous_command_index;

						previous_commands[(newest_previous_command_index - v4 + NUMBEROF(previous_commands)) % NUMBEROF(previous_commands)].copy_to(input_text, NUMBEROF(input_text));
						edit_text_selection_reset(&console_globals.input_state.edit);
					}
					break;
				}
				else if (key->vk_code != NONE && key->key_type == _key_type_char)
				{
					csnzappendf(console_globals.input_state.result, NUMBEROF(console_globals.input_state.result), key->character);
					break;
				}
				else
				{
					suggestion_current_index = 0;
					console_token_buffer.clear();
				}
			}
		}
		else if (!debugging_system_has_focus())
		{
			s_key_state key{};
			if (input_peek_key(&key, _input_type_game))
			{
				if (!key.repeating && !key.modifier_flags && key.key_type == _key_type_down && (key.ascii_code == _key_tilde || key.ascii_code == _key_f1))
				{
					input_get_key(&key, _input_type_game);
					console_open(false);
				}
			}

			debug_keys_update();
		}

		if ((console_globals.open_timeout_seconds - shell_seconds_elapsed) >= 0.0f)
		{
			console_globals.open_timeout_seconds -= shell_seconds_elapsed;
		}
		else
		{
			console_globals.open_timeout_seconds = 0.0f;
		}
	}
}

void __cdecl console_warning(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	if (is_main_thread())
	{
		char buffer[255]{};
		cvsnzprintf(buffer, sizeof(buffer), format, arglist);
		terminal_printf(global_real_argb_red, "%s", buffer);
	}
	va_end(arglist);
}

bool __cdecl console_process_command(const char* command, bool interactive)
{
	if (strlen(command) >= 255)
	{
		return false;
	}

	if (!command[0] || command[0] == ';')
	{
		return false;
	}

	main_status("console_command", "%s", command);

	int16 command_index = (console_globals.newest_previous_command_index + 1) % NUMBEROF(console_globals.previous_commands);
	console_globals.newest_previous_command_index = command_index;
	console_globals.previous_commands[command_index].set(command);
	if (++console_globals.previous_command_count > NUMBEROF(console_globals.previous_commands))
	{
		console_globals.previous_command_count = NUMBEROF(console_globals.previous_commands);
	}
	console_globals.selected_previous_command_index = NONE;

	bool result = hs_compile_and_evaluate(_event_message, "console_command", command, interactive);

#if 0
	if (!result)
	{
		tokens_t tokens{};
		int32 token_count = 0;
		command_tokenize(command, tokens, &token_count);

		if (token_count == 2 && load_preference(tokens[0]->get_string(), tokens[1]->get_string()))
		{
			return true;
		}

		if (token_count > 0)
		{
			const char* command_name = tokens[0]->get_string();

			bool command_found = false;
			for (int32 i = 0; i < NUMBEROF(k_registered_commands); i++)
			{
				if (tokens[0]->is_equal(k_registered_commands[i].name))
				{
					command_found = true;

					callback_result_t callback_result = k_registered_commands[i].callback(&k_registered_commands[i], token_count, tokens);

					c_console::write(callback_result.get_string());

					int32 succeeded = callback_result.index_of(": succeeded");
					result = succeeded != NONE || tokens[0]->is_equal("help");

					if (result)
						console_printf("command '%s' succeeded", command_name);
					else
						console_warning("command '%s' failed: %s", command_name, callback_result.get_string());

					return result;
				}
			}
		}
	}
#endif

	main_status("console_command", NULL);
	return result;
}

bool __cdecl debugging_system_has_focus()
{
	return console_is_active() || debug_menu_get_active();
}

void status_line_add_single(s_status_line* inStatusLine)
{
	ASSERT(NULL == inStatusLine->prev);
	ASSERT(NULL == inStatusLine->next);

	inStatusLine->next = NULL;
	inStatusLine->prev = g_status_line_tail;
	
	if (g_status_line_tail)
		g_status_line_tail->next = inStatusLine;
	else
		g_status_line_head = inStatusLine;

	if (inStatusLine->next)
		inStatusLine->next->prev = inStatusLine;
	else
		g_status_line_tail = inStatusLine;
}

void status_line_draw()
{
	PROFILER(status_line_draw)
	{
		if (!console_globals.status_render/* || !can_use_claws()*/)
			return;

		for (int32 i = 0; i < NUMBEROF(g_status_strings); i++)
		{
			s_status_line& status_line = g_status_strings[i].line;
			if (!status_line.text.is_empty())
			{
				uns32 time = system_milliseconds();
				int32 time_delta = time - g_status_strings[i].time_created;
				if (time_delta > 10000)
				{
					status_line.text.clear();
				}
				else
				{
					if (time_delta > 5000)
					{
						time = time_delta - 5000;
						status_line.alpha = 1.0f - (real32(time) / 5000);
					}
					else
					{
						status_line.alpha = 1.0f;
					}
				}
			}
		}

		c_rasterizer_draw_string draw_string{};
		c_font_cache_mt_safe font_cache{};
		s_string_cache string_cache{};

		string_cache.string.clear();

		for (s_status_line* status_line = g_status_line_head; status_line; status_line = status_line->next)
		{
			if (!status_line_visible(status_line))
				continue;

			e_text_justification justification = e_text_justification(!status_line->flags.test(_status_line_left_justify_bit));

			const char* string = status_line->text.get_string();
			if (status_line->flags.test(_status_line_blink_bit) && system_milliseconds() % 500 < 250)
				string = "|n";

			if (!string_cache_add(&string_cache, string, status_line->alpha, status_line->color, justification))
			{
				string_cache_render(&string_cache, &draw_string, &font_cache);
				string_cache_add(&string_cache, string, status_line->alpha, status_line->color, justification);

				if (status_line->flags.test(_status_line_draw_once_bit))
					status_line->text.clear();
			}
		}

		string_cache_render(&string_cache, &draw_string, &font_cache);
	}
}

void status_line_dump()
{
	for (s_status_line* status_line = g_status_line_head; status_line; status_line = status_line->next)
	{
		if (status_line_visible(status_line))
			event(_event_message, "status_lines: %s", status_line->text.get_string());
	}
}

void status_line_remove_single(s_status_line* status_line)
{
	ASSERT(status_line->next != NULL || status_line == g_status_line_tail);
	ASSERT(status_line->prev != NULL || status_line == g_status_line_head);

	if (status_line->prev)
	{
		status_line->prev->next = status_line->next;
	}
	else
	{
		g_status_line_head = status_line->next;
	}

	if (status_line->next)
	{
		status_line->next->prev = status_line->prev;
	}
	else
	{
		g_status_line_tail = status_line->prev;
	}

	status_line->prev = NULL;
	status_line->next = NULL;
}

bool status_line_visible(const s_status_line* line)
{
	return (!line->in_use || *line->in_use) && !line->text.is_empty() && !line->flags.test(_status_line_inhibit_drawing_bit);
}

void status_lines_clear_text(s_status_line* status_lines, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		status_lines[i].text.clear();
	}
}

void status_lines_disable(const char* identifier)
{
	for (s_status_line* status_line = g_status_line_head; status_line; status_line = status_line->next)
	{
		if (status_line->identifier && status_line->in_use && !*status_line->in_use)
		{
			if (csstristr(status_line->identifier, identifier))
			{
				*status_line->in_use = false;
			}
		}
	}
}

void status_lines_dispose(s_status_line* status_lines, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		status_line_remove_single(&status_lines[i]);
	}
}

void status_lines_enable(const char* identifier)
{
	for (s_status_line* status_line = g_status_line_head; status_line; status_line = status_line->next)
	{
		if (status_line->identifier && status_line->in_use && !*status_line->in_use)
		{
			if (csstristr(status_line->identifier, identifier))
			{
				*status_line->in_use = true;
			}
		}
	}
}

void status_lines_initialize(s_status_line* status_lines, bool* flag, int32 count)
{
	status_lines_initialize_simple(status_lines, flag, NULL, count);
}

void status_lines_initialize_simple(s_status_line* status_line, bool* flag, const char* identifier, int32 count)
{
	csmemset(status_line, 0, sizeof(s_status_line) * count);

	for (int32 i = 0; i < count; i++)
	{
		s_status_line& line = status_line[i];
		line.in_use = flag;
		line.identifier = identifier;
		line.color = *global_real_rgb_white;
		line.alpha = 1.0f;
		status_line_add_single(&line);
	}
}

void status_lines_set_flags(s_status_line* status_lines, e_status_line_flags flag, bool enable, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		status_lines[i].flags.set(flag, enable);
	}
}

void status_printf(const char* format, ...)
{
	if (is_main_thread())
	{
		va_list arglist;
		va_start(arglist, format);
		status_printf_va(format, arglist);
		va_end(arglist);
	}
}

void status_printf_va(const char* format, char* argument_list)
{
	char buffer[1024]{};
	cvsnzprintf(buffer, sizeof(buffer), format, argument_list);
	status_string_internal(format, buffer);
}

void status_string_internal(const char* status, const char* message)
{
	for (int32 i = 0; i < NUMBEROF(g_status_strings); i++)
	{
		s_status_string& status_string = g_status_strings[i];
		if (!status_string.line.text.is_empty() && status_string.format_string.is_equal(status))
		{
			status_string.time_created = system_milliseconds();
			status_string.line.text.set(message);
			return;
		}
	}

	for (int32 i = 0; i < NUMBEROF(g_status_strings); i++)
	{
		s_status_string& status_string = g_status_strings[i];
		if (status_string.line.text.is_empty())
		{
			status_string.time_created = system_milliseconds();
			status_string.line.text.set(message);
			status_string.format_string.set(status);
			break;
		}
	}
}

void status_strings(const char* status, const char* strings)
{
	if (is_main_thread())
	{
		char buffer[1024]{};
		char* data[3]{};
		int32 line = 0;

		csstrnzcpy(buffer, strings, sizeof(buffer));
		for (char* line_end = csstrtok(buffer, "\r\n", 1, data); line_end; line_end = csstrtok(NULL, "\r\n", 1, data))
		{
			c_static_string<256> string;
			string.print("%d%s", line++, status);
			status_string_internal(string.get_string(), line_end);
		}
	}
}

bool string_cache_add(s_string_cache* cache, const char* string, real32 alpha, const real_rgb_color& color, e_text_justification justification)
{
	bool result = false;
	if (cache->string.is_empty())
	{
		cache->color = color;
		cache->alpha = alpha;
		cache->justification = justification;
		result = true;
	}
	else if (cache->alpha == alpha
		&& cache->color.red == color.red
		&& cache->color.green == color.green
		&& cache->color.blue == color.blue
		&& cache->justification == justification)
	{
		result = true;
	}

	if (result)
	{
		cache->string.append(string);
		cache->string.append("|n");
	}

	return result;
}

void string_cache_render(s_string_cache* cache, c_draw_string* draw_string, c_font_cache_base* font_cache)
{
	if (cache->string.is_empty())
		return;

	real_argb_color color{};
	real_argb_color shadow_color{};

	color.rgb = cache->color;
	color.alpha = cache->alpha * 0.5f;

	shadow_color.rgb = *global_real_rgb_black;
	shadow_color.alpha = cache->alpha;

	draw_string->set_justification(cache->justification);
	draw_string->set_color(&color);
	draw_string->set_shadow_color(&shadow_color);
	draw_string->draw(font_cache, cache->string.get_string());

	cache->string.clear();
}

