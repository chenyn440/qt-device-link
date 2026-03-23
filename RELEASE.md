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

如果本机安装了 `Inno Setup`，会同时产出：

- `DeviceLink-windows-<version>.zip`
- `DeviceLink-windows-<version>-setup.exe`

如果没有安装 `Inno Setup`，仍会产出 zip，不会阻塞构建。

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

## GitHub Secrets

如果要启用 macOS 签名 / 公证，需要在 GitHub 仓库中配置：

- `APPLE_SIGN_IDENTITY`
- `APPLE_NOTARY_PROFILE`

当前工作流中：

- 未配置这些 Secrets 时，产出未签名 macOS 包
- 已配置时，`scripts/package-macos.sh` 会尝试签名和 notarization
