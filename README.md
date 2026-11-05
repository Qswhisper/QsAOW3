# QsAOW3

[English](README.en.md)

Art of War 3 的非官方 Android 离线单机适配。保留原 Unity/IL2CPP 引擎、操作和界面，用本地生涯、结算和 AI 席位承接已接入的玩法。

**[下载安装包](https://github.com/Qswhisper/QsAOW3/releases/latest)** · [构建说明](docs/BUILD.md) · [架构](docs/ARCHITECTURE.md) · [开发路线](ROADMAP.md)

公开源码是 C++ 离线适配层、规则、存档系统、测试和构建工具。引擎与原游戏内容由固定版本的启动 APK 提供；其权利归原权利人，见 [第三方说明](THIRD_PARTY_NOTICES.md)。

## 游玩

- Android 7.0 / API 24 及以上，ARM64。安装 Release 中的 APK 即可，不需要联网或另拷缓存。
- 首次安装进入原开篇教程。新生涯从 1000 信用点、20 金币、0 经验、0 星和零已购研究开始。
- 原设置中的“新开局（保留旧进度）”可创建独立生涯；本地存档按钮可切换已有生涯。
- 同包名、同签名的版本可以覆盖安装。卸载/清数据会删除本地进度；使用自行生成签名的开发包时，请先阅读[签名与存档](docs/BUILD.md#签名与存档)。

| 内容 | 当前可玩范围 | 边界 |
| --- | --- | --- |
| 战役 | 联盟 01–21、反抗 01–34，共 55 个普通章节 | 后续 45 个普通章节、完整困难输入尚缺 |
| 训练 | 九种原战斗格式、地空海 AI，可提前进入 | 不发正式奖励、不改变正式实力 |
| 正式对战 | 原 1v1、2v2、3v3，其他席位由 AI 填充 | 保留原军衔门槛；没有真人联网匹配 |
| 合作 | 已接入的反抗军南瓜合作关卡、AI 队友和结算 | 其他独立合作/赛事输入不齐 |
| 成长 | 科研、英雄、技能、库存、部分永久改装及箱子/合同 | 仍有未知前置、混合效果和缺失原奖励池 |
| 生涯 | 独立本地进度、真实战斗结算、履历与档案 | 各生涯资源独立；没有在线账号同步 |

正式 1v1 依军衔 3 / 18 星开放，合作依军衔 7 / 110 星开放，团队沿原卡面门槛。纯联网、充值、广告、签到等未接入入口已停用。[玩法规则](docs/GAMEPLAY.md)列出单机适配与原机制的边界。

## 构建

需要 Python 3.11+、Java 17+、Android SDK Build Tools 36.0.0 和 NDK 29.0.14206865。工具提供 Windows、Linux、macOS 主机路径，当前实际构建验证在 Windows 完成。

```sh
git clone https://github.com/Qswhisper/QsAOW3.git
cd QsAOW3
python tools/fetch_base.py
python tools/verify.py
python tools/build.py
```

输出为 `dist/QsAOW3.apk`。默认生成只属于当前工作副本的开发签名。已有启动包可以通过 `--base PATH` 使用，全程校验 SHA256；不需要原作者电脑、旧报告、模拟器数据或历史目录。

```sh
# Linux/macOS：独立规则和存档测试
python tools/test.py

# Windows 或指定 Android 设备
python tools/test.py --android --serial DEVICE_SERIAL
```

[完整构建说明](docs/BUILD.md)包含 SDK 设置、签名、内容修改和设备测试方法。

## 接手开发

- `native/`：入口接管、AI、成长、结算和存档；独立测试与实现放在一起。
- `content/`：战役/合作路由，使用内容哈希定位原输入。
- `data/`：启动输入的校验清单与原任务身份元数据。
- `tests/fixtures/`：规则定义样本与测试替身，不含玩家存档。
- `tools/`：独立构建、内容处理、检查和测试工具。
- `docs/`：当前机制与接口说明，不含聊天交接记录或旧封版脚本。

从 [架构](docs/ARCHITECTURE.md)、[内容扩展](docs/CONTENT.md)、[AI](docs/AI.md)和 [贡献指南](CONTRIBUTING.md)开始。当前验证范围见 [VALIDATION.md](docs/VALIDATION.md)。

## 许可

新增适配代码与工具采用 [MIT License](LICENSE)。原游戏引擎、素材、地图、剧情、原始配置与商标保留其原有权利，MIT 不对这些内容授予许可。本项目与原游戏开发商无隶属关系；详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
