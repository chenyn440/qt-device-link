# Release Guide

## 本地打包和线上打包对应关系

| 场景 | 平台 | 入口 | 实际执行 |
|---|---|---|---|
| 本地打包 | macOS | `bash scripts/release-local.sh macos` | `scripts/package-macos.sh` |
| 本地打包 | Windows | `scripts\release-local.sh windows` 或直接 PowerShell | `scripts/package-windows.ps1` |
| GitHub Tag 发布 | macOS | `.github/workflows/release.yml` | `scripts/package-macos.sh` |
| GitHub Tag 发布 | Windows | `.github/workflows/release.yml` | `scripts/package-windows.ps1` |

## 本地发布步骤

### macOS

```bash
bash scripts/release-local.sh macos
```

如果需要签名 / 公证：

```bash
export APPLE_SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)"
export APPLE_NOTARY_PROFILE="notary-profile-name"
bash scripts/release-local.sh macos
```

### Windows

在 Windows PowerShell 中执行：

```powershell
./scripts/package-windows.ps1
```

固定产物：

- `DeviceLink-windows-<version>.zip`

如果本机安装了 `Inno Setup`，还会额外产出：

- `DeviceLink-windows-<version>-setup.exe`

如果没有安装 `Inno Setup`，仍会产出 zip，不会阻塞构建。
如果安装器编译失败，脚本会保留 zip 并继续返回成功，避免阻断正式发布。

## GitHub 线上发布步骤

1. 提交代码并 push 到 GitHub
2. 创建并推送版本 tag

```bash
git tag v1.0.0
git push origin v1.0.0
```

3. GitHub Actions 自动执行：
   - macOS 打包
   - Windows 打包
   - 创建 GitHub Release
   - 上传 Release Assets

说明：

- GitHub Release 以 macOS `.dmg` 和 Windows `.zip` 为正式主产物
- Windows 安装器 `.exe` 属于附加产物，生成成功时会一起上传

## GitHub Secrets

如果要启用 macOS 签名 / 公证，需要在 GitHub 仓库中配置：

- `APPLE_SIGN_IDENTITY`
- `APPLE_NOTARY_PROFILE`

当前工作流中：

- 未配置这些 Secrets 时，产出未签名 macOS 包
- 已配置时，`scripts/package-macos.sh` 会尝试签名和 notarization

## 当前已验证状态

- `main` 已支持本地 macOS 打包
- `main` 已支持本地 Windows zip 打包
- GitHub tag 发布链路已验证打通
- Windows 安装器当前默认使用英文 Inno Setup 向导
