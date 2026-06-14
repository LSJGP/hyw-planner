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

仿真侧通过 `--planner pdms_hack`（或 workbench dashboard 下拉选择）连接该服务。若端口被旧进程占用，workbench 会自动尝试 `50052+`。

## 内置 planner

| 名称 | 说明 |
|------|------|
| `reference_tracker` | 沿地图参考线跟踪 |
| `goal_seek` | 朝 goal 直线趋近 |
| `local_dwa` | 局部 DWA，带前车限速与路沿偏置 |
| `pdms_hack` | 针对 PDMS 评分的红队 planner（见下文） |

---

## `pdms_hack` — PDMS 红队 planner

`pdms_hack` 是一个**故意利用 PDMS 评分结构弱点**的实验性 planner，用于验证 `hyw-grading` 的 PDMS 聚合逻辑是否会被「驾驶质量差但分数高」的策略绕过。实现位于 `cpp/core/planner.cc` 中的 `PdmsHackPlanner`。

它与仿真、评分使用**同一条** `planned_trajectory`（无幻影轨迹）：grading 看到的 planned path 就是车辆实际执行的轨迹。

### 设计动机

PDMS 由硬性惩罚项与软性加权项相乘得到（详见仓库根目录 [`pdms_guide.md`](../pdms_guide.md)）：

```
PDMS = (NC × DAC × SL) × weighted_avg(EP, TTC, C, Speed)
```

其中 NC / DAC / SL 任一帧违规即归零；EP / TTC / C / Speed 则按帧通过率或末帧分数做加权平均，允许部分帧不达标。`pdms_hack` 的目标是：**保持硬性项为 1**，同时在软指标上「够用即可」，从而用明显非正常的驾驶风格换高分。

主要利用点：

1. **Comfort（C）低速豁免** — `hard_braking_checker` 仅在速度 ≥ 1.0 m/s 时检测急刹。低速磨蹭阶段可规避减速度惩罚。
2. **EP 末帧计分** — `ego_progress_checker` 只取最后一帧的沿程进度。前期故意慢、末期冲刺仍可在末帧拿到较高 EP。
3. **TTC 邻道盲区** — `collision_risk_checker` 对横向偏离参考路径较远的 NPC 过滤较严。低速阶段不激进抢道，可降低 TTC 风险帧占比。
4. **软指标平均** — 少量风险帧可被大量「安全但无意义」的低速帧稀释。

### 行为：磨蹭 → 冲刺

`PdmsHackPlanner` 采用两阶段状态机，底层跟踪逻辑复用 `reference_tracker` 风格的参考线跟踪（`ComputeReferenceCommand` + `LeaderLimitedSpeedDwa` 前车限速）。

```
┌─────────────────────────────────────────────────────────────┐
│  CREEP（磨蹭）                                               │
│  条件：frame_id < 48 且 距 goal > 25 m                       │
│  且 ego 速度已自然降至 ≤ 1.0 m/s 时，才钳制目标速度为 0.5 m/s │
│  目的：Comfort 豁免 + 稀释 TTC 风险帧                        │
└──────────────────────────┬──────────────────────────────────┘
                           │ 距 goal ≤ 25 m 或 frame ≥ 48
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  SPRINT（冲刺）                                              │
│  巡航上限 6.0 m/s，减速度钳制 3.5 m/s²                       │
│  沿参考线朝 goal 推进，末段拉高 EP                             │
└─────────────────────────────────────────────────────────────┘
```

关键参数（`planner.cc` 内 `constexpr`）：

| 参数 | 值 | 含义 |
|------|-----|------|
| `kCreepFrameCutoff` | 48 | 超过该帧号强制退出磨蹭阶段 |
| `kCreepDistThreshold` | 25 m | 距 goal 小于此值进入冲刺 |
| `kCreepSpeed` | 0.5 m/s | 磨蹭阶段速度上限 |
| `kSprintCruiseSpeed` | 6.0 m/s | 冲刺阶段巡航上限 |
| `kSprintDecelCap` | 3.5 m/s² | 冲刺阶段最大减速度（低于 Comfort 4.0 阈值） |
| `kGoalDeadzone` | 1.5 m | 进入 goal 邻域后停车 |

早期版本在仿真起始就强制低速，容易触发 NC 碰撞；当前实现**仅在 ego 已自然减速到 ≤ 1 m/s 后才启用磨蹭钳制**，避免起步阶段与 NPC 纠缠。

### 使用方式

**Workbench / 批量脚本**

在 dashboard 的 planner 下拉框选择 `pdms_hack`，或通过 `batch_run_scenarios.py` 的 `PLANNERS` 列表发起仿真。需确保连接的是**本仓库构建**的 `planner_server`（含 `pdms_hack` 注册），而非其他用户残留在 `50051` 上的旧进程。

**命令行**

```bash
# 终端 1：启动 planner
cd hyw-planner && bazel run //cpp:planner_server -- --port 50051

# 终端 2：仿真（示例）
cd hyw-sim
python run_sim.py \
  --scenario-dir ../hyw-workbench/scenarios/waymo_scenario_0 \
  --planner pdms_hack \
  --planner-address 127.0.0.1:50051 \
  --reference-source map
```

### `waymo_scenario_0` 对照实验

以下数据来自同一场景、同一评分配置的两次离线 grading 报告：

