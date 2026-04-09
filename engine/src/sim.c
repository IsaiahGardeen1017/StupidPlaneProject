#include "sim.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t sim_rng_next(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static float vec3_len(sim_vec3 v);

void sim_default_config(sim_config *out_config) {
    if (out_config == NULL) {
        return;
    }

    memset(out_config, 0, sizeof(*out_config));
    out_config->schema_version = SIM_CONFIG_SCHEMA_VERSION;
    out_config->dt = 1.0f / 60.0f;
    out_config->world_floor_altitude = 0.0f;
    out_config->friendly_fire = 0;
    out_config->trigger_cooldown_seconds = 0.2f;
    out_config->projectile_speed = 320.0f;
    out_config->projectile_ttl_seconds = 3.0f;
    out_config->projectile_damage = 35.0f;
    out_config->hit_radius = 8.0f;
    out_config->gravity = 9.81f;
}

int sim_validate_config(const sim_config *config) {
    if (config == NULL) {
        return 0;
    }

    if (config->schema_version != SIM_CONFIG_SCHEMA_VERSION) {
        return 0;
    }
    if (config->dt <= 0.0f || config->dt > 0.1f) {
        return 0;
    }
    if (config->trigger_cooldown_seconds < 0.0f) {
        return 0;
    }
    if (config->projectile_speed <= 0.0f || config->projectile_ttl_seconds <= 0.0f) {
        return 0;
    }
    if (config->projectile_damage <= 0.0f || config->hit_radius <= 0.0f) {
        return 0;
    }
    if (config->gravity < 0.0f) {
        return 0;
    }

    return 1;
}

int sim_config_serialize(const sim_config *config, char *out_buffer, size_t out_buffer_size) {
    if (!sim_validate_config(config) || out_buffer == NULL || out_buffer_size == 0) {
        return -1;
    }

    int written = snprintf(
        out_buffer,
        out_buffer_size,
        "schema_version=%u\n"
        "dt=%.9g\n"
        "world_floor_altitude=%.9g\n"
        "friendly_fire=%u\n"
        "trigger_cooldown_seconds=%.9g\n"
        "projectile_speed=%.9g\n"
        "projectile_ttl_seconds=%.9g\n"
        "projectile_damage=%.9g\n"
        "hit_radius=%.9g\n"
        "gravity=%.9g\n",
        config->schema_version,
        config->dt,
        config->world_floor_altitude,
        (unsigned int)config->friendly_fire,
        config->trigger_cooldown_seconds,
        config->projectile_speed,
        config->projectile_ttl_seconds,
        config->projectile_damage,
        config->hit_radius,
        config->gravity);

    if (written < 0 || (size_t)written >= out_buffer_size) {
        return -1;
    }
    return written;
}

int sim_config_deserialize(sim_config *out_config, const char *serialized) {
    if (out_config == NULL || serialized == NULL) {
        return -1;
    }

    sim_config cfg;
    sim_default_config(&cfg);

    unsigned int schema_version = 0;
    unsigned int friendly_fire = 0;
    int parsed = sscanf(
        serialized,
        "schema_version=%u\n"
        "dt=%f\n"
        "world_floor_altitude=%f\n"
        "friendly_fire=%u\n"
        "trigger_cooldown_seconds=%f\n"
        "projectile_speed=%f\n"
        "projectile_ttl_seconds=%f\n"
        "projectile_damage=%f\n"
        "hit_radius=%f\n"
        "gravity=%f\n",
        &schema_version,
        &cfg.dt,
        &cfg.world_floor_altitude,
        &friendly_fire,
        &cfg.trigger_cooldown_seconds,
        &cfg.projectile_speed,
        &cfg.projectile_ttl_seconds,
        &cfg.projectile_damage,
        &cfg.hit_radius,
        &cfg.gravity);

    if (parsed != 10) {
        return -1;
    }
    cfg.schema_version = schema_version;
    cfg.friendly_fire = (uint8_t)(friendly_fire ? 1u : 0u);

    if (!sim_validate_config(&cfg)) {
        return -1;
    }

    *out_config = cfg;
    return 0;
}

