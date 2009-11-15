/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "SimTKcommon.h"

#include "simbody/internal/common.h"
#include "simbody/internal/MobilizedBody.h"
#include "simbody/internal/SimbodyMatterSubsystem.h"
#include "simbody/internal/Force.h"

#include "ForceImpl.h"

namespace SimTK {

const GeneralForceSubsystem& Force::getForceSubsystem() const {
	return getImpl().getForceSubsystem();
}
ForceIndex Force::getForceIndex() const {
    return getImpl().getForceIndex();
}

//-------------------------- TwoPointLinearSpring ------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::TwoPointLinearSpring, Force::TwoPointLinearSpringImpl, Force);

Force::TwoPointLinearSpring::TwoPointLinearSpring(GeneralForceSubsystem& forces, const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real k, Real x0) : Force(new TwoPointLinearSpringImpl(
        body1, station1, body2, station2, k, x0)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::TwoPointLinearSpringImpl::TwoPointLinearSpringImpl(const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real k, Real x0) : matter(body1.getMatterSubsystem()),
        body1(body1.getMobilizedBodyIndex()), station1(station1),
        body2(body2.getMobilizedBodyIndex()), station2(station2), k(k), x0(x0) {
}

void Force::TwoPointLinearSpringImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const Transform& X_GB1 = matter.getMobilizedBody(body1).getBodyTransform(state);
    const Transform& X_GB2 = matter.getMobilizedBody(body2).getBodyTransform(state);

    const Vec3 s1_G = X_GB1.R() * station1;
    const Vec3 s2_G = X_GB2.R() * station2;

    const Vec3 p1_G = X_GB1.p() + s1_G; // station measured from ground origin
    const Vec3 p2_G = X_GB2.p() + s2_G;

    const Vec3 r_G		 = p2_G - p1_G; // vector from point1 to point2
    const Real d		 = r_G.norm();  // distance between the points
    const Real stretch   = d - x0;		// + -> tension, - -> compression
    const Real frcScalar = k*stretch;	// k(x-x0)

    const Vec3 f1_G = (frcScalar/d) * r_G;
    bodyForces[body1] +=  SpatialVec(s1_G % f1_G, f1_G);
    bodyForces[body2] -=  SpatialVec(s2_G % f1_G, f1_G);
}

Real Force::TwoPointLinearSpringImpl::calcPotentialEnergy(const State& state) const {
    const Transform& X_GB1 = matter.getMobilizedBody(body1).getBodyTransform(state);
    const Transform& X_GB2 = matter.getMobilizedBody(body2).getBodyTransform(state);

    const Vec3 s1_G = X_GB1.R() * station1;
    const Vec3 s2_G = X_GB2.R() * station2;

    const Vec3 p1_G = X_GB1.p() + s1_G; // station measured from ground origin
    const Vec3 p2_G = X_GB2.p() + s2_G;

    const Vec3 r_G = p2_G - p1_G; // vector from point1 to point2
    const Real d   = r_G.norm();  // distance between the points
    const Real stretch   = d - x0; // + -> tension, - -> compression

    return 0.5*k*stretch*stretch; // 1/2 k (x-x0)^2
}


//-------------------------- TwoPointLinearDamper ------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::TwoPointLinearDamper, Force::TwoPointLinearDamperImpl, Force);

Force::TwoPointLinearDamper::TwoPointLinearDamper(GeneralForceSubsystem& forces, const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real damping) : Force(new TwoPointLinearDamperImpl(
        body1, station1, body2, station2, damping)) {
    assert(damping >= 0);
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::TwoPointLinearDamperImpl::TwoPointLinearDamperImpl(const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real damping) : matter(body1.getMatterSubsystem()),
        body1(body1.getMobilizedBodyIndex()), station1(station1),
        body2(body2.getMobilizedBodyIndex()), station2(station2), damping(damping) {
}

void Force::TwoPointLinearDamperImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const Transform& X_GB1 = matter.getMobilizedBody(body1).getBodyTransform(state);
    const Transform& X_GB2 = matter.getMobilizedBody(body2).getBodyTransform(state);

    const Vec3 s1_G = X_GB1.R() * station1;
    const Vec3 s2_G = X_GB2.R() * station2;

    const Vec3 p1_G = X_GB1.p() + s1_G; // station measured from ground origin
    const Vec3 p2_G = X_GB2.p() + s2_G;

    const Vec3 v1_G = matter.getMobilizedBody(body1).findStationVelocityInGround(state, station1);
    const Vec3 v2_G = matter.getMobilizedBody(body2).findStationVelocityInGround(state, station2);
    const Vec3 vRel = v2_G - v1_G; // relative velocity

    const UnitVec3 d(p2_G - p1_G); // direction from point1 to point2
    const Real frc = damping*dot(vRel, d); // c*v

    const Vec3 f1_G = frc*d;
    bodyForces[body1] +=  SpatialVec(s1_G % f1_G, f1_G);
    bodyForces[body2] -=  SpatialVec(s2_G % f1_G, f1_G);
}

Real Force::TwoPointLinearDamperImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//-------------------------- TwoPointConstantForce -----------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::TwoPointConstantForce, Force::TwoPointConstantForceImpl, Force);

Force::TwoPointConstantForce::TwoPointConstantForce(GeneralForceSubsystem& forces, const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real force) : Force(new TwoPointConstantForceImpl(
        body1, station1, body2, station2, force)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::TwoPointConstantForceImpl::TwoPointConstantForceImpl(const MobilizedBody& body1, const Vec3& station1,
        const MobilizedBody& body2, const Vec3& station2, Real force) : matter(body1.getMatterSubsystem()),
        body1(body1.getMobilizedBodyIndex()), station1(station1),
        body2(body2.getMobilizedBodyIndex()), station2(station2), force(force) {
}

void Force::TwoPointConstantForceImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const Transform& X_GB1 = matter.getMobilizedBody(body1).getBodyTransform(state);
    const Transform& X_GB2 = matter.getMobilizedBody(body2).getBodyTransform(state);

    const Vec3 s1_G = X_GB1.R() * station1;
    const Vec3 s2_G = X_GB2.R() * station2;

    const Vec3 p1_G = X_GB1.p() + s1_G; // station measured from ground origin
    const Vec3 p2_G = X_GB2.p() + s2_G;

    const Vec3 r_G = p2_G - p1_G; // vector from point1 to point2
    const Real x   = r_G.norm();  // distance between the points
    const UnitVec3 d(r_G/x, true);

    const Vec3 f2_G = force * d;
    bodyForces[body1] -=  SpatialVec(s1_G % f2_G, f2_G);
    bodyForces[body2] +=  SpatialVec(s2_G % f2_G, f2_G);
}

Real Force::TwoPointConstantForceImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//--------------------------- MobilityLinearSpring -----------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::MobilityLinearSpring, Force::MobilityLinearSpringImpl, Force);

Force::MobilityLinearSpring::MobilityLinearSpring(GeneralForceSubsystem& forces, const MobilizedBody& body, int coordinate,
        Real k, Real x0) : Force(new MobilityLinearSpringImpl(body, coordinate, k, x0)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::MobilityLinearSpringImpl::MobilityLinearSpringImpl(const MobilizedBody& body, int coordinate,
        Real k, Real x0) : matter(body.getMatterSubsystem()), body(body.getMobilizedBodyIndex()), coordinate(coordinate), k(k), x0(x0) {
}

void Force::MobilityLinearSpringImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const MobilizedBody& mb = matter.getMobilizedBody(body);
    const Real q = mb.getOneQ(state, coordinate);
    const Real frc = -k*(q-x0);
    mb.applyOneMobilityForce(state, coordinate, frc, mobilityForces);
}

Real Force::MobilityLinearSpringImpl::calcPotentialEnergy(const State& state) const {
    const MobilizedBody& mb = matter.getMobilizedBody(body);
    const Real q = mb.getOneQ(state, coordinate);
    const Real frc = -k*(q-x0);
    return 0.5*k*(q-x0)*(q-x0);
}



//--------------------------- MobilityLinearDamper -----------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::MobilityLinearDamper, Force::MobilityLinearDamperImpl, Force);

Force::MobilityLinearDamper::MobilityLinearDamper(GeneralForceSubsystem& forces, const MobilizedBody& body, int coordinate,
        Real damping) : Force(new MobilityLinearDamperImpl(body, coordinate, damping)) {
    assert(damping >= 0);
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::MobilityLinearDamperImpl::MobilityLinearDamperImpl(const MobilizedBody& body, int coordinate,
        Real damping) : matter(body.getMatterSubsystem()), body(body.getMobilizedBodyIndex()), coordinate(coordinate), damping(damping) {
}

void Force::MobilityLinearDamperImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const MobilizedBody& mb = matter.getMobilizedBody(body);
    const Real u = mb.getOneU(state, coordinate);
    const Real frc = -damping*u;
    mb.applyOneMobilityForce(state, coordinate, frc, mobilityForces);
}

Real Force::MobilityLinearDamperImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}



