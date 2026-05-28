#pragma once

#include "proto/sim/runtime.pb.h"

namespace hyw_planner {

void StepVehicle(hyw_sim::proto::VehicleState* ego,
                 const hyw_sim::proto::PlanCommand& cmd, double dt,
                 const hyw_sim::proto::VehicleParams& params);

hyw_sim::proto::PlannerTrajectory BuildTrajectoryFromCommand(
    const hyw_sim::proto::PlanCommand& cmd,
    const hyw_sim::proto::VehicleState& ego,
    const hyw_sim::proto::VehicleParams& params,
    const hyw_sim::proto::PlannerConfig& config);

}  // namespace hyw_planner
