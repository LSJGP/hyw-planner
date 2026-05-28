#include "cpp/service/session_manager.h"

#include <sstream>

namespace hyw_planner {

std::string SessionManager::CreateSession(
    const std::string& planner_name, const hyw_sim::proto::PlannerInputs& inputs,
    std::string* error) {
  auto planner = CreatePlanner(planner_name, inputs, error);
  if (!planner) {
    return {};
  }
  std::ostringstream sid;
  sid << "sess-" << next_id_++;
  const std::string session_id = sid.str();
  SessionEntry entry;
  entry.planner = std::move(planner);
  entry.name = entry.planner->Name();
  std::lock_guard<std::mutex> lk(mu_);
  sessions_.emplace(session_id, std::move(entry));
  return session_id;
}

Planner* SessionManager::Get(const std::string& session_id) {
  std::lock_guard<std::mutex> lk(mu_);
  const auto it = sessions_.find(session_id);
  if (it == sessions_.end()) {
    return nullptr;
  }
  return it->second.planner.get();
}

std::string SessionManager::PlannerName(const std::string& session_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  const auto it = sessions_.find(session_id);
  if (it == sessions_.end()) {
    return {};
  }
  return it->second.name;
}

bool SessionManager::Close(const std::string& session_id) {
  std::lock_guard<std::mutex> lk(mu_);
  return sessions_.erase(session_id) > 0;
}

size_t SessionManager::Size() const {
  std::lock_guard<std::mutex> lk(mu_);
  return sessions_.size();
}

}  // namespace hyw_planner
