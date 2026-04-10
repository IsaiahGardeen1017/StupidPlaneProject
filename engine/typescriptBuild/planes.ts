import { PlaneStats } from "./utils";

export const planes: Record<string, PlaneStats> = {
    'bf109': {
        weight: 2700,
        liftCoefficientByAoa: [{x: -5, y: 0.0}, {x: 0, y: 0.45}, {x: 5, y: 0.9}, {x: 10, y: 1.35}, {x: 15, y: 1.45}, {x: 25, y: 0.8}], //Don't know if reasonable
        wingSurfaceArea: 16.2,
        parasiteDragReferenceArea: 0.3,
        parasiteDragCoefficient: 0.03,
        thrustByAirDensity: [{x: 1.2250, y: 8000}, {x: 0.905, y: 7000}],
        AspectRatioOswaldConstant: 6.14 * 0.75
    }
}
