#include "cpp/service/planner_service_impl.h"

namespace hyw_planner {

PlannerServiceImpl::PlannerServiceImpl(SessionManager* sessions)
    : sessions_(sessions) {}

grpc::Status PlannerServiceImpl::ListPlanners(
    grpc::ServerContext* /*context*/,
    const hyw_planner::proto::ListPlannersRequest* /*request*/,
    hyw_planner::proto::ListPlannersResponse* response) {
  for (const auto& name : AvailablePlannerNames()) {
    response->add_planner_names(name);
  }
  return grpc::Status::OK;
}

grpc::Status PlannerServiceImpl::CreateSession(
    grpc::ServerContext* /*context*/,
    const hyw_planner::proto::CreateSessionRequest* request,
    hyw_planner::proto::CreateSessionResponse* response) {
  std::string error;
  const std::string session_id =
      sessions_->CreateSession(request->planner_name(), request->inputs(), &error);
  if (session_id.empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error);
  }
  response->set_session_id(session_id);
  response->set_planner_name(sessions_->PlannerName(session_id));
  return grpc::Status::OK;
}

grpc::Status PlannerServiceImpl::Plan(
    grpc::ServerContext* /*context*/,
    const hyw_planner::proto::PlanRequest* request,
    hyw_planner::proto::PlanResponse* response) {
  Planner* planner = sessions_->Get(request->session_id());
  if (planner == nullptr) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "unknown session");
  }
  *response->mutable_trajectory() = planner->Plan(request->observation());
  return grpc::Status::OK;
}

grpc::Status PlannerServiceImpl::CloseSession(
    grpc::ServerContext* /*context*/,
    const hyw_planner::proto::CloseSessionRequest* request,
    hyw_planner::proto::CloseSessionResponse* /*response*/) {
  if (!sessions_->Close(request->session_id())) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "unknown session");
  }
  return grpc::Status::OK;
}

grpc::Status PlannerServiceImpl::Health(
    grpc::ServerContext* /*context*/,
    const hyw_planner::proto::HealthRequest* /*request*/,
    hyw_planner::proto::HealthResponse* response) {
  response->set_ok(true);
  response->set_version("hyw-planner-1");
  return grpc::Status::OK;
}

}  // namespace hyw_planner
