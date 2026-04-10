export type CurvePoint2D = {
    x: number;
    y: number;
};

export type CurvePoint3D = {
    x: number;
    y: number;
};

export type Curve2D = CurvePoint2D[];
export type Curve3D = CurvePoint3D[];

export type PlaneStats = {
    weight: number; //kg
    thrustBySpeedAndAltitude: Curve2D; //Speed as input
    dragBySpeedAndAltitude: Curve3D; //Speed, Altitude

    /*
        Lift Equation: L = (1/2 * p) * V^2 * S * C
        L: Total Lift
        p: Air Density
        V: Velocity
        S: Surface Area
        C: Coefficient of Lift (Includes Angle of Attack)

        We simplify this to: L = 0.5 * V^2 * cs
        We will assume angle of attack is always flat for simplicty for now
        Each plane type will have a single lift stat, which will represent Surface area * Lift Coefficent

    */
    liftCoefficient: number;

    thrust;
};

export function generateFlat2DCurve(
    minx: number,
    maxx: number,
    flatValue: number,
): Curve2D {
    return [{
        x: minx,
        y: flatValue,
    }, {
        x: maxx,
        y: flatValue,
    }];
}
