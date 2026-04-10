#include "sim.h"

#include <assert.h>
#include <stdio.h>

static const sim_curve_point thrust_by_air_density_curve[] = {
    {0.905f, 7000.0f},
    {1.225f, 8000.0f},
};

static const sim_curve_point lift_curve[] = {
    {-5.0f, 0.0f},
    {0.0f, 0.45f},
    {5.0f, 0.9f},
    {10.0f, 1.35f},
    {15.0f, 1.45f},
    {25.0f, 0.8f},
};

static const sim_curve_point rate_curve[] = {
    {0.0f, 0.0f},
    {1.0f, 1.4f},
};

static sim_world_config make_world_config(void) {
    sim_world_config cfg;
    sim_default_world_config(&cfg);
    return cfg;
}

static sim_plane_stats make_plane_stats(void) {
    sim_plane_stats stats;
    sim_default_plane_stats(&stats);
    stats.thrust_by_air_density =
        (sim_curve){thrust_by_air_density_curve, sizeof(thrust_by_air_density_curve) / sizeof(thrust_by_air_density_curve[0])};
    stats.lift_by_aoa = (sim_curve){lift_curve, sizeof(lift_curve) / sizeof(lift_curve[0])};
    stats.roll_rate_by_input = (sim_curve){rate_curve, sizeof(rate_curve) / sizeof(rate_curve[0])};
    stats.pitch_rate_by_input = (sim_curve){rate_curve, sizeof(rate_curve) / sizeof(rate_curve[0])};
    return stats;
}

static sim_aircraft make_plane(float x, float y, float z, uint8_t team) {
    sim_aircraft a = {
        .position = {x, y, z},
        .velocity = {80.0f, 0.0f, 0.0f},
        .orientation = {0.0f, 0.0f, 0.0f},
        .angular_rates = {0.0f, 0.0f, 0.0f},
        .throttle_state = 0.0f,
        .health = 100.0f,
        .ammo = 100,
        .alive = 1,
        .team = team,
        .plane_stats_index = 0,
    };
    return a;
}

static void run_basic_controls(sim_world *world, int ticks) {
    for (int i = 0; i < ticks; i++) {
        sim_set_control(world, 0, (sim_control){.throttle = 0.8f, .pitch = 0.55f, .roll = 0.5f, .trigger = (i % 5 == 0)});
        if (world->aircraft_count > 1) {
            sim_set_control(world, 1, (sim_control){.throttle = 0.6f, .pitch = 0.5f, .roll = 0.5f, .trigger = 0});
        }
        sim_tick(world);
    }
}

static void test_determinism(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world a;
    sim_world b;

    sim_world_init(&a, &cfg, &stats, 1, 12345);
    sim_world_init(&b, &cfg, &stats, 1, 12345);

    sim_aircraft a0 = make_plane(0, 1200, 0, 0);
    sim_aircraft a1 = make_plane(600, 1200, 0, 1);
    sim_aircraft b0 = make_plane(0, 1200, 0, 0);
    sim_aircraft b1 = make_plane(600, 1200, 0, 1);

    assert(sim_spawn_aircraft(&a, &a0) >= 0);
    assert(sim_spawn_aircraft(&a, &a1) >= 0);

    assert(sim_spawn_aircraft(&b, &b0) >= 0);
    assert(sim_spawn_aircraft(&b, &b1) >= 0);

    run_basic_controls(&a, 600);
    run_basic_controls(&b, 600);

    uint64_t hash_a = sim_world_hash(&a);
    uint64_t hash_b = sim_world_hash(&b);
    assert(hash_a == hash_b);
}

static void test_projectile_damage(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world w;
    sim_world_init(&w, &cfg, &stats, 1, 9);

    sim_aircraft shooter = make_plane(0, 1000, 0, 0);
    shooter.velocity.x = 0.0f;
    sim_aircraft target = make_plane(100, 1000, 0, 1);
    target.velocity.x = 0.0f;

    assert(sim_spawn_aircraft(&w, &shooter) == 0);
    assert(sim_spawn_aircraft(&w, &target) == 1);

    for (int i = 0; i < 120; i++) {
        sim_set_control(&w, 0, (sim_control){.throttle = 0.0f, .pitch = 0.5f, .roll = 0.5f, .trigger = 1});
        sim_set_control(&w, 1, (sim_control){.throttle = 0.0f, .pitch = 0.5f, .roll = 0.5f, .trigger = 0});
        sim_tick(&w);
    }

    assert(w.aircraft[1].health < 100.0f);
    assert(w.aircraft[0].ammo < 100);
}

static void test_config_validation_and_defaults(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    assert(sim_validate_world_config(&cfg) == 1);
    assert(sim_validate_plane_stats(&stats) == 1);

    cfg.schema_version = 999u;
    assert(sim_validate_world_config(&cfg) == 0);

    sim_world w;
    sim_world_init(&w, &cfg, &stats, 1, 77u);
    assert(w.world_config.schema_version == SIM_WORLD_CONFIG_SCHEMA_VERSION);
    assert(w.world_config.dt > 0.0f);
}

