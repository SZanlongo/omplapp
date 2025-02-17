#include <ompl/base/goals/GoalState.h>
#include <ompl/base/spaces/SE2StateSpace.h>
#include <ompl/base/spaces/DiscreteStateSpace.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/config.h>
#include <iostream>
#include <limits>
#include <boost/math/constants/constants.hpp>

namespace ob = ompl::base;
namespace oc = ompl::control;

void propagate(const oc::SpaceInformation *si, const ob::State *state, const oc::Control* control, const double duration, ob::State *result) {
    static double timeStep = .01;
    int nsteps = ceil(duration / timeStep);
    double dt = duration / nsteps;
    const double *u = control->as<oc::RealVectorControlSpace::ControlType>()->values;

    ob::CompoundStateSpace::StateType& s = *result->as<ob::CompoundStateSpace::StateType>();
    ob::SE2StateSpace::StateType& se2 = *s.as<ob::SE2StateSpace::StateType>(0);
    ob::RealVectorStateSpace::StateType& velocity = *s.as<ob::RealVectorStateSpace::StateType>(1);
    // ob::DiscreteStateSpace::StateType& gear = *s.as<ob::DiscreteStateSpace::StateType>(2);

    si->getStateSpace()->copyState(result, state);
    for (int i = 0; i < nsteps; i++) {
        se2.setX(se2.getX() + dt * velocity.values[0] * cos(se2.getYaw()));
        se2.setY(se2.getY() + dt * velocity.values[0] * sin(se2.getYaw()));
        se2.setYaw(se2.getYaw() + dt * u[0]);
        // velocity.values[0] = velocity.values[0] + dt * (u[1]*gear.value);
        velocity.values[0] = velocity.values[0] + dt * u[1];

        // 'guards' - conditions to change gears
        // if (gear.value > 0)
        // {
        //     if (gear.value < 3 && velocity.values[0] > 10*(gear.value + 1))
        //         gear.value++;
        //     else if (gear.value > 1 && velocity.values[0] < 10*gear.value)
        //         gear.value--;
        // }

        if (!si->satisfiesBounds(result))
            return;
    }
}

// The free space consists of two narrow corridors connected at right angle.
// To make the turn, the car will have to downshift.
bool isStateValid(const oc::SpaceInformation *si, const ob::State *state) {
    const ob::SE2StateSpace::StateType *se2 = state->as<ob::CompoundState>()->as<ob::SE2StateSpace::StateType>(0);
    // return si->satisfiesBounds(state) && (se2->getX() < -80. || se2->getY() > 80.);

    // check if bounds are satisfied
    // if (si->satisfiesBounds(state)) {        
        if ((se2->getX() > -60. && se2->getX() < -30 && se2->getY() > -60. && se2->getY() < -30) ||
            (se2->getX() > -60. && se2->getX() < -30 && se2->getY() > 30. && se2->getY() < 60) ||
            (se2->getX() > 30. && se2->getX() < 60 && se2->getY() > 30. && se2->getY() < 60)) {
            return false;
        } else {
            return true;
        }
    // } else {    
    //     return false;
    // }    
}

int main(int, char**) {
    // plan for hybrid car in SE(2) with discrete gears
    ob::StateSpacePtr SE2(new ob::SE2StateSpace());
    ob::StateSpacePtr velocity(new ob::RealVectorStateSpace(1));
    // set the range for gears: [-1,3] inclusive
    // ob::StateSpacePtr gear(new ob::DiscreteStateSpace(-1,3));
    ob::StateSpacePtr stateSpace = SE2 + velocity;

    // set the bounds for the R^2 part of SE(2)
    ob::RealVectorBounds bounds(2);
    bounds.setLow(-100);
    bounds.setHigh(100);
    SE2->as<ob::SE2StateSpace>()->setBounds(bounds);

    // set the bounds for the velocity
    ob::RealVectorBounds velocityBound(1);
    velocityBound.setLow(0);
    velocityBound.setHigh(60);
    velocity->as<ob::RealVectorStateSpace>()->setBounds(velocityBound);

    // create start and goal states
    ob::ScopedState<> start(stateSpace);
    ob::ScopedState<> goal(stateSpace);

    // Both start and goal are states with high velocity with the car in third gear.
    // However, to make the turn, the car cannot stay in third gear and will have to
    // shift to first gear.
    start[0] = -79.6228; //position
    start[1] = -81.93; //position
    start[2] = -0.911333; // orientation
    // start[3] = 40.; // velocity
    // start->as<ob::CompoundState>()->as<ob::DiscreteStateSpace::StateType>(2)->value = 3; // gear

    goal[0] = -50; // position
    goal[1] = -50; //position
    // goal[2] = 0; // orientation
    // goal[3] = 40.; // velocity
    // goal->as<ob::CompoundState>()->as<ob::DiscreteStateSpace::StateType>(2)->value = 3; // gear

    oc::ControlSpacePtr cmanifold(new oc::RealVectorControlSpace(stateSpace, 2));

    // set the bounds for the control manifold
    ob::RealVectorBounds cbounds(2);
    // bounds for steering input
    cbounds.setLow(0, -3.);
    cbounds.setHigh(0, 3.);
    // bounds for brake/gas input
    cbounds.setLow(1, -20.);
    cbounds.setHigh(1, 20.);
    cmanifold->as<oc::RealVectorControlSpace>()->setBounds(cbounds);

    oc::SimpleSetup setup(cmanifold);
    setup.setStartAndGoalStates(start, goal, 5.);
    setup.setStateValidityChecker(boost::bind(&isStateValid, setup.getSpaceInformation().get(), _1));
    setup.setStatePropagator(boost::bind(&propagate, setup.getSpaceInformation().get(), _1, _2, _3, _4));
    setup.getSpaceInformation()->setPropagationStepSize(.1);
    setup.getSpaceInformation()->setMinMaxControlDuration(2, 3);

    // try to solve the problem
    if (setup.solve(30)) {
        // print the (approximate) solution path: print states along the path
        // and controls required to get from one state to the next
        oc::PathControl& path(setup.getSolutionPath());

        // print out full state on solution path
        // (format: x, y, theta, v, u0, u1, dt)
        for (unsigned int i = 0; i < path.getStateCount(); ++i) {
            const ob::State* state = path.getState(i);
            const ob::SE2StateSpace::StateType *se2 = state->as<ob::CompoundState>()->as<ob::SE2StateSpace::StateType>(0);
            const ob::RealVectorStateSpace::StateType *velocity = state->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1);

            std::cout << se2->getX() << ' ' << se2->getY() << ' ' << se2->getYaw() << ' ' << velocity->values[0] << ' ';
            if (i == 0)
                // null controls applied for zero seconds to get to start state
                std::cout << "0 0 0";
            else {
                // print controls and control duration needed to get from state i-1 to state i
                const double* u = path.getControl(i - 1)->as<oc::RealVectorControlSpace::ControlType>()->values;
                std::cout << u[0] << ' ' << u[1] << ' ' << path.getControlDuration(i - 1);
            }
            std::cout << std::endl;
        }
        if (!setup.haveExactSolutionPath()) {
            std::cout << "Solution is approximate. Distance to actual goal is " << setup.getProblemDefinition()->getSolutionDifference() << std::endl;
        }
    }

    return 0;
}