float sim_curve_sample(const sim_curve *curve, float x) {
    if (curve == NULL || curve->points == NULL || curve->count == 0) {
        return 0.0f;
    }
    if (curve->count == 1 || x <= curve->points[0].x) {
        return curve->points[0].y;
    }

    for (size_t i = 1; i < curve->count; i++) {
        const sim_curve_point a = curve->points[i - 1];
        const sim_curve_point b = curve->points[i];
        if (x <= b.x) {
            const float width = b.x - a.x;
            if (width <= 1e-6f) {
                return b.y;
            }
            const float t = (x - a.x) / width;
            return a.y + (b.y - a.y) * t;
        }
    }

    return curve->points[curve->count - 1].y;
}

void sim_world_init(sim_world *world, const sim_config *config, uint32_t seed) {
    memset(world, 0, sizeof(*world));
    world->seed = seed;
    sim_default_config(&world->config);
    if (config != NULL && sim_validate_config(config)) {
        world->config = *config;
    }
}

sim_world *sim_world_create(const sim_config *config, uint32_t seed) {
    sim_world *world = (sim_world *)malloc(sizeof(*world));
    if (world == NULL) {
        return NULL;
    }
    sim_world_init(world, config, seed);
    return world;
}

void sim_world_destroy(sim_world *world) {
    free(world);
}

void sim_world_reset(sim_world *world, const sim_config *config, uint32_t seed) {
    if (world == NULL) {
        return;
    }
    sim_world_init(world, config, seed);
}

int sim_spawn_aircraft(sim_world *world, const sim_aircraft *aircraft) {
    if (world->aircraft_count >= SIM_MAX_AIRCRAFT || aircraft == NULL) {
        return -1;
    }
    size_t idx = world->aircraft_count++;
    world->aircraft[idx] = *aircraft;
    world->controls[idx] = (sim_control){.throttle = 0.0f, .pitch = 0.5f, .roll = 0.5f, .trigger = 0};
    world->trigger_cooldown[idx] = 0.0f;
    return (int)idx;
}

void sim_set_control(sim_world *world, size_t aircraft_index, sim_control control) {
    if (world == NULL || aircraft_index >= world->aircraft_count) {
        return;
    }
    world->controls[aircraft_index] = (sim_control){
        .throttle = clampf(control.throttle, 0.0f, 1.0f),
        .pitch = clampf(control.pitch, 0.0f, 1.0f),
        .roll = clampf(control.roll, 0.0f, 1.0f),
        .trigger = control.trigger ? 1 : 0,
    };
}

int sim_get_aircraft_telemetry(const sim_world *world, size_t aircraft_index, sim_aircraft_telemetry *out_telemetry) {
    if (world == NULL || out_telemetry == NULL || aircraft_index >= world->aircraft_count) {
        return -1;
    }

    const sim_aircraft *a = &world->aircraft[aircraft_index];
    out_telemetry->speed = vec3_len(a->velocity);
    out_telemetry->altitude = a->position.y - world->config.world_floor_altitude;
    out_telemetry->climb_rate = a->velocity.y;
    out_telemetry->throttle = a->throttle_state;
    out_telemetry->pitch_attitude = a->orientation.x;
    out_telemetry->roll_attitude = a->orientation.z;
    out_telemetry->turn_rate = a->angular_rates.z;
    out_telemetry->aoa_estimate = clampf(a->orientation.x, -1.2f, 1.2f);
    out_telemetry->health = a->health;
    out_telemetry->ammo = a->ammo;

    return 0;
}

