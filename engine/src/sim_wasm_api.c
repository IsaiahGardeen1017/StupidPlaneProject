#include "sim.h"

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

static sim_world g_world;
static int g_initialized = 0;

static const sim_curve_point g_thrust_by_air_density[] = {
    {0.905f, 7000.0f},
    {1.225f, 8000.0f},
};

static const sim_curve_point g_lift_by_aoa[] = {
    {-5.0f, 0.0f},
    {0.0f, 0.45f},
    {5.0f, 0.9f},
    {10.0f, 1.35f},
    {15.0f, 1.45f},
    {25.0f, 0.8f},
};

static const sim_curve_point g_rate_curve[] = {
    {0.0f, 0.0f},
    {1.0f, 1.4f},
};

static void sim_wasm_build_defaults(sim_world_config *out_world_cfg, sim_plane_stats *out_stats) {
    sim_default_world_config(out_world_cfg);
    sim_default_plane_stats(out_stats);

    out_stats->thrust_by_air_density =
        (sim_curve){g_thrust_by_air_density, sizeof(g_thrust_by_air_density) / sizeof(g_thrust_by_air_density[0])};
    out_stats->lift_by_aoa = (sim_curve){g_lift_by_aoa, sizeof(g_lift_by_aoa) / sizeof(g_lift_by_aoa[0])};
    out_stats->roll_rate_by_input = (sim_curve){g_rate_curve, sizeof(g_rate_curve) / sizeof(g_rate_curve[0])};
    out_stats->pitch_rate_by_input = (sim_curve){g_rate_curve, sizeof(g_rate_curve) / sizeof(g_rate_curve[0])};
}

EMSCRIPTEN_KEEPALIVE int sim_wasm_init(void) {
    sim_world_config world_cfg;
    sim_plane_stats plane_stats;
    sim_wasm_build_defaults(&world_cfg, &plane_stats);

    sim_world_init(&g_world, &world_cfg, &plane_stats, 1, 123u);

    sim_aircraft player = {
        .position = {0.0f, 1200.0f, 0.0f},
        .velocity = {80.0f, 0.0f, 0.0f},
        .orientation = {0.0f, 0.0f, 0.0f},
        .angular_rates = {0.0f, 0.0f, 0.0f},
        .throttle_state = 0.0f,
        .health = 100.0f,
        .ammo = 250,
        .alive = 1,
        .team = 0,
        .plane_stats_index = 0,
    };

    if (sim_spawn_aircraft(&g_world, &player) < 0) {
        return -1;
    }

    g_initialized = 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE void sim_wasm_reset(void) {
    (void)sim_wasm_init();
}

EMSCRIPTEN_KEEPALIVE void sim_wasm_set_control(float throttle, float pitch, float roll, int trigger) {
    if (!g_initialized || g_world.aircraft_count == 0) {
        return;
    }

    sim_set_control(
        &g_world,
        0,
        (sim_control){
            .throttle = throttle,
            .pitch = pitch,
            .roll = roll,
            .trigger = trigger ? 1 : 0,
        });
}

EMSCRIPTEN_KEEPALIVE void sim_wasm_tick(void) {
    if (!g_initialized) {
        return;
    }
    sim_tick(&g_world);
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_speed(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.speed;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_altitude(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.altitude;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_climb_rate(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.climb_rate;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_pitch_attitude(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.pitch_attitude;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_roll_attitude(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.roll_attitude;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_turn_rate(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.turn_rate;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_aoa_estimate(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.aoa_estimate;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_throttle(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.throttle;
}

EMSCRIPTEN_KEEPALIVE float sim_wasm_get_health(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0.0f;
    }
    return telemetry.health;
}

EMSCRIPTEN_KEEPALIVE int sim_wasm_get_ammo(void) {
    sim_aircraft_telemetry telemetry;
    if (!g_initialized || sim_get_aircraft_telemetry(&g_world, 0, &telemetry) != 0) {
        return 0;
    }
    return telemetry.ammo;
}
