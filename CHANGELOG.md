# Changelog

## v0.1.4 - 2026-03-23

- 修复 Windows 安装器在 GitHub Actions 中对缺失中文语言文件的依赖问题
- 调整 Windows 打包脚本，确保安装器异常时不会阻断 `.zip` 正式包发布
- 补充并校正文档，明确 `.zip` 为强保证产物、`.exe` 为附加产物

## v0.1.3 - 2026-03-23

- 调整 Windows 打包策略，安装器打包失败时保留 `.zip` 包并继续发布流程
- 收敛 GitHub Release 失败面，优先保证主产物可用

## v0.1.2 - 2026-03-23

- 修复 `macdeployqt` / `windeployqt` 在 CI runner 中的查找逻辑
- 提升 Qt 工具路径兼容性，减少对固定安装目录的依赖

## v0.1.1 - 2026-03-23

- 修复发布脚本对 shell 可执行位和 `bash` 环境的隐式依赖
- 稳定 macOS / Windows 发布脚本在 GitHub Actions 中的执行行为

## v0.1.0 - 2026-03-23

- 首次补齐正式发布链路
- 增加应用版本信息注入
- 增加 macOS 打包脚本
- 增加 Windows 打包脚本和 Inno Setup 安装器脚本
- 增加 GitHub Actions `CI` / `Release` 工作流
- 补充本地打包与线上发布说明文档
