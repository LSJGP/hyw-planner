#pragma once

#include "cpp/service/session_manager.h"
#include "proto/planner/planner_service.grpc.pb.h"

namespace hyw_planner {

class PlannerServiceImpl final
    : public hyw_planner::proto::PlannerService::Service {
 public:
  explicit PlannerServiceImpl(SessionManager* sessions);

  grpc::Status ListPlanners(grpc::ServerContext* context,
                            const hyw_planner::proto::ListPlannersRequest* request,
                            hyw_planner::proto::ListPlannersResponse* response) override;

  grpc::Status CreateSession(grpc::ServerContext* context,
                             const hyw_planner::proto::CreateSessionRequest* request,
                             hyw_planner::proto::CreateSessionResponse* response) override;

  grpc::Status Plan(grpc::ServerContext* context,
                    const hyw_planner::proto::PlanRequest* request,
                    hyw_planner::proto::PlanResponse* response) override;

  grpc::Status CloseSession(grpc::ServerContext* context,
                            const hyw_planner::proto::CloseSessionRequest* request,
                            hyw_planner::proto::CloseSessionResponse* response) override;

  grpc::Status Health(grpc::ServerContext* context,
                      const hyw_planner::proto::HealthRequest* request,
                      hyw_planner::proto::HealthResponse* response) override;

 private:
  SessionManager* sessions_;
};

}  // namespace hyw_planner
