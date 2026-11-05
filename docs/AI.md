# AI 与可重玩性

公开对手池有 24 个固定指挥官。18 个席位使用共用预算/生产规划，覆盖均衡、科技和协同三种规划风格；6 个突击席位保留原调度。难度/本地实力与玩家军衔独立，匹配按接近实力的候选加权随机选择。

## 经济与订单

`ai_economy_policy.h` 使用一个有目标、下一动作、价格、期限和取消原因的预算计划。生产只扣除实际预留，不让一个不可执行项目冻结全部资金；原命令拒绝、威胁、完成或期限到达会释放/重排计划。

`ai_tech_dependencies.h` 保留完整原配方身份、建筑前置与进行中任务去重。资源、价格、训练/升级耗时及命令结果仍来自原引擎。电力符号、地形、视野和出生区域按原数据处理，避免通过假造现金、全图情报或改价格解决规划问题。

`ai_base_policy.h`、`ai_water_plan.h`、`ai_coastal_recon_plan.h`、`ai_guard_policy.h` 分担基地恢复、水域连通、海岸位置与可见威胁防守。原引擎负责移动、寻路、射击、伤害及终局。

## 评测边界

当前版本验证了预算释放、现金对账、合法订单与多格式初始化等工程性质；没有证明统一高于原 AI 的长期胜率。更深科技链、全部地图/成长组合和团队标尺仍需要验证。

策略种子只控制本地策略的同分选择，原引擎随机尚未建立完整受控契约。相同本地种子不等于整局可重复。超时、开局错误、进程退出和正常终局分别统计，不能为了出结果伪造平局或胜者。

隔离 AI 对局入口见 `ai_replay_config.h`、`duel_lab.h`、`ai_lab.h`。新增批跑工具应明确设备、输入版本、席位/阵营/地图/成长、停止条件与逐局结果，并验证诊断对局没有写玩家生涯。

## 参考

以下项目用于理解预算与生产调度，本项目没有复制其实现：

- [0 A.D. Petra queue manager](https://github.com/0ad/0ad/blob/ed9b8fd73c293eb5c3ec7ac2c50df689b5cc6d85/binaries/data/mods/public/simulation/ai/petra/queueManager.js)：预算账户与可执行队列。
- [UAlbertaBot ProductionManager](https://github.com/davechurchill/ualbertabot/blob/b964678090bd41f337ce3c9de194050f6b6fc26c/UAlbertaBot/Source/ProductionManager.cpp)：生产调度和施工预留的职责。
- [Steamhammer development notes](https://www.satirist.org/ai/starcraft/blog/archives/256-Steamhammer-1.2.2-change-list.html)：无效施工预留和重新规划。