//-------------------------- MobilityConstantForce -----------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::MobilityConstantForce, Force::MobilityConstantForceImpl, Force);

Force::MobilityConstantForce::MobilityConstantForce(GeneralForceSubsystem& forces, const MobilizedBody& body, int coordinate,
        Real force) : Force(new MobilityConstantForceImpl(body, coordinate, force)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::MobilityConstantForceImpl::MobilityConstantForceImpl(const MobilizedBody& body, int coordinate,
        Real force) : matter(body.getMatterSubsystem()), body(body.getMobilizedBodyIndex()), coordinate(coordinate), force(force) {
}

void Force::MobilityConstantForceImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const MobilizedBody& mb = matter.getMobilizedBody(body);
    const Real q = mb.getOneQ(state, coordinate);
    mb.applyOneMobilityForce(state, coordinate, force, mobilityForces);
}

Real Force::MobilityConstantForceImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//------------------------------ LinearBushing ---------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::LinearBushing, 
                                        Force::LinearBushingImpl, Force);

Force::LinearBushing::LinearBushing
   (GeneralForceSubsystem& forces, 
    const MobilizedBody& bodyA, const Transform& frameOnA,
    const MobilizedBody& bodyB, const Transform& frameOnB,
    const Vec6& stiffness,  const Vec6&  damping) 
