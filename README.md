# DeviceLink

基于 `Qt 6 + C++ + Qt Widgets + QSerialPort` 的串口上位机示例工程。

## 功能

- 串口参数配置与连接
- 接收原始 HEX 日志显示
- 按固定协议拆帧与校验
- 解析 `0x21` 状态回包
- 实时展示加热状态、延时关机状态、4 路设置温度与实际温度
- 发送控制命令：
  - 开启/关闭加热
  - 设置四路目标温度
  - 读取当前状态
  - 设置延时关机时间
  - 开启/关闭延时关机

## 协议

- 帧格式：`24 24 | len | cmd | data(len) | checksum | 26`
- 字节序：`little-endian`
- 校验：`data` 区逐字节求和，取低 8 位
- 状态回包：`cmd = 0x21`
- `0x21 data` 长度：按字段表实际总长为 `0x2E`
- 实际温度显示值：原始 `int32 / 10.0`

说明：你给的协议示例里 `0x21` 有一处长度写成了 `0x2C`，但按字��映射
`4 + 4 + 4 + 2 + 16 + 16 = 46 (0x2E)`。当前实现按字段表解析；如果你后面抓到真机日志证明长度字节确实发 `0x2C`，需要再回头修正设备协议说明或拆帧策略。

## 构建

确保本机已安装 Qt 6，并且 `Qt6_DIR` 或 CMake toolchain 已正确配置。

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/lib/cmake
cmake --build build
```

如果你的 Qt 安装路径已经在 CMake 可发现范围内，可以省略 `CMAKE_PREFIX_PATH`。

## 正式打包

项目已补齐本地打包脚本和 GitHub 自动发布工作流。

### 本地打包

macOS:

```bash
bash scripts/package-macos.sh
```

Windows:

```powershell
./scripts/package-windows.ps1
```

产物统一输出到 `dist/`：

- macOS: `DeviceLink-macOS-<version>.dmg`
- Windows: `DeviceLink-windows-<version>.zip`
- Windows 安装器: `DeviceLink-windows-<version>-setup.exe`

### GitHub 自动发布

仓库内已提供：

- `.github/workflows/ci.yml`
- `.github/workflows/release.yml`

工作流行为：

- 普通 `push` / `pull_request`：只做编译和测试
- 推送版本 tag：自动打正式包并创建 GitHub Release

版本 tag 规范：

```bash
git tag v1.0.0
git push origin v1.0.0
```

推送后会自动生成：

- macOS `.dmg`
- Windows `.zip`
- Windows 安装器 `.exe`

并作为 Release Assets 上传到对应 GitHub Release。

应用窗口标题、macOS Bundle 版本和打包文件名都统一跟随 `CMake project version` 与发布 tag。

### 可选签名 / 公证

`scripts/package-macos.sh` 已预留以下环境变量：

- `APPLE_SIGN_IDENTITY`
- `APPLE_NOTARY_PROFILE`

未提供时按未签名包构建；提供后会尝试执行签名和 notarization。

### 当前限制

- macOS 签名 / 公证依赖你在本机或 GitHub Secrets 里提供证书和 notary profile
- 如果后续需要 `.pkg`、`.exe` 安装器或自动更新，可在这套链路上继续扩展

## 测试

工程内置了协议测试目标 `DeviceLinkTests`，覆盖：

- 发包 HEX 是否与协议样例一致
- `0x21` 状态回包解析
- 半包处理
- 粘包处理
- 错误校验帧处理

构建后运行：

```bash
ctest --test-dir build --output-on-failure
```

如果你更习惯直接执行二进制，也可以运行 `build/DeviceLinkTests`。

## 示例报文

可直接参考 [examples/sample_report_0x21.txt](/Users/chenyongnuan/Desktop/百度/deviceLink/examples/sample_report_0x21.txt) 里的状态回包样例做联调。
