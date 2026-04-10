export type CurvePoint2D = {
    x: number;
    y: number;
};

export type Curve2D = CurvePoint2D[];

export type PlaneStats = {
    weight: number; //kg
    /*
        Lift Equation: L = (1/2 * p) * V^2 * S * C
        L: Total Lift
        p: Air Density
        V: Velocity
        S: Surface Area
        C: Coefficient of Lift (Includes Angle of Attack)

        We simplify this to: L = 0.5 * V^2 * cs
        We will assume angle of attack is always flat for simplicty for now
        Each plane type will have a single lift stat, cs, which will represent Surface area * Lift Coefficent
    */
    liftCoefficientByAoa: Curve2D; // constant, depending on Angle of Attack
    wingSurfaceArea: number; //meter squared

    /*
        Parasite Drag equation: D = 1/2 * p * V^2 * Cdo * S
        p: Air Density
        V: Velocity
        Cdo: Zero Lift Drag Coefficient
        S: Reference Area
    */
    parasiteDragReferenceArea: number; //meters squared
    parasiteDragCoefficient: number; //constant

    /*
        Thrust is modeled as a function of air density. In engine physics we will derive
        local air density from altitude (assuming fixed temperature) and sample this curve.
        Output Value is in newtons.
    */
    thrustByAirDensity: Curve2D;

    /*
        Induced drag equation
        Di = 1/2 * p * V^2 * S * Dci
        Dci = (C ^ 2) / (pi * AR * e)

        We already use C (liftCoefficientByAoa) and we will need a new stat (AR * e) 
    */
   AspectRatioOswaldConstant: number;
};
