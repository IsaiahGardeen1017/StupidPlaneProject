#ifndef STUPID_PLANE_SIM_H
#define STUPID_PLANE_SIM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIM_MAX_AIRCRAFT 64
#define SIM_MAX_PROJECTILES 4096
#define SIM_MAX_PLANE_STATS 32
#define SIM_WORLD_CONFIG_SCHEMA_VERSION 1u
#define SIM_PLANE_STATS_SCHEMA_VERSION 1u

typedef struct {
    float x;
    float y;
    float z;
} sim_vec3;

typedef struct {
    float x;
    float y;
    float z;
} sim_euler;

typedef struct {
    float throttle; /* [0,1] */
    float pitch;    /* [0,1] where 0.5 is neutral */
    float roll;     /* [0,1] where 0.5 is neutral */
    uint8_t trigger;
} sim_control;

typedef struct {
    sim_vec3 position;
    sim_vec3 velocity;
    sim_euler orientation;
    sim_vec3 angular_rates;
    float throttle_state;
    float health;
    int ammo;
    uint8_t alive;
    uint8_t team;
    uint8_t plane_stats_index;
} sim_aircraft;

typedef struct {
    uint8_t active;
    uint8_t owner_index;
    sim_vec3 position;
    sim_vec3 velocity;
    float ttl_seconds;
    float damage;
} sim_projectile;

typedef struct {
    float x;
    float y;
} sim_curve_point;

typedef struct {
    const sim_curve_point *points;
    size_t count;
} sim_curve;

typedef struct {
    uint32_t schema_version;
    float dt;
    float world_floor_altitude;
    uint8_t friendly_fire;
    float trigger_cooldown_seconds;
    float projectile_speed;
    float projectile_ttl_seconds;
    float projectile_damage;
    float hit_radius;
    float gravity;
} sim_world_config;

typedef struct {
    uint32_t schema_version;
    float mass_kg;
    float wing_surface_area;
    float parasite_drag_reference_area;
    float parasite_drag_coefficient;
    float aspect_ratio_oswald_constant;
    sim_curve thrust_by_air_density;
    sim_curve lift_by_aoa;
    sim_curve roll_rate_by_input;
    sim_curve pitch_rate_by_input;
} sim_plane_stats;

typedef struct {
    uint32_t seed;
    sim_world_config world_config;
    sim_plane_stats plane_stats[SIM_MAX_PLANE_STATS];
    size_t plane_stats_count;
    sim_aircraft aircraft[SIM_MAX_AIRCRAFT];
    sim_control controls[SIM_MAX_AIRCRAFT];
    float trigger_cooldown[SIM_MAX_AIRCRAFT];
    size_t aircraft_count;

    sim_projectile projectiles[SIM_MAX_PROJECTILES];
    size_t projectile_count;

    uint64_t tick;
} sim_world;

typedef struct {
    float speed;
    float altitude;
    float climb_rate;
    float throttle;
    float pitch_attitude;
    float roll_attitude;
    float turn_rate;
    float aoa_estimate;
    float health;
    int ammo;
} sim_aircraft_telemetry;

typedef struct {
    uint64_t tick;
    uint32_t seed;
    size_t aircraft_count;
    size_t alive_aircraft_count;
    size_t projectile_count;
    sim_aircraft_telemetry aircraft[SIM_MAX_AIRCRAFT];
} sim_world_snapshot;

typedef struct {
    uint64_t tick;
    size_t aircraft_count;
    size_t alive_aircraft_count;
    size_t projectile_count;
    float total_health;
    int total_ammo;
} sim_world_metrics;

void sim_default_world_config(sim_world_config *out_config);
int sim_validate_world_config(const sim_world_config *config);
int sim_world_config_serialize(const sim_world_config *config, char *out_buffer, size_t out_buffer_size);
int sim_world_config_deserialize(sim_world_config *out_config, const char *serialized);
void sim_default_plane_stats(sim_plane_stats *out_stats);
int sim_validate_plane_stats(const sim_plane_stats *stats);
sim_world *sim_world_create(
    const sim_world_config *world_config,
    const sim_plane_stats *plane_stats,
    size_t plane_stats_count,
    uint32_t seed);
void sim_world_destroy(sim_world *world);
void sim_world_reset(
    sim_world *world,
    const sim_world_config *world_config,
    const sim_plane_stats *plane_stats,
    size_t plane_stats_count,
    uint32_t seed);
void sim_world_init(
    sim_world *world,
    const sim_world_config *world_config,
    const sim_plane_stats *plane_stats,
    size_t plane_stats_count,
    uint32_t seed);
int sim_spawn_aircraft(sim_world *world, const sim_aircraft *aircraft);
void sim_set_control(sim_world *world, size_t aircraft_index, sim_control control);
int sim_get_aircraft_telemetry(const sim_world *world, size_t aircraft_index, sim_aircraft_telemetry *out_telemetry);
int sim_get_world_snapshot(const sim_world *world, sim_world_snapshot *out_snapshot);
int sim_get_world_metrics(const sim_world *world, sim_world_metrics *out_metrics);
void sim_tick(sim_world *world);

/* Utility helpers for deterministic validation. */
uint64_t sim_world_hash(const sim_world *world);
float sim_curve_sample(const sim_curve *curve, float x);

#ifdef __cplusplus
}
#endif

#endif
