param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [string]$PackageId = "GuiTerminal",

    [string]$Title = "GuiTerminal",

    [string]$Authors = "Mauro H. Leggieri",

    [string]$Description = "Native Win32/x64 terminal control library for Visual C++ applications.",

    [string]$RepositoryUrl = "https://github.com/mxmauro/GuiTerminal",

    [string]$OutputDirectory = ".\\nuget\\artifacts"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

function Find-VsWhere {
    $candidates = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Find-MSBuild {
    $vswhere = Find-VsWhere
    if ($vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if ($found) {
            return $found
        }
    }

    $fallback = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($fallback) {
        return $fallback.Source
    }

    throw "MSBuild.exe was not found. Install Visual Studio Build Tools or update nuget\\pack.ps1 with your MSBuild path."
}

$msbuild = Find-MSBuild
$packageOutputDirectory = Join-Path $repoRoot $OutputDirectory
$stagingRoot = Join-Path $repoRoot "nuget\artifacts\staging"
$packageRoot = Join-Path $stagingRoot "$PackageId.$Version"
$packageFile = Join-Path $packageOutputDirectory "$PackageId.$Version.nupkg"
$zipFile = Join-Path $packageOutputDirectory "$PackageId.$Version.zip"

New-Item -ItemType Directory -Force -Path $packageOutputDirectory | Out-Null

if (Test-Path $packageRoot) {
    Remove-Item -Recurse -Force $packageRoot
}

New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null

$buildMatrix = @(
    @{ Configuration = "DebugMT"; Platform = "Win32"; PackageConfiguration = "Debug"; RuntimeSubdir = "MT"; PlatformTarget = "x86" },
    @{ Configuration = "DebugMD"; Platform = "Win32"; PackageConfiguration = "Debug"; RuntimeSubdir = "MD"; PlatformTarget = "x86" },
    @{ Configuration = "ReleaseMT"; Platform = "Win32"; PackageConfiguration = "Release"; RuntimeSubdir = "MT"; PlatformTarget = "x86" },
    @{ Configuration = "ReleaseMD"; Platform = "Win32"; PackageConfiguration = "Release"; RuntimeSubdir = "MD"; PlatformTarget = "x86" },
    @{ Configuration = "DebugMT"; Platform = "x64"; PackageConfiguration = "Debug"; RuntimeSubdir = "MT"; PlatformTarget = "x64" },
    @{ Configuration = "DebugMD"; Platform = "x64"; PackageConfiguration = "Debug"; RuntimeSubdir = "MD"; PlatformTarget = "x64" },
    @{ Configuration = "ReleaseMT"; Platform = "x64"; PackageConfiguration = "Release"; RuntimeSubdir = "MT"; PlatformTarget = "x64" },
    @{ Configuration = "ReleaseMD"; Platform = "x64"; PackageConfiguration = "Release"; RuntimeSubdir = "MD"; PlatformTarget = "x64" }
)

foreach ($build in $buildMatrix) {
    & $msbuild (Join-Path $repoRoot "GuiTerminal.vcxproj") /t:Rebuild /p:Configuration=$($build.Configuration) /p:Platform=$($build.Platform)
    if ($LASTEXITCODE -ne 0) {
        throw "$($build.Platform) $($build.Configuration) build failed."
    }
}

$resolvedNuspec = (Get-Content (Join-Path $repoRoot "GuiTerminal.nuspec") -Raw).
    Replace('$id$', $PackageId).
    Replace('$version$', $Version).
    Replace('$title$', $Title).
    Replace('$authors$', $Authors).
    Replace('$description$', $Description).
    Replace('$repositoryUrl$', $RepositoryUrl)

New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "build\native\include") | Out-Null

Copy-Item (Join-Path $repoRoot "include\*.h") (Join-Path $packageRoot "build\native\include") -Force

foreach ($build in $buildMatrix) {
    $packagePlatform = if ($build.Platform -eq "Win32") { "Win32" } else { "x64" }
    $packageLibDirectory = Join-Path $packageRoot ("build\native\lib\{0}\{1}\{2}" -f $packagePlatform, $build.PackageConfiguration, $build.RuntimeSubdir)
    $builtLibDirectory = Join-Path $repoRoot ("lib\{0}\{1}\{2}" -f $build.PlatformTarget, $build.PackageConfiguration, $build.RuntimeSubdir)

    New-Item -ItemType Directory -Force -Path $packageLibDirectory | Out-Null

    Copy-Item (Join-Path $builtLibDirectory "GuiTerminal.lib") (Join-Path $packageLibDirectory "GuiTerminal.lib") -Force
    Copy-Item (Join-Path $builtLibDirectory "GuiTerminal.pdb") (Join-Path $packageLibDirectory "GuiTerminal.pdb") -Force
}

Copy-Item (Join-Path $repoRoot "nuget\build\native\GuiTerminal.props") (Join-Path $packageRoot "build\native\GuiTerminal.props") -Force

Copy-Item (Join-Path $repoRoot "README.md") (Join-Path $packageRoot "README.md") -Force
Copy-Item (Join-Path $repoRoot "LICENSE") (Join-Path $packageRoot "LICENSE") -Force

Set-Content -Path (Join-Path $packageRoot "$PackageId.nuspec") -Value $resolvedNuspec -Encoding UTF8

if (Test-Path $packageFile) {
    Remove-Item -Force $packageFile
}

if (Test-Path $zipFile) {
    Remove-Item -Force $zipFile
}

Compress-Archive -Path (Join-Path $packageRoot "*") -DestinationPath $zipFile
Move-Item -Force $zipFile $packageFile

