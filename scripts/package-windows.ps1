$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = Join-Path $RootDir "build-release-windows"
$DistDir = Join-Path $RootDir "dist"
$Version = bash (Join-Path $RootDir "scripts/version.sh")
$AppName = "DeviceLink"
$PackageDir = Join-Path $DistDir "$AppName-windows-$Version"
$ZipPath = "$PackageDir.zip"

if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

cmake -S $RootDir -B $BuildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $BuildDir --config Release
ctest --test-dir $BuildDir --output-on-failure -C Release

$ExePath = Join-Path $BuildDir "Release\$AppName.exe"
if (!(Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildDir "$AppName.exe"
}
if (!(Test-Path $ExePath)) {
    throw "missing executable: $ExePath"
}

if (Test-Path $PackageDir) {
    Remove-Item -Recurse -Force $PackageDir
}

New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null
Copy-Item $ExePath $PackageDir

$DeployTool = Get-Command windeployqt -ErrorAction Stop
& $DeployTool.Source --release --dir $PackageDir $ExePath

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}

Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipPath
Write-Host "created $ZipPath"
