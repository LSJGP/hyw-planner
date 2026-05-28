# hyw-planner

独立 C++ planner 进程，通过 gRPC 为 `hyw-sim` 提供规划服务。

## 构建

```bash
cd hyw-planner
bazel build //cpp:planner_server
```

## 运行

```bash
bazel run //cpp:planner_server -- --port 50051
```

## 内置 planner

- `reference_tracker`
- `goal_seek`
- `local_dwa`
