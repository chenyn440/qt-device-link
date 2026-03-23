$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = Join-Path $RootDir "build-release-windows"
$DistDir = Join-Path $RootDir "dist"
$AppName = "DeviceLink"
$InstallerScript = Join-Path $RootDir "scripts/windows-installer.iss"

function Get-Version {
    if ($env:GITHUB_REF_NAME) {
        return $env:GITHUB_REF_NAME -replace '^v', ''
    }

    $GitVersion = git describe --tags --exact-match 2>$null
    if ($LASTEXITCODE -eq 0 -and $GitVersion) {
        return $GitVersion -replace '^v', ''
    }

    return "0.1.0"
}

function Find-QtTool {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ToolName
    )

    $Command = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($Command) {
        return $Command.Source
    }

    if ($env:Qt6_DIR) {
        $SearchDir = $env:Qt6_DIR
        for ($i = 0; $i -lt 5 -and $SearchDir; $i++) {
            $Candidate = Join-Path (Split-Path -Parent $SearchDir) "bin\$ToolName.exe"
            if (Test-Path $Candidate) {
                return $Candidate
            }
            $ParentDir = Split-Path -Parent $SearchDir
            if ($ParentDir -eq $SearchDir) {
                break
            }
            $SearchDir = $ParentDir
        }
    }

    throw "missing Qt tool: $ToolName"
}

function Find-InnoSetup {
    $Command = Get-Command iscc -ErrorAction SilentlyContinue
    if ($Command) {
        return $Command.Source
    }

    $Candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    ) | Where-Object { $_ -and (Test-Path $_) }

    if ($Candidates.Count -gt 0) {
        return $Candidates[0]
    }

    return $null
}

$Version = Get-Version
$PackageDir = Join-Path $DistDir "$AppName-windows-$Version"
$ZipPath = "$PackageDir.zip"
$InstallerPath = Join-Path $DistDir "$AppName-windows-$Version-setup.exe"

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

$DeployTool = Find-QtTool -ToolName "windeployqt"
& $DeployTool --release --dir $PackageDir $ExePath

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}

Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipPath
Write-Host "created $ZipPath"

$Iscc = Find-InnoSetup
if ($Iscc) {
    try {
        $env:DEVICE_LINK_VERSION = $Version
        $env:DEVICE_LINK_PACKAGE_DIR = $PackageDir
        $env:DEVICE_LINK_DIST_DIR = $DistDir
        & $Iscc $InstallerScript
        if ($LASTEXITCODE -ne 0) {
            throw "Inno Setup exited with code $LASTEXITCODE"
        }
        if (Test-Path $InstallerPath) {
            Write-Host "created $InstallerPath"
        }
    }
    catch {
        Write-Warning "Windows installer packaging failed: $($_.Exception.Message)"
        Write-Warning "zip package was created successfully and release will continue."
        $global:LASTEXITCODE = 0
    }
}

exit 0
