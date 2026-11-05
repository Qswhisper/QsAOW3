# 构建与测试

## 输入与工具链

公开构建以 `project.json` 指定的 v0.1.0 启动 APK 为不可变基础。它包含原 Unity/IL2CPP 引擎及 381 项离线输入；`data/assets-manifest.json` 逐文件记录大小和 SHA256。构建只重编译适配层并重新打包，不需要历史下载目录、官方账号、反编译全量 dump 或个人存档。

工具链：Python 3.11+、Java 17+、Android Build Tools 36.0.0、NDK 29.0.14206865。Android 安装最低 API 24，仅打包 ARM64。

```sh
sdkmanager "build-tools;36.0.0" "ndk;29.0.14206865" "platform-tools"
python tools/fetch_base.py
python tools/verify.py
python tools/build.py
```

Android SDK 路径通过 `ANDROID_HOME`、`ANDROID_SDK_ROOT` 或 `--sdk PATH` 设置；NDK 可通过 `ANDROID_NDK_HOME` 或 `--ndk PATH` 设置。Windows 的默认 Android SDK 目录也会被识别。工具通过参数数组启动进程，支持含空格的路径，不修改全局环境或代理。

已有启动包时：

```sh
python tools/build.py --base /path/to/QsAOW3-v0.1.0.apk --sdk /path/to/android-sdk
```

启动 APK 的 SHA256、IL2CPP 与原 Unity loader 哈希必须吻合。换一个原游戏版本需要重新核验全部原生接口与地址，不能仅修改版本号或跳过哈希检查。

`dist/QsAOW3.apk` 是输出，旁边的 `.build.json` 记录输入/原生库/产物哈希。签名和 16 KiB ZIP/ELF 对齐会在构建末尾检查。项目版本见 `project.json`，原客户端的 Android 版本字段仍沿原基线。

## 签名与存档

默认构建在 `.local/development.keystore` 创建开发密钥。同一工作副本重建会复用此密钥。它通常与 Release 的维护者签名不同，因此不能直接覆盖已安装的公开 Release。

维护者签名需要由仓库所有者保管，不能提交进 Git。已有应用更新需要保持 `com.art3rts00.aow` 和签名；改包名会使用另一个数据目录，改签名会被 Android 拒绝覆盖。卸载或清数据会删除进度。

```sh
# 在进程环境中设置密码，避免写进命令行或仓库文件
# QSAOW3_STORE_PASSWORD / QSAOW3_KEY_PASSWORD
python tools/build.py --keystore /private/release.keystore --key-alias art3offline
```

密码由进程环境传给 `keytool` / `apksigner`，不会出现在构建命令的公开参数中。维护者的 GitHub 加密 Secrets、手动发布模板和签名连续性见 [MAINTAINERS.md](MAINTAINERS.md)。

## 独立测试

Linux/macOS 可用 `clang++` 或 `g++` 直接运行；也可通过 `CXX` 指定编译器。

```sh
python tools/test.py
python tools/test.py --filter offline_account
```

Windows 可在 WSL/Linux 下运行，或用 NDK 将测试编译到明确指定的 Android 设备：

```sh
adb devices
python tools/test.py --android --serial DEVICE_SERIAL
# 自定义 ADB server：再加 --adb-port PORT
```

测试只使用新建的临时目录。Android 测试位于 `/data/local/tmp`，不安装游戏、不加载玩家档；目录在测试完成/失败后清理。供应、库存和改装的依赖样本由测试程序现场生成，图谱与研究定义样本在 `tests/fixtures`。战役目录的损坏检查使用合成二进制，不将其用作游戏场景。

## 原引擎验证

独立规则测试无法替代 Unity 场景/脚本验收。改动原生接口、场景切换、终局、结算事务或存档版本后，使用独立 Android 测试用户/测试设备完成相关路径，保留当前玩家进度。不要用 `pm clear`、旧档覆盖或修改胜负标记“修复”玩家档。

典型路径：干净数据断网首启 → 原教程；已有安装覆盖 → 当前生涯；冷启动 → 当前进度；一次受影响模式的真实终局 → 结算 → 再次冷启。其余无关玩法按变更范围复用已有证据。

## 修改内容

```sh
python tools/content.py stage
# 修改 .local/assets 中的原输入以及 content/ 中的路由
python tools/content.py rebuild
python tools/build.py --assets .local/assets
```

内容目录与采集输入的约束见 [CONTENT.md](CONTENT.md)。默认 `verify.py` 对照当前提交的 `data/assets-manifest.json` 检查输入，内容变更时应同步更新该校验清单。
