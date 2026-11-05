# 架构与维护入口

## 启动与原生边界

Android 原 Activity 和 Unity NativeLoader 继续加载原引擎。适配库替换 `libmain.so`，原 loader 保留为 `libart3originalmain.so`。`JNI_OnLoad` 先调用原 loader，随后等待 `libil2cpp.so` 并绑定 IL2CPP API。

`native/offline_bootstrap.cpp` 是整合入口。它核验原 ARM64 方法前导指令，安装 trampoline/hook，安装包内输入，再在原登录状态机的 `CheckFirstRun` / 状态 6 接管离线初始化。`ClientMainContext`、原配置接收器、场景过渡和对战逻辑继续运行。

基线已经包含三个静态修补：原首次联网游客路径关闭、Google Play 可用性关闭，以及空阵营首装从状态 5 进入状态 6。最后一项避免首装在离线接管前先进入联网阵营选择。离线商业插件由 `ART3_MAIN` 编译边界禁用。实际 Manifest 没有 INTERNET 权限。

原方法地址只适用于 `project.json` 中固定的 IL2CPP 二进制。`offline_entry_hooks.generated.h` 记录界面 hook 的原前导指令；其他 hook 在相应 runtime 头文件中定义。现有 trampoline 只处理已实现的 PC 相对指令，修改 hook 前必须重新检查前导指令及 ARM64 ABI。

## 文件与职责

| 入口 | 职责 |
| --- | --- |
| `offline_entry_runtime.h`、`new_start_policy.h` | 原界面的生涯切换、新开局、教程偏好隔离、停用入口 |
| `offline_account_store.h`、`faction_store.h`、`faction_runtime.h` | 生涯选择器、双阵营目录和原 profile 的本地投影 |
| `local_progress.h`、`career_runtime.h`、`research_runtime.h` | 钱包、成长收据、研究和原配置应用 |
| `campaign_catalog.h`、`campaign_runtime.h`、`campaign_result_runtime.h` | 内容寻址、场景开局、真实原终局和最佳奖励 |
| `local_match_runtime.h`、`local_training_runtime.h`、`cooperative_runtime.h` | AI 席位、原格式/训练大厅/合作流程 |
| `ai_lab.h`、`ai_economy_policy.h`、`ai_tech_dependencies.h` | 原引擎 AI 调度、预算、可执行前置与订单 |
| `local_match_store.h`、`cooperative_store.h`、`local_history_*` | 对局日志、冻结资格、一次结算、原卡片履历 |
| `*_content.generated.h`、`*_contracts.h` | 已接入的原身份、定义和经过核对的效果映射 |
| `*_audit.h`、`interaction_trace.h`、`duel_lab.h` | 显式诊断/隔离对局；普通玩家路径不启用这些标记 |

多数 runtime 头文件共享引擎绑定和全局状态，由 bootstrap 按顺序包含；不能把它们当作相互独立的 C++ 翻译单元。`*_policy.h`、`*_store.h` 与 `*_test.cpp` 中可独立部分用于原生测试。

## 输入安装与资源

`assets/offline/install.list` 列出 381 项基线输入。首次启动由应用自身写入本包外部 files 目录，创建所需目录和离线启用标记；无需 ADB 预置。字典/协议对象通过原序列化器恢复，资源目录通过原 Addressables loader 接入。

`offline-campaign/catalog.bin`、`offline-cooperative/catalog.bin` 使用有界表、校验和及内容地址引用。真正的地图、原脚本和任务目标仍来自原输入，目录项本身不生成新场景。

## 结算与故障处理

场景目标和胜负由原引擎判定。正式/合作开局时冻结奖励资格；正常终局先写独立对局记录，再以 run/收据状态对账成长，冷启动可恢复未完成的钱包事务。训练、诊断、投降、中断和异常不被改写成正式正常胜利。

钱包变更先在候选状态中验证所有奖励/扣费/上限，再一次保存。未知奖励或混合效果应整笔拒绝，不能先收费、部分发奖或吞异常后显示成功。存档和历史 ABI 见 [SAVES.md](SAVES.md)。

## 调试

原生日志 tag 为 `Art3Offline`；设备 files 下的 `art3offline-main.txt` 记录离线初始化，其他诊断文件按对应模块命名。仅在隔离设备/生涯中启用诊断标记。可以用 `tools/inspect_binary.py` 对固定 IL2CPP 版本的指定 RVA 做有界反汇编。
