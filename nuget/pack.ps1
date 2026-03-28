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
    @{ Configuration = "Debug"; Platform = "Win32" },
    @{ Configuration = "Release"; Platform = "Win32" },
    @{ Configuration = "Debug"; Platform = "x64" },
    @{ Configuration = "Release"; Platform = "x64" }
)

foreach ($build in $buildMatrix) {
    & $msbuild (Join-Path $repoRoot "GuiTerminal.vcxproj") /t:Build /p:Configuration=$($build.Configuration) /p:Platform=$($build.Platform)
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
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "build\native\lib\Win32\Debug") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "build\native\lib\Win32\Release") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "build\native\lib\x64\Debug") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "build\native\lib\x64\Release") | Out-Null

Copy-Item (Join-Path $repoRoot "include\*.h") (Join-Path $packageRoot "build\native\include") -Force

Copy-Item (Join-Path $repoRoot "lib\x86\Debug\GuiTerminal.lib") (Join-Path $packageRoot "build\native\lib\Win32\Debug\GuiTerminal.lib") -Force
Copy-Item (Join-Path $repoRoot "lib\x86\Debug\GuiTerminal.pdb") (Join-Path $packageRoot "build\native\lib\Win32\Debug\GuiTerminal.pdb") -Force
Copy-Item (Join-Path $repoRoot "lib\x86\Release\GuiTerminal.lib") (Join-Path $packageRoot "build\native\lib\Win32\Release\GuiTerminal.lib") -Force
Copy-Item (Join-Path $repoRoot "lib\x86\Release\GuiTerminal.pdb") (Join-Path $packageRoot "build\native\lib\Win32\Release\GuiTerminal.pdb") -Force
Copy-Item (Join-Path $repoRoot "lib\x64\Debug\GuiTerminal.lib") (Join-Path $packageRoot "build\native\lib\x64\Debug\GuiTerminal.lib") -Force
Copy-Item (Join-Path $repoRoot "lib\x64\Debug\GuiTerminal.pdb") (Join-Path $packageRoot "build\native\lib\x64\Debug\GuiTerminal.pdb") -Force
Copy-Item (Join-Path $repoRoot "lib\x64\Release\GuiTerminal.lib") (Join-Path $packageRoot "build\native\lib\x64\Release\GuiTerminal.lib") -Force
Copy-Item (Join-Path $repoRoot "lib\x64\Release\GuiTerminal.pdb") (Join-Path $packageRoot "build\native\lib\x64\Release\GuiTerminal.pdb") -Force

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