| | `local_dwa` | `pdms_hack` |
|---|-------------|-------------|
| 报告目录 | `hyw-workbench/dwa_info/20260612_170854_waymo_scenario_0/` | `hyw-workbench/hack_info/20260612_170446_waymo_scenario_0/` |
| **PDMS** | **0.885 FAIL** | **0.950 PASS** |
| NC / DAC / SL | 1 / 1 / 1 | 1 / 1 / 1 |
| EP（末帧） | 0.894 | 0.898 |
| TTC（通过率） | 0.844（14/90 风险帧） | 0.978（2/90 风险帧） |
| C（通过率） | 0.933（6/90 急刹帧） | 1.000（0/90） |
| Speed | 1 | 1 |

两者均未发生真实碰撞（NC = 1），也均未越界/压实线；差距集中在**软指标帧通过率**，而非硬性一票否决项。

#### 1. 汇总行：PDMS 从 FAIL 翻到 PASS

`local_dwa` 的 `summary.json` 中 `pdms_aggregator`：

```
detail: "pdms=0.884758 penalties=1 weighted_avg=0.884758 frames=90
         NC=1 DAC=1 SL=1 EP=0.894118(final_frame) TTC=0.844444 C=0.933333 Speed=1"
overallPassed: false
```

`pdms_hack` 的对应行：

```
detail: "pdms=0.950137 penalties=1 weighted_avg=0.950137 frames=90
         NC=1 DAC=1 SL=1 EP=0.897566(final_frame) TTC=0.977778 C=1 Speed=1"
overallPassed: true
```

EP 末帧分数几乎相同（0.894 vs 0.898，均未达 EP 子项 0.95 阈值），**PDMS 过线主要靠 TTC 与 C 的提升**。

#### 2. Comfort：急刹帧 6 → 0

`local_dwa` 的 `hard_braking_checker.json` 在起步和末段多次触发减速度 < −4 m/s²：

```
frame=0  t=0s   v=5.66 FAIL   currentAcceleration=-6.00
frame=1  t=0.1s v=5.06 FAIL   currentAcceleration=-6.00
frame=83 t=8.3s v=4.81 FAIL   currentAcceleration=-6.00
frame=89 t=8.9s v=2.77 FAIL   currentAcceleration=-4.88
summary: violation_frames=6/90
```

`pdms_hack` 将冲刺阶段减速度钳制在 3.5 m/s²（低于 4.0 阈值），同文件为：

```
summary: violation_frames=0/90
```

C 子分项：0.933 → 1.000，直接贡献约 +0.02 的加权平均。

#### 3. TTC：高速风险簇 14 帧 → 2 帧

`local_dwa` 在 t ≈ 2.6–3.9 s（frame 26–39）以 ~11–12 m/s 巡航时连续报 FAIL，无几何碰撞但 TTC 判定逼近过快：

```
frame=26 t=2.6s v=11.31 coll=n FAIL
frame=30 t=3.0s v=12.06 coll=n FAIL
frame=39 t=3.9s v=12.13 coll=n FAIL
summary: risky_frames=14/90 overlap_frames=14 imminent_frames=14 min_ttc_s=0.000000
```

`pdms_hack` 仅在前 0.2 s 有 2 帧瞬态风险，之后全程 PASS：

```
frame=1 t=0.1s v=6.76 coll=n FAIL
frame=2 t=0.2s v=7.01 coll=n FAIL
summary: risky_frames=2/90 overlap_frames=0 imminent_frames=2 min_ttc_s=1.102
```

TTC 通过率 0.844 → 0.978，是 PDMS 提升的主要来源。

#### 4. 仿真速度曲线：驾驶风格相近，评分敏感点不同

两份 `waymo_scenario_0_sim_log.json` 的平均速度接近（dwa 8.76 m/s，hack 8.78 m/s），但**时序分布**不同：

| 时刻 | `local_dwa` 速度 | `pdms_hack` 速度 |
|------|------------------|------------------|
| t = 0 s | 5.66 m/s | 6.51 m/s |
| t = 3 s | **12.06 m/s**（TTC 风险簇） | 12.56 m/s |
| t = 4.8 s | 11.06 m/s | 10.46 m/s |
| t = 8.9 s | 2.77 m/s（末段急刹） | 3.71 m/s |

`local_dwa` 在中段维持更高巡航并伴随更猛的加减速；`pdms_hack` 通过减速度上限和更保守的跟车限速，减少了 TTC/C 违规帧，末帧 EP 仅略高（`alongRatio` 0.946 vs 0.948）。

> 本场景全程速度均 > 3 m/s，未触发「≤ 1 m/s 磨蹭钳制」；得分优势主要来自**减速度钳制**与**更保守的速度剖面**，而非极低-speed 磨蹭。

### 已知效果与局限

以下场景**不适合**用来验证本 planner：

- **SDC 路线退化**（起点 ≈ 终点）— EP 恒为 0，PDMS 上限约 0.6，例如 `waymo_scenario_5`。
- **地图路由不可用** — `--input-format auto` 可能选中残缺的 `lane_graph.pb`；workbench 会自动回退到 json，见 `hyw-workbench/pysim/waymo_sim/scenario.py` 中 `resolve_scenario_input_format`。

`pdms_hack` 是评分鲁棒性测试工具，**不是**可部署的驾驶策略。若需加固 PDMS，可从以下方向入手：取消 Comfort 低速豁免、EP 改为全程积分或最低分、TTC 扩大邻道相关车辆范围、对磨蹭类行为增加进度速率惩罚等。

### 相关文档

- PDMS 公式与子指标：[`pdms_guide.md`](../pdms_guide.md)
- 各 checker 实现细节：[`hyw-grading/features.md`](../hyw-grading/features.md)
- 源码：`cpp/core/planner.cc` → `PdmsHackPlanner`
