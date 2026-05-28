#include <iostream>
#include <memory>
#include <string>

#include "cpp/service/planner_service_impl.h"
#include "cpp/service/session_manager.h"
#include "grpcpp/grpcpp.h"

namespace {

struct ServerArgs {
  std::string address = "0.0.0.0";
  int port = 50051;
};

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " [--address <host>] [--port <port>]\n";
}

bool ParseArgs(int argc, char** argv, ServerArgs* args) {
  for (int i = 1; i < argc; ++i) {
    const std::string k = argv[i];
    auto next = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++i];
    };
    if (k == "--address") {
      args->address = next("--address");
    } else if (k == "--port") {
      args->port = std::stoi(next("--port"));
    } else if (k == "-h" || k == "--help") {
      PrintUsage(argv[0]);
      return false;
    } else {
      throw std::runtime_error("unknown arg: " + k);
    }
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  ServerArgs args;
  try {
    if (!ParseArgs(argc, argv, &args)) {
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "[planner_server] bad args: " << e.what() << "\n";
    PrintUsage(argv[0]);
    return 1;
  }

  hyw_planner::SessionManager sessions;
  hyw_planner::PlannerServiceImpl service(&sessions);

  const std::string server_address = args.address + ":" + std::to_string(args.port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  if (!server) {
    std::cerr << "[planner_server] failed to listen on " << server_address << "\n";
    return 2;
  }

  std::cout << "[planner_server] listening on " << server_address << "\n";
  server->Wait();
  return 0;
}