int sim_get_world_snapshot(const sim_world *world, sim_world_snapshot *out_snapshot) {
    if (world == NULL || out_snapshot == NULL) {
        return -1;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));
    out_snapshot->tick = world->tick;
    out_snapshot->seed = world->seed;
    out_snapshot->aircraft_count = world->aircraft_count;
    out_snapshot->projectile_count = world->projectile_count;

    for (size_t i = 0; i < world->aircraft_count; i++) {
        if (sim_get_aircraft_telemetry(world, i, &out_snapshot->aircraft[i]) != 0) {
            return -1;
        }
        if (world->aircraft[i].alive) {
            out_snapshot->alive_aircraft_count += 1;
        }
    }

    return 0;
}

int sim_get_world_metrics(const sim_world *world, sim_world_metrics *out_metrics) {
    if (world == NULL || out_metrics == NULL) {
        return -1;
    }

    memset(out_metrics, 0, sizeof(*out_metrics));
    out_metrics->tick = world->tick;
    out_metrics->aircraft_count = world->aircraft_count;
    out_metrics->projectile_count = world->projectile_count;

    for (size_t i = 0; i < world->aircraft_count; i++) {
        const sim_aircraft *a = &world->aircraft[i];
        if (a->alive) {
            out_metrics->alive_aircraft_count += 1;
        }
        out_metrics->total_health += a->health;
        out_metrics->total_ammo += a->ammo;
    }

    return 0;
}

