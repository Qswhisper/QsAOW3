# 发布与签名连续性

公开 Release 的维护者签名与普通 clone 默认生成的开发签名不同。维护者需要保持签名证书连续，才能让已有安装覆盖更新并保留 Android 应用数据。

维护者密钥不得放进 Git、Release 或构建日志。本仓库使用 GitHub Actions 的加密 Secrets 保管发布密钥，以下名字与 `ci/release.yml` 模板对应：

| Secret | 内容 |
| --- | --- |
| `QSAOW3_KEYSTORE_B64` | 发布 keystore 的 Base64 内容 |
| `QSAOW3_STORE_PASSWORD` | keystore 密码 |
| `QSAOW3_KEY_PASSWORD` | 私钥密码 |
| `QSAOW3_KEY_ALIAS` | 发布别名 |

GitHub 不通过读取 API 返回 Secret 的值；维护者可在受信任的构建 workflow 中使用它们。Secrets 的公开列表只包含名称和更新时间。当前发布证书 SHA256 记录在 `project.json` 中，可与 APK 签名核对。

`ci/release.yml` 是可审查的手动发布模板。仓库所有者启用时，将它复制为 `.github/workflows/release.yml`，并检查权限、依赖和主分支内容。部分 OAuth Git 凭据需要额外的 workflow 权限才能提交该目录；模板本身不依赖当前电脑。

只在可信主分支的手动发布中使用密钥。外来 Pull Request 的代码不应带发布 Secrets 执行。普通贡献者可以用自己的开发签名验证功能，并将源码修改提交给维护者。

初始版本的预编译 APK 同时作为固定构建输入，文件名和 SHA256 见 `project.json`。保留此 Release，使后来者能从新电脑重建。新的发布标签、说明、APK 和校验和应指向同一份源码；发布后下载远端附件并核验 SHA256。

若更换仓库所有者，应移交仓库权限和签名使用能力。若只能换签名，先提供面向玩家的进度迁移方案，不通过要求玩家直接卸载来隐去不兼容。