:   Force(new LinearBushingImpl(bodyA,frameOnA,bodyB,frameOnB,
                                stiffness,damping))
{
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}


Force::LinearBushing::LinearBushing
   (GeneralForceSubsystem& forces, 
    const MobilizedBody& bodyA, // assume body frames
    const MobilizedBody& bodyB,
    const Vec6& stiffness,  const Vec6&  damping) 
:   Force(new LinearBushingImpl(bodyA,Transform(),bodyB,Transform(),
                                stiffness,damping))
{
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

const Vec6& Force::LinearBushing::
getQ(const State& s) const 
{   getImpl().ensurePositionCacheValid(s); 
    return getImpl().getPositionCache(s).q; }
const Vec6& Force::LinearBushing::
getQDot(const State& s) const 
{   getImpl().ensureVelocityCacheValid(s); 
    return getImpl().getVelocityCache(s).qdot; }

const Transform& Force::LinearBushing::
getX_FM(const State& s) const 
{   getImpl().ensurePositionCacheValid(s); 
    return getImpl().getPositionCache(s).X_FM; }
const SpatialVec& Force::LinearBushing::
getV_FM(const State& s) const 
{   getImpl().ensureVelocityCacheValid(s); 
    return getImpl().getVelocityCache(s).V_FM; }

const Vec6& Force::LinearBushing::
getF(const State& s) const
{   getImpl().ensureForceCacheValid(s); 
    return getImpl().getForceCache(s).f; }
const SpatialVec& Force::LinearBushing::
getF_GF(const State& s) const
{   getImpl().ensureForceCacheValid(s); 
    return getImpl().getForceCache(s).F_GF; }
const SpatialVec& Force::LinearBushing::
getF_GM(const State& s) const
{   getImpl().ensureForceCacheValid(s); 
    return getImpl().getForceCache(s).F_GM; }

const Real& Force::LinearBushing::
getPotentialEnergy(const State& s) const
{   getImpl().ensurePotentialEnergyValid(s); 
    return getImpl().getPotentialEnergyCache(s); }

Force::LinearBushingImpl::LinearBushingImpl
   (const MobilizedBody& bodyA, const Transform& frameOnA,
    const MobilizedBody& bodyB, const Transform& frameOnB,
    const Vec6& stiffness, const Vec6& damping)
:   bodyA(bodyA), X_AF(frameOnA),
    bodyB(bodyB), X_BM(frameOnB), 
    k(stiffness), c(damping) 
{
}

void Force::LinearBushingImpl::
ensurePositionCacheValid(const State& state) const {
    if (isPositionCacheValid(state)) return;

    PositionCache& pc = updPositionCache(state);

    const Transform& X_GA = bodyA.getBodyTransform(state);
    const Transform& X_GB = bodyB.getBodyTransform(state);
    pc.X_GF =     X_GA*   X_AF;   // 63 flops
    pc.X_GM =     X_GB*   X_BM;   // 63 flops
    pc.X_FM = ~pc.X_GF*pc.X_GM;   // 63 flops

    // Re-express local vectors in the Ground frame.
    pc.p_AF_G =    X_GA.R() *    X_AF.p();    // 15 flops
    pc.p_BM_G =    X_GB.R() *    X_BM.p();    // 15 flops
    pc.p_FM_G = pc.X_GF.R() * pc.X_FM.p();    // 15 flops

    // Calculate the 1-2-3 body B-fixed Euler angles; these are the
    // rotational coordinates.
    pc.q.updSubVec<3>(0) = pc.X_FM.R().convertRotationToBodyFixedXYZ();

    // The translation vector in X_FM contains our translational coordinates.
    pc.q.updSubVec<3>(3) = pc.X_FM.p();

    markPositionCacheValid(state);
}

void Force::LinearBushingImpl::
ensureVelocityCacheValid(const State& state) const {
    if (isVelocityCacheValid(state)) return;

    // We'll be needing this.
    ensurePositionCacheValid(state);
    const PositionCache& pc = getPositionCache(state);
    const Rotation& R_GF = pc.X_GF.R();
    const Rotation& R_FM = pc.X_FM.R();
    const Vec3&     q    = pc.q.getSubVec<3>(0);
    const Vec3&     p    = pc.q.getSubVec<3>(3);

    VelocityCache& vc = updVelocityCache(state);

    // Now do velocities.
    const SpatialVec& V_GA = bodyA.getBodyVelocity(state);
    const SpatialVec& V_GB = bodyB.getBodyVelocity(state);

    vc.V_GF = SpatialVec(V_GA[0], V_GA[1] + V_GA[0] % pc.p_AF_G);
    vc.V_GM = SpatialVec(V_GB[0], V_GB[1] + V_GB[0] % pc.p_BM_G);

    // This is the velocity of M in F, but with the time derivative
    // taken in the Ground frame.
    const SpatialVec V_FM_G = vc.V_GM - vc.V_GF;

    // To get derivative in F, we must remove the part due to the
    // angular velocity w_GF of F in G.
    vc.V_FM = ~R_GF * SpatialVec(V_FM_G[0], 
                                 V_FM_G[1] - vc.V_GF[0] % pc.p_FM_G);

    // Need angular velocity in M frame for conversion to qdot.
    const Vec3  w_FM_M = ~R_FM*vc.V_FM[0];
    const Mat33 N_FM   = Rotation::calcNForBodyXYZInBodyFrame(q);
    vc.qdot.updSubVec<3>(0) = N_FM * w_FM_M;
    vc.qdot.updSubVec<3>(3) = vc.V_FM[1];

    markVelocityCacheValid(state);
}

// This will also calculate potential energy since we can do it on the
// cheap simultaneously with the force.
void Force::LinearBushingImpl::
ensureForceCacheValid(const State& state) const {
    if (isForceCacheValid(state)) return;

    ForceCache& fc = updForceCache(state);

    ensurePositionCacheValid(state);
    const PositionCache& pc = getPositionCache(state);

    const Transform& X_GA = bodyA.getBodyTransform(state);
    const Rotation&  R_GA = X_GA.R();

    const Transform& X_GB = bodyB.getBodyTransform(state);
    const Rotation&  R_GB = X_GB.R();

    const Vec3&      p_AF = X_AF.p();
    const Vec3&      p_BM = X_BM.p();

    const Rotation&  R_GF = pc.X_GF.R();
    const Vec3&      p_GF = pc.X_GF.p();

    const Rotation&  R_GM = pc.X_GM.R();
    const Vec3&      p_GM = pc.X_GM.p();

    const Rotation&  R_FM = pc.X_FM.R();
    const Vec3&      p_FM = pc.X_FM.p();

    // Calculate stiffness generalized forces and potential
    // energy (cheap to do here).
    const Vec6& q = pc.q;
    Vec6 fk; Real pe2=0;
    for (int i=0; i<6; ++i) pe2 += (fk[i]=k[i]*q[i])*q[i]; 
    updPotentialEnergyCache(state) = pe2/2;
    markPotentialEnergyValid(state);

    ensureVelocityCacheValid(state);
    const VelocityCache& vc = getVelocityCache(state);

    const Vec6& qd = vc.qdot;
    Vec6 fv;
    for (int i=0; i<6; ++i) fv[i]=c[i]*qd[i];

    fc.f             = -(fk+fv); // generalized forces on body B
    const Vec3& fB_q = fc.f.getSubVec<3>(0); // in q basis
    const Vec3& fM_F = fc.f.getSubVec<3>(3); // acts at M, but exp. in F frame

    // Calculate the matrix relating q-space generalized forces to a real-space
    // moment vector. We know qforce = ~H * moment (where H is the
    // the hinge matrix for a mobilizer using qdots as generalized speeds).
    // In that case H would be N^-1, qforce = ~(N^-1)*moment so
    // moment = ~N*qforce. Caution: our N wants the moment in the outboard
    // body frame, in this case M.
    const Mat33 N_FM = Rotation::calcNForBodyXYZInBodyFrame(q.getSubVec<3>(0));
    const Vec3  mB_M = ~N_FM * fB_q; // moment acting on body B, exp. in M
    const Vec3  mB_G = R_GM*mB_M;    // moment on B, now exp. in G

    // Transform force from F frame to ground. This is the force to 
    // apply to body B at point OM; -f goes on body A at the same
    // spatial location. Here we actually apply it at OF, and since
    // we know the force acts along the line OF-OM the change in 
    // location does not generate a moment.
    const Vec3 fM_G = R_GF*fM_F;

    fc.F_GM = SpatialVec(mB_G, fM_G);
    fc.F_GF = -fc.F_GM; // see above for why force is OK here w/o shift

    // Shift forces to body origins.
    fc.F_GB = SpatialVec(fc.F_GM[0] + pc.p_BM_G % fc.F_GM[1], fc.F_GM[1]);
    fc.F_GA = SpatialVec(fc.F_GF[0] + pc.p_AF_G % fc.F_GF[1], fc.F_GF[1]);

    markForceCacheValid(state);
}

// This calculate is only performed if the PE is requested without
// already having calculated the force.
void Force::LinearBushingImpl::
ensurePotentialEnergyValid(const State& state) const {
    if (isPotentialEnergyValid(state)) return;

    ensurePositionCacheValid(state);
    const PositionCache& pc = getPositionCache(state);
    const Vec6& q = pc.q;

    Real pe2=0;
    for (int i=0; i<6; ++i) pe2 += k[i]*square(q[i]);

    updPotentialEnergyCache(state) = pe2 / 2;
    markPotentialEnergyValid(state);
}

void Force::LinearBushingImpl::
calcForce(const State& state, Vector_<SpatialVec>& bodyForces, 
          Vector_<Vec3>& particleForces, Vector& mobilityForces) const 
{
    const MobilizedBodyIndex bodyAx = bodyA.getMobilizedBodyIndex();
    const MobilizedBodyIndex bodyBx = bodyB.getMobilizedBodyIndex();

    ensureForceCacheValid(state);
    const ForceCache& fc = getForceCache(state);
    bodyForces[bodyBx] +=  fc.F_GB;
    bodyForces[bodyAx] +=  fc.F_GA; // apply forces
}

// If the force was calculated, then the potential energy will already
// be valid. Otherwise we'll have to calculate it.
Real Force::LinearBushingImpl::
calcPotentialEnergy(const State& state) const {
    ensurePotentialEnergyValid(state);
    return getPotentialEnergyCache(state);
}



//------------------------------ ConstantForce ---------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::ConstantForce, Force::ConstantForceImpl, Force);

Force::ConstantForce::ConstantForce(GeneralForceSubsystem& forces, const MobilizedBody& body, const Vec3& station, const Vec3& force) :
        Force(new ConstantForceImpl(body, station, force)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::ConstantForceImpl::ConstantForceImpl(const MobilizedBody& body, const Vec3& station, const Vec3& force) :
        matter(body.getMatterSubsystem()), body(body.getMobilizedBodyIndex()), station(station), force(force) {
}

void Force::ConstantForceImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const Transform& X_GB = matter.getMobilizedBody(body).getBodyTransform(state);
    const Vec3 station_G = X_GB.R() * station;
    bodyForces[body] += SpatialVec(station_G % force, force);
}

Real Force::ConstantForceImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//------------------------------ ConstantTorque --------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::ConstantTorque, Force::ConstantTorqueImpl, Force);

Force::ConstantTorque::ConstantTorque(GeneralForceSubsystem& forces, const MobilizedBody& body, const Vec3& torque) :
        Force(new ConstantTorqueImpl(body, torque)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::ConstantTorqueImpl::ConstantTorqueImpl(const MobilizedBody& body, const Vec3& torque) :
        matter(body.getMatterSubsystem()), body(body.getMobilizedBodyIndex()), torque(torque) {
}

void Force::ConstantTorqueImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    bodyForces[body][0] += torque;
}

Real Force::ConstantTorqueImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//------------------------------- GlobalDamper ---------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::GlobalDamper, Force::GlobalDamperImpl, Force);

Force::GlobalDamper::GlobalDamper(GeneralForceSubsystem& forces, const SimbodyMatterSubsystem& matter,
        Real damping) : Force(new GlobalDamperImpl(matter, damping)) {
    assert(damping >= 0);
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::GlobalDamperImpl::GlobalDamperImpl(const SimbodyMatterSubsystem& matter, Real damping) : matter(matter), damping(damping) {
}

void Force::GlobalDamperImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    mobilityForces -= damping*matter.getU(state);
}

Real Force::GlobalDamperImpl::calcPotentialEnergy(const State& state) const {
    return 0;
}


//-------------------------------- Thermostat ----------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::Thermostat, Force::ThermostatImpl, Force);

Force::Thermostat::Thermostat
   (GeneralForceSubsystem& forces, const SimbodyMatterSubsystem& matter,
    Real boltzmannsConstant, Real bathTemperature, Real relaxationTime) 
:   Force(new ThermostatImpl(matter, boltzmannsConstant, bathTemperature, relaxationTime))
{
	SimTK_APIARGCHECK1_ALWAYS(boltzmannsConstant > 0, 
		"Force::Thermostat","ctor", "Illegal Boltzmann's constant %g.", boltzmannsConstant);
	SimTK_APIARGCHECK1_ALWAYS(bathTemperature > 0, 
		"Force::Thermostat","ctor", "Illegal bath temperature %g.", bathTemperature);
	SimTK_APIARGCHECK1_ALWAYS(relaxationTime > 0, 
		"Force::Thermostat","ctor", "Illegal relaxation time %g.", relaxationTime);

    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Force::Thermostat& Force::Thermostat::setDefaultNumChains(int numChains) {
	SimTK_APIARGCHECK1_ALWAYS(numChains > 0, 
		"Force::Thermostat","setDefaultNumChains", 
		"Illegal number of chains %d.", numChains);

	getImpl().invalidateTopologyCache();
	updImpl().defaultNumChains = numChains;
	return *this;
}

Force::Thermostat& Force::Thermostat::setDefaultBathTemperature(Real bathTemperature) {
	SimTK_APIARGCHECK1_ALWAYS(bathTemperature > 0, 
		"Force::Thermostat","setDefaultBathTemperature", 
		"Illegal bath temperature %g.", bathTemperature);

	getImpl().invalidateTopologyCache();
	updImpl().defaultBathTemp = bathTemperature;
	return *this;
}

Force::Thermostat& Force::Thermostat::setDefaultRelaxationTime(Real relaxationTime) {
	SimTK_APIARGCHECK1_ALWAYS(relaxationTime > 0, 
		"Force::Thermostat","setDefaultRelaxationTime", 
		"Illegal bath temperature %g.", relaxationTime);

	getImpl().invalidateTopologyCache();
	updImpl().defaultRelaxationTime = relaxationTime;
	return *this;
}

int Force::Thermostat::getDefaultNumChains() const {return getImpl().defaultNumChains;}
Real Force::Thermostat::getDefaultBathTemperature() const {return getImpl().defaultBathTemp;}
Real Force::Thermostat::getDefaultRelaxationTime() const {return getImpl().defaultRelaxationTime;}
Real Force::Thermostat::getBoltzmannsConstant() const {return getImpl().Kb;}

void Force::Thermostat::setNumChains(State& s, int numChains) const {
	SimTK_APIARGCHECK1_ALWAYS(numChains > 0, 
		"Force::Thermostat","setNumChains", 
		"Illegal number of chains %d.", numChains);

	getImpl().updNumChains(s) = numChains;
}

void Force::Thermostat::setBathTemperature(State& s, Real bathTemperature) const {
	SimTK_APIARGCHECK1_ALWAYS(bathTemperature > 0, 
		"Force::Thermostat","setBathTemperature", 
		"Illegal bath temperature %g.", bathTemperature);

	getImpl().updBathTemp(s) = bathTemperature;
}

void Force::Thermostat::setRelaxationTime(State& s, Real relaxationTime) const {
	SimTK_APIARGCHECK1_ALWAYS(relaxationTime > 0, 
		"Force::Thermostat","setRelaxationTime", 
		"Illegal bath temperature %g.", relaxationTime);

	getImpl().updRelaxationTime(s) = relaxationTime;
}

int Force::Thermostat::getNumChains(const State& s) const {return getImpl().getNumChains(s);}
Real Force::Thermostat::getBathTemperature(const State& s) const {return getImpl().getBathTemp(s);}
Real Force::Thermostat::getRelaxationTime(const State& s) const {return getImpl().getRelaxationTime(s);}

void Force::Thermostat::initializeChainState(State& s) const {
	const ThermostatImpl& impl = getImpl();
	const int nChains = impl.getNumChains(s);
	for (int i=0; i < 2*nChains; ++i)
		impl.updZ(s, i) = 0;
}

void Force::Thermostat::setChainState(State& s, const Vector& z) const {
	const ThermostatImpl& impl = getImpl();
	const int nChains = impl.getNumChains(s);
	SimTK_APIARGCHECK2_ALWAYS(z.size() == 2*nChains,
		"Force::Thermostat", "setChainState", 
		"Number of values supplied (%d) didn't match the number of chains %d.", z.size(), nChains);
	for (int i=0; i < 2*nChains; ++i)
		impl.updZ(s, i) = z[i];
}

Vector Force::Thermostat::getChainState(const State& s) const {
	const ThermostatImpl& impl = getImpl();
	const int nChains = impl.getNumChains(s);
	Vector out(2*nChains);
	for (int i=0; i < 2*nChains; ++i)
		out[i] = impl.getZ(s, i);
	return out;
}


Real Force::Thermostat::getCurrentTemperature(const State& s) const {
	const Real ke = getImpl().getKE(s);	// Cached value for kinetic energy
	const Real Kb = getImpl().Kb;		// Boltzmann's constant
	const int  N  = getImpl().getNumDOFs(s);
	return (2*ke) / (N*Kb);
}

int Force::Thermostat::getNumDegreesOfFreedom(const State& s) const {
	return getImpl().getNumDOFs(s);
}

// Bath energy is KEb + PEb where
//    KEb = 1/2 kT t^2 (N z0^2 + sum(zi^2))
//    PEb = kT (N s0 + sum(si))
Real Force::Thermostat::calcBathEnergy(const State& state) const {
	const ThermostatImpl& impl = getImpl();
	const int nChains = impl.getNumChains(state);
	const int  N = impl.getNumDOFs(state);
	const Real kT = impl.Kb * impl.getBathTemp(state);
	const Real t = impl.getRelaxationTime(state);

	Real zsqsum = N * square(impl.getZ(state,0));
	for (int i=1; i < nChains; ++i)
		zsqsum += square(impl.getZ(state,i));

	Real ssum = N * impl.getZ(state, nChains);
	for (int i=1; i < nChains; ++i)
		ssum += impl.getZ(state, nChains+i);

	const Real KEb = (kT/2) * t*t * zsqsum;
	const Real PEb = kT * ssum;

	return KEb + PEb;
}

// This is the number of dofs. TODO: we're ignoring constraint redundancy
// but we shouldn't be! That could result in negative dofs, so we'll 
// make sure that doesn't happen. But don't expect meaningful results
// in that case. Note that it is the acceleration-level constraints that
// matter; they remove dofs regardless of whether there is a corresponding
// velocity constraint.
int Force::ThermostatImpl::getNumDOFs(const State& state) const {
	const int N = std::max(1, state.getNU() - state.getNUDotErr());
	return N;
}

void Force::ThermostatImpl::calcForce
   (const State& state, Vector_<SpatialVec>&, Vector_<Vec3>&, Vector& mobilityForces) const 
{
	Vector p;	// momentum per mobility
	matter.calcMV(state, state.getU(), p);

	// Generate momentum-weighted forces and apply to mobilities.
    mobilityForces -= getZ(state, 0)*p;
}

// Allocate and initialize state variables.
void Force::ThermostatImpl::realizeTopology(State& state) const {
	// Make these writable just here where we need to fill in the Topology "cache"
	// variables; after this they are const.
	Force::ThermostatImpl* mutableThis = const_cast<Force::ThermostatImpl*>(this);
	mutableThis->dvNumChains = 
		getForceSubsystem().allocateDiscreteVariable(state, Stage::Model, 
												     new Value<int>(defaultNumChains));
	mutableThis->dvBathTemp = 
		getForceSubsystem().allocateDiscreteVariable(state, Stage::Instance, 
												     new Value<Real>(defaultBathTemp));
	mutableThis->dvRelaxationTime = 
		getForceSubsystem().allocateDiscreteVariable(state, Stage::Instance, 
												     new Value<Real>(defaultRelaxationTime));

	// This cache entry holds the auxiliary state index of our first
	// thermostat state variable. It is valid after realizeModel().
	mutableThis->cacheZ0Index = 
		getForceSubsystem().allocateCacheEntry(state, Stage::Model, 
											   new Value<ZIndex>());

	// This cache entry holds the generalized momentum M*u. The vector
	// will be allocated to hold nu values.
	mutableThis->cacheMomentumIndex =
		getForceSubsystem().allocateCacheEntry(state, Stage::Velocity, 
											   new Value<Vector>());

	// This cache entry holds the kinetic energy ~u*M*u/2.
	mutableThis->cacheKEIndex =
		getForceSubsystem().allocateCacheEntry(state, Stage::Velocity, 
											   new Value<Real>(NaN));
}

// Allocate the chain state variables and bath energy variables.
// TODO: this should be done at Instance stage.
void Force::ThermostatImpl::realizeModel(State& state) const {
	const int nChains = getNumChains(state);
	const Vector zInit(2*nChains, Zero);
	updZ0Index(state) = getForceSubsystem().allocateZ(state, zInit);
}

// Calculate velocity-dependent terms.
void Force::ThermostatImpl::realizeVelocity(const State& state) const {
	Vector& Mu = updMomentum(state);
	matter.calcMV(state, state.getU(), Mu);
	updKE(state) = (~state.getU() * Mu) / 2;
}

void Force::ThermostatImpl::realizeDynamics(const State& state) const {
	const Real t	= getRelaxationTime(state);
	const Real oot2 = 1 / square(t);
	const int  m	= getNumChains(state);

	// This is the desired kinetic energy per dof.
	const Real Eb = Kb * getBathTemp(state) / 2;

	const int  N = getNumDOFs(state);

	// This is the current average kinetic energy per dof.
	const Real E = getKE(state) / N;

	updZDot(state, 0) = (E/Eb - 1) * oot2;

	int Ndofs = N;	// only for z0
	for (int k=1; k < m; ++k) {
		const Real zk1 = getZ(state, k-1);
		const Real zk  = getZ(state, k);
		updZDot(state, k-1) -= zk1 * zk;
		updZDot(state, k) = Ndofs * square(zk1) - oot2;
		Ndofs = 1; // z1..m-1 control only 1 dof each
	}

	// Calculate sdot's for energy calculation.
	for (int k=0; k < m; ++k)
		updZDot(state, m+k) = getZ(state, k);
}


//------------------------------ UniformGravity --------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::UniformGravity, Force::UniformGravityImpl, Force);

Force::UniformGravity::UniformGravity(GeneralForceSubsystem& forces, const SimbodyMatterSubsystem& matter,
        const Vec3& g, Real zeroHeight) : Force(new UniformGravityImpl(matter, g, zeroHeight)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

Vec3 Force::UniformGravity::getGravity() const {
    return getImpl().getGravity();
}

void Force::UniformGravity::setGravity(const Vec3& g) {
    updImpl().setGravity(g);
}

Real Force::UniformGravity::getZeroHeight() const {
    return getImpl().getZeroHeight();
}

void Force::UniformGravity::setZeroHeight(Real height) {
    updImpl().setZeroHeight(height);
}

Force::UniformGravityImpl::UniformGravityImpl(const SimbodyMatterSubsystem& matter, const Vec3& g, Real zeroHeight) : matter(matter), g(g), zeroHeight(zeroHeight) {
}

void Force::UniformGravityImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    const int nBodies    = matter.getNumBodies();
    const int nParticles = matter.getNumParticles();

    if (nParticles) {
        const Vector& m = matter.getAllParticleMasses(state);
        const Vector_<Vec3>& loc_G = matter.getAllParticleLocations(state);
        for (int i=0; i < nParticles; ++i) {
            particleForces[i] += g * m[i];
        }
    }

    // no need to apply gravity to Ground!
    for (MobilizedBodyIndex i(1); i < nBodies; ++i) {
        const MassProperties& mprops = matter.getMobilizedBody(i).getBodyMassProperties(state);
        const Real&      m       = mprops.getMass();
        const Vec3&      com_B   = mprops.getMassCenter();
        const Transform& X_GB    = matter.getMobilizedBody(i).getBodyTransform(state);
        const Vec3       com_B_G = X_GB.R()*com_B;
        const Vec3       frc_G   = m*g;

        bodyForces[i] += SpatialVec(com_B_G % frc_G, frc_G); 
    }
}

Real Force::UniformGravityImpl::calcPotentialEnergy(const State& state) const {
    const int nBodies    = matter.getNumBodies();
    const int nParticles = matter.getNumParticles();
    Real pe = 0.0;

    if (nParticles) {
        const Vector& m = matter.getAllParticleMasses(state);
        const Vector_<Vec3>& loc_G = matter.getAllParticleLocations(state);
        for (int i=0; i < nParticles; ++i) {
            pe -= m[i]*(~g*loc_G[i] + zeroHeight); // odd signs because height is in -g direction
        }
    }

    // no need to apply gravity to Ground!
    for (MobilizedBodyIndex i(1); i < nBodies; ++i) {
        const MassProperties& mprops = matter.getMobilizedBody(i).getBodyMassProperties(state);
        const Real&      m       = mprops.getMass();
        const Vec3&      com_B   = mprops.getMassCenter();
        const Transform& X_GB    = matter.getMobilizedBody(i).getBodyTransform(state);
        const Vec3       com_B_G = X_GB.R()*com_B;
        const Vec3       com_G   = X_GB.p() + com_B_G;

        pe -= m*(~g*com_G + zeroHeight); // odd signs because height is in -g direction
    }
    return pe;
}


//---------------------------------- Custom ------------------------------------
//------------------------------------------------------------------------------

SimTK_INSERT_DERIVED_HANDLE_DEFINITIONS(Force::Custom, Force::CustomImpl, Force);

Force::Custom::Custom(GeneralForceSubsystem& forces, Implementation* implementation) : 
        Force(new CustomImpl(implementation)) {
    updImpl().setForceSubsystem(forces, forces.adoptForce(*this));
}

const Force::Custom::Implementation& Force::Custom::getImplementation() const {
    return getImpl().getImplementation();
}

Force::Custom::Implementation& Force::Custom::updImplementation() {
    return updImpl().updImplementation();    
}


Force::CustomImpl::CustomImpl(Force::Custom::Implementation* implementation) : implementation(implementation) {
}

void Force::CustomImpl::calcForce(const State& state, Vector_<SpatialVec>& bodyForces, Vector_<Vec3>& particleForces, Vector& mobilityForces) const {
    implementation->calcForce(state, bodyForces, particleForces, mobilityForces);
}

Real Force::CustomImpl::calcPotentialEnergy(const State& state) const {
    return implementation->calcPotentialEnergy(state);
}

} // namespace SimTK