static float vec3_len(sim_vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static sim_vec3 vec3_add(sim_vec3 a, sim_vec3 b) {
    return (sim_vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static sim_vec3 vec3_scale(sim_vec3 v, float s) {
    return (sim_vec3){v.x * s, v.y * s, v.z * s};
}

static sim_vec3 forward_from_euler(sim_euler o) {
    float cp = cosf(o.x);
    float sp = sinf(o.x);
    float cy = cosf(o.z);
    float sy = sinf(o.z);
    return (sim_vec3){cp * cy, sp, cp * sy};
}

static void spawn_projectile(sim_world *world, size_t owner_idx) {
    if (world->projectile_count >= SIM_MAX_PROJECTILES) {
        return;
    }
    sim_aircraft *owner = &world->aircraft[owner_idx];
    sim_projectile *p = &world->projectiles[world->projectile_count++];
    sim_vec3 forward = forward_from_euler(owner->orientation);
    p->active = 1;
    p->owner_index = (uint8_t)owner_idx;
    p->position = owner->position;
    p->velocity = vec3_add(owner->velocity, vec3_scale(forward, world->config.projectile_speed));
    p->ttl_seconds = world->config.projectile_ttl_seconds;
    p->damage = world->config.projectile_damage;
}

static void update_aircraft(sim_world *world, size_t i) {
    sim_aircraft *a = &world->aircraft[i];
    if (!a->alive) {
        return;
    }

    const sim_control c = world->controls[i];
    const float dt = world->config.dt;

    /* Smooth throttle response to avoid abrupt acceleration. */
    const float throttle_alpha = 2.5f;
    a->throttle_state += (c.throttle - a->throttle_state) * throttle_alpha * dt;
    a->throttle_state = clampf(a->throttle_state, 0.0f, 1.0f);

    const float speed = vec3_len(a->velocity);
    const float aoa = clampf(a->orientation.x, -1.2f, 1.2f);

    const float thrust_coeff = sim_curve_sample(&world->config.thrust_by_speed, speed);
    const float drag_coeff = sim_curve_sample(&world->config.drag_by_speed, speed);
    const float lift_coeff = sim_curve_sample(&world->config.lift_by_aoa, aoa);

    const float roll_input = (c.roll - 0.5f) * 2.0f;
    const float pitch_input = (c.pitch - 0.5f) * 2.0f;

    const float roll_rate_cmd = sim_curve_sample(&world->config.roll_rate_by_input, fabsf(roll_input));
    const float pitch_rate_cmd = sim_curve_sample(&world->config.pitch_rate_by_input, fabsf(pitch_input));

    a->angular_rates.z = roll_rate_cmd * (roll_input < 0.0f ? -1.0f : 1.0f);
    a->angular_rates.x = pitch_rate_cmd * (pitch_input < 0.0f ? -1.0f : 1.0f);

    a->orientation.x = clampf(a->orientation.x + a->angular_rates.x * dt, -1.4f, 1.4f);
    a->orientation.z += a->angular_rates.z * dt;

    const sim_vec3 forward = forward_from_euler(a->orientation);
    const sim_vec3 thrust = vec3_scale(forward, thrust_coeff * a->throttle_state);
    const sim_vec3 drag = vec3_scale(a->velocity, -drag_coeff);
    const sim_vec3 lift = (sim_vec3){0.0f, lift_coeff * speed, 0.0f};
    const sim_vec3 gravity = (sim_vec3){0.0f, -world->config.gravity, 0.0f};

    const sim_vec3 accel = vec3_add(vec3_add(thrust, drag), vec3_add(lift, gravity));
    a->velocity = vec3_add(a->velocity, vec3_scale(accel, dt));
    a->position = vec3_add(a->position, vec3_scale(a->velocity, dt));

    if (a->position.y <= world->config.world_floor_altitude) {
        a->position.y = world->config.world_floor_altitude;
        a->velocity.y = 0.0f;
        if (speed > 70.0f) {
            a->health = 0.0f;
            a->alive = 0;
        }
    }

    if (world->trigger_cooldown[i] > 0.0f) {
        world->trigger_cooldown[i] -= dt;
    }

    if (c.trigger && a->ammo > 0 && world->trigger_cooldown[i] <= 0.0f) {
        spawn_projectile(world, i);
        a->ammo -= 1;
        world->trigger_cooldown[i] = world->config.trigger_cooldown_seconds;
    }
}

static float distance_sq(sim_vec3 a, sim_vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

static void update_projectiles(sim_world *world) {
    const float dt = world->config.dt;
    const float hit_r2 = world->config.hit_radius * world->config.hit_radius;

    for (size_t i = 0; i < world->projectile_count; i++) {
        sim_projectile *p = &world->projectiles[i];
        if (!p->active) {
            continue;
        }

        p->position = vec3_add(p->position, vec3_scale(p->velocity, dt));
        p->ttl_seconds -= dt;
        if (p->ttl_seconds <= 0.0f) {
            p->active = 0;
            continue;
        }

        for (size_t j = 0; j < world->aircraft_count; j++) {
            sim_aircraft *target = &world->aircraft[j];
            if (!target->alive || j == p->owner_index) {
                continue;
            }

            sim_aircraft *owner = &world->aircraft[p->owner_index];
            if (!world->config.friendly_fire && owner->team == target->team) {
                continue;
            }

            if (distance_sq(p->position, target->position) <= hit_r2) {
                target->health -= p->damage;
                if (target->health <= 0.0f) {
                    target->health = 0.0f;
                    target->alive = 0;
                }
                p->active = 0;
                break;
            }
        }
    }

    size_t out = 0;
    for (size_t i = 0; i < world->projectile_count; i++) {
        if (world->projectiles[i].active) {
            world->projectiles[out++] = world->projectiles[i];
        }
    }
    world->projectile_count = out;
}

void sim_tick(sim_world *world) {
    if (world == NULL) {
        return;
    }

    /* Advance aircraft in stable array order for deterministic updates. */
    for (size_t i = 0; i < world->aircraft_count; i++) {
        update_aircraft(world, i);
    }

    update_projectiles(world);

    /* Deterministic RNG advancement keeps random stream aligned even in v1. */
    (void)sim_rng_next(&world->seed);
    world->tick += 1;
}

uint64_t sim_world_hash(const sim_world *world) {
    const uint8_t *bytes = (const uint8_t *)world;
    uint64_t hash = 1469598103934665603ULL;
    const uint64_t prime = 1099511628211ULL;
    for (size_t i = 0; i < sizeof(*world); i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= prime;
    }
    return hash;
}
