#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "cpp/core/planner.h"

namespace hyw_planner {

class SessionManager {
 public:
  std::string CreateSession(const std::string& planner_name,
                            const hyw_sim::proto::PlannerInputs& inputs,
                            std::string* error);
  Planner* Get(const std::string& session_id);
  std::string PlannerName(const std::string& session_id) const;
  bool Close(const std::string& session_id);
  size_t Size() const;

 private:
  struct SessionEntry {
    std::unique_ptr<Planner> planner;
    std::string name;
  };

  mutable std::mutex mu_;
  std::unordered_map<std::string, SessionEntry> sessions_;
  uint64_t next_id_ = 1;
};

}  // namespace hyw_planner