static void test_telemetry_output(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world w;
    sim_world_init(&w, &cfg, &stats, 1, 42u);
    sim_aircraft plane = make_plane(0.0f, 1500.0f, 0.0f, 0);
    assert(sim_spawn_aircraft(&w, &plane) == 0);

    sim_set_control(&w, 0, (sim_control){.throttle = 1.0f, .pitch = 0.6f, .roll = 0.5f, .trigger = 0});
    sim_tick(&w);

    sim_aircraft_telemetry telemetry;
    assert(sim_get_aircraft_telemetry(&w, 0, &telemetry) == 0);
    assert(telemetry.altitude > 0.0f);
    assert(telemetry.speed > 0.0f);
    assert(telemetry.throttle >= 0.0f);
}

static void test_world_snapshot_output(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world w;
    sim_world_init(&w, &cfg, &stats, 1, 111u);
    sim_aircraft a0 = make_plane(0.0f, 1400.0f, 0.0f, 0);
    sim_aircraft a1 = make_plane(200.0f, 1400.0f, 0.0f, 1);
    assert(sim_spawn_aircraft(&w, &a0) == 0);
    assert(sim_spawn_aircraft(&w, &a1) == 1);

    sim_set_control(&w, 0, (sim_control){.throttle = 0.9f, .pitch = 0.5f, .roll = 0.5f, .trigger = 1});
    sim_set_control(&w, 1, (sim_control){.throttle = 0.7f, .pitch = 0.5f, .roll = 0.5f, .trigger = 0});
    sim_tick(&w);

    sim_world_snapshot snapshot;
    assert(sim_get_world_snapshot(&w, &snapshot) == 0);
    assert(snapshot.tick == w.tick);
    assert(snapshot.aircraft_count == 2);
    assert(snapshot.alive_aircraft_count == 2);
    assert(snapshot.projectile_count == w.projectile_count);
    assert(snapshot.aircraft[0].speed > 0.0f);
    assert(snapshot.aircraft[1].altitude > 0.0f);
}

static void test_config_serialization_roundtrip(void) {
    sim_world_config cfg = make_world_config();
    cfg.friendly_fire = 1;
    cfg.world_floor_altitude = -100.0f;

    char serialized[512];
    int size = sim_world_config_serialize(&cfg, serialized, sizeof(serialized));
    assert(size > 0);

    sim_world_config parsed;
    assert(sim_world_config_deserialize(&parsed, serialized) == 0);
    assert(parsed.schema_version == cfg.schema_version);
    assert(parsed.friendly_fire == cfg.friendly_fire);
    assert(parsed.projectile_damage == cfg.projectile_damage);
    assert(parsed.world_floor_altitude == cfg.world_floor_altitude);

    assert(sim_world_config_deserialize(&parsed, "not=a_config\n") == -1);
}

static void test_world_lifecycle_helpers(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world *world = sim_world_create(&cfg, &stats, 1, 2026u);
    assert(world != NULL);
    assert(world->tick == 0);
    assert(world->seed == 2026u);

    sim_aircraft plane = make_plane(0.0f, 1000.0f, 0.0f, 0);
    assert(sim_spawn_aircraft(world, &plane) == 0);
    sim_tick(world);
    assert(world->tick == 1);

    sim_world_reset(world, &cfg, &stats, 1, 4040u);
    assert(world->tick == 0);
    assert(world->seed == 4040u);
    assert(world->aircraft_count == 0);

    sim_world_destroy(world);
}

static void test_world_metrics_output(void) {
    sim_world_config cfg = make_world_config();
    sim_plane_stats stats = make_plane_stats();
    sim_world w;
    sim_world_init(&w, &cfg, &stats, 1, 55u);

    sim_aircraft p0 = make_plane(0.0f, 1200.0f, 0.0f, 0);
    sim_aircraft p1 = make_plane(300.0f, 1200.0f, 0.0f, 1);
    assert(sim_spawn_aircraft(&w, &p0) == 0);
    assert(sim_spawn_aircraft(&w, &p1) == 1);

    sim_set_control(&w, 0, (sim_control){.throttle = 0.8f, .pitch = 0.5f, .roll = 0.5f, .trigger = 1});
    sim_set_control(&w, 1, (sim_control){.throttle = 0.8f, .pitch = 0.5f, .roll = 0.5f, .trigger = 0});
    sim_tick(&w);

    sim_world_metrics metrics;
    assert(sim_get_world_metrics(&w, &metrics) == 0);
    assert(metrics.tick == w.tick);
    assert(metrics.aircraft_count == 2);
    assert(metrics.alive_aircraft_count <= 2);
    assert(metrics.total_health > 0.0f);
    assert(metrics.total_ammo > 0);
}

int main(void) {
    test_determinism();
    test_projectile_damage();
    test_config_validation_and_defaults();
    test_telemetry_output();
    test_world_snapshot_output();
    test_config_serialization_roundtrip();
    test_world_lifecycle_helpers();
    test_world_metrics_output();
    printf("All simulation tests passed.\n");
    return 0;
}
