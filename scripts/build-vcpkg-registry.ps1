param(
    [Parameter(Mandatory = $true)]
    [string]$RegistryPath,

    [ValidateSet("Git", "Filesystem")]
    [string]$RegistryKind = "Filesystem",

    [ValidateSet("LocalSource", "GitHub")]
    [string]$SourceMode = "LocalSource",

    [string]$Version,

    [string]$SourcePath,

    [string]$GitHubRepository,

    [string]$SourceRef,

    [string]$SourceArchiveSha512,

    [string]$DefaultBranch = "master",

    [switch]$InitializeGit
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

# Project-specific metadata lives here.
$ProjectName = "GuiTerminal"
$PortName = "guiterminal"
$GitHubRepositoryDefault = "mxmauro/GuiTerminal"
$PackageDescription = "Native C++ terminal control library for Win32 desktop applications rendered with Direct2D and DirectWrite."
$PackageHomepage = "https://github.com/mxmauro/GuiTerminal"
$PackageLicense = "MIT"
$PackageSupports = "windows & !uwp"
$CMakeConfigPath = "lib/cmake/GuiTerminal"
$CMakeDemoOption = "GUITERMINAL_BUILD_DEMO"
$CMakeSharedOption = "GUITERMINAL_BUILD_SHARED"
$CMakeSharedOptionReference = ('${' + $CMakeSharedOption + '}')
$ExportedTargetName = "GuiTerminal::GuiTerminal"
$SharedHeaderName = "GuiTerminalC.h"

function Get-ProjectVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot
    )

    $cmakeLists = Get-Content (Join-Path $RepoRoot "CMakeLists.txt") -Raw
    $match = [regex]::Match($cmakeLists, ("project\s*\(\s*{0}\s+VERSION\s+([0-9A-Za-z.+-]+)" -f [regex]::Escape($ProjectName)))
    if (-not $match.Success) {
        throw "Unable to determine the project version from CMakeLists.txt."
    }

    return $match.Groups[1].Value
}

function Normalize-PathForCMake {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    return ((Resolve-Path $Path).Path -replace '\\', '/')
}

function Find-VsWhere {
    # Resolve vswhere from the standard Visual Studio installer location so we
    # can discover the bundled vcpkg instance on developer machines and runners.
    $programFiles = if (${env:ProgramFiles(x86)}) { ${env:ProgramFiles(x86)} } else { "C:\Program Files (x86)" }
    $candidate = Join-Path $programFiles "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $candidate) {
        return $candidate
    }

    throw "vswhere.exe was not found."
}

function Find-Vcpkg {
    # Prefer the vcpkg instance shipped with the installed Visual Studio
    # toolchain. Fall back to PATH only if that discovery path is unavailable.
    $vswhere = Find-VsWhere
    $installations = & $vswhere -products * -requires Microsoft.Component.MSBuild -format json | ConvertFrom-Json

    foreach ($installation in $installations) {
        $candidate = Join-Path $installation.installationPath "VC\vcpkg\vcpkg.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    $fallback = Get-Command vcpkg.exe -ErrorAction SilentlyContinue
    if ($fallback) {
        return $fallback.Source
    }

    throw "vcpkg.exe was not found."
}

function Get-GitTreeForPort {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RegistryPath,

        [Parameter(Mandatory = $true)]
        [string]$PortName
    )

    # Git registries store a git-tree hash for each published port version.
    # Compute that hash from the generated ports/<port> directory in isolation
    # so the value matches what the registry will contain after commit.
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("{0}-registry-tree-{1}" -f $PortName, [guid]::NewGuid().ToString("N"))
    $tempPortRoot = Join-Path $tempRoot ("ports\{0}" -f $PortName)

    try {
        New-Item -ItemType Directory -Force -Path $tempPortRoot | Out-Null
        Copy-Item (Join-Path $RegistryPath ("ports\{0}\*" -f $PortName)) $tempPortRoot -Recurse -Force

        & git -C $tempRoot init | Out-Null
        & git -C $tempRoot config user.email "codex@example.invalid" | Out-Null
        & git -C $tempRoot config user.name "Codex" | Out-Null
        & git -C $tempRoot add --all | Out-Null
        & git -C $tempRoot commit -m "Compute port tree" | Out-Null

        $gitTree = (& git -C $tempRoot rev-parse ("HEAD:ports/{0}" -f $PortName)).Trim()
        if (-not $gitTree) {
            throw "Unable to compute git-tree for ports/$PortName."
        }

        return $gitTree
    } finally {
        if (Test-Path $tempRoot) {
            Remove-Item -Recurse -Force $tempRoot
        }
    }
}

function Update-GitVersionsFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string]$Version,

        [Parameter(Mandatory = $true)]
        [string]$GitTree
    )

    # Keep older published versions for the same port, but replace the entry for
    # the current version so regenerated metadata stays deterministic.
    $versions = @()
    if (Test-Path $FilePath) {
        $existing = Get-Content $FilePath -Raw | ConvertFrom-Json
        if ($existing.versions) {
            foreach ($entry in $existing.versions) {
                if ($entry.'version-string' -ne $Version) {
                    $versions += @{
                        "version-string" = $entry.'version-string'
                        "port-version" = [int]$entry.'port-version'
                        "git-tree" = $entry.'git-tree'
                    }
                }
            }
        }
    }

    $versions = @(
        @{
            "version-string" = $Version
            "port-version" = 0
            "git-tree" = $GitTree
        }
    ) + $versions

    @{ versions = $versions } | ConvertTo-Json -Depth 10 | Set-Content -Path $FilePath -Encoding UTF8
}

function Update-FilesystemVersionsFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string]$Version,

        [Parameter(Mandatory = $true)]
        [string]$PortName
    )

    # Filesystem registries point directly at a versioned folder instead of a
    # git-tree hash, but they still need the same per-version history file.
    $versions = @()
    if (Test-Path $FilePath) {
        $existing = Get-Content $FilePath -Raw | ConvertFrom-Json
        if ($existing.versions) {
            foreach ($entry in $existing.versions) {
                if ($entry.'version-string' -ne $Version) {
                    $versions += @{
                        "version-string" = $entry.'version-string'
                        "port-version" = [int]$entry.'port-version'
                        path = $entry.path
                    }
                }
            }
        }
    }

    $versions = @(
        @{
            "version-string" = $Version
            "port-version" = 0
            path = ("$/ports/{0}/{1}" -f $PortName, $Version)
        }
    ) + $versions

    @{ versions = $versions } | ConvertTo-Json -Depth 10 | Set-Content -Path $FilePath -Encoding UTF8
}

function Update-BaselineFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string]$PortName,

        [Parameter(Mandatory = $true)]
        [string]$Version
    )

    # Preserve existing baseline entries for other ports and update only the
    # selected port. This keeps the script safe once the registry hosts more
    # than one package.
    $defaultEntries = [ordered]@{}

    if (Test-Path $FilePath) {
        $existing = Get-Content $FilePath -Raw | ConvertFrom-Json
        if ($existing.default) {
            foreach ($property in $existing.default.PSObject.Properties) {
                if ($property.Name -ne $PortName) {
                    $defaultEntries[$property.Name] = @{
                        baseline = $property.Value.baseline
                        "port-version" = [int]$property.Value.'port-version'
                    }
                }
            }
        }
    }

    $defaultEntries[$PortName] = @{
        baseline = $Version
        "port-version" = 0
    }

    @{
        default = $defaultEntries
    } | ConvertTo-Json -Depth 10 | Set-Content -Path $FilePath -Encoding UTF8
}

if (-not $Version) {
    $Version = Get-ProjectVersion -RepoRoot $repoRoot
}

if (-not $SourcePath) {
    $SourcePath = $repoRoot
}

if ([string]::IsNullOrWhiteSpace($GitHubRepository)) {
    $GitHubRepository = $GitHubRepositoryDefault
}

if ($SourceMode -eq "GitHub" -and [string]::IsNullOrWhiteSpace($GitHubRepository)) {
    throw "-GitHubRepository is required when -SourceMode GitHub is used."
}

if (-not (Test-Path $RegistryPath)) {
    New-Item -ItemType Directory -Force -Path $RegistryPath | Out-Null
}

if ($InitializeGit -and -not (Test-Path (Join-Path $RegistryPath ".git"))) {
    & git -C $RegistryPath init | Out-Null
}

$portRoot = if ($RegistryKind -eq "Git") {
    Join-Path $RegistryPath ("ports\{0}" -f $PortName)
} else {
    Join-Path $RegistryPath ("ports\{0}\{1}" -f $PortName, $Version)
}
$versionsRoot = Join-Path $RegistryPath "versions"
$versionPrefixRoot = Join-Path $versionsRoot ("{0}-" -f $PortName.Substring(0, 1))

# Generate the port files in the layout expected by either:
# - a git registry: ports/<port>
# - a filesystem registry: ports/<port>/<version>
New-Item -ItemType Directory -Force -Path $portRoot | Out-Null
New-Item -ItemType Directory -Force -Path $versionPrefixRoot | Out-Null

$portManifest = @{
    name = $PortName
    "version-string" = $Version
    description = $PackageDescription
    homepage = $PackageHomepage
    license = $PackageLicense
    supports = $PackageSupports
    dependencies = @(
        @{ name = "vcpkg-cmake"; host = $true },
        @{ name = "vcpkg-cmake-config"; host = $true }
    )
}
$portManifest | ConvertTo-Json -Depth 10 | Set-Content -Path (Join-Path $portRoot "vcpkg.json") -Encoding UTF8

if ($SourceMode -eq "GitHub") {
    if (-not $SourceRef) {
        throw "-SourceRef is required when -SourceMode GitHub is used."
    }

    if (-not $SourceArchiveSha512) {
        # Download the exact source archive that the registry port will fetch
        # and hash it now so the generated portfile is self-contained.
        $archiveUrl = "https://github.com/{0}/archive/{1}.tar.gz" -f $GitHubRepository, $SourceRef
        $archivePath = Join-Path ([System.IO.Path]::GetTempPath()) ("{0}-{1}.tar.gz" -f $PortName, [guid]::NewGuid().ToString("N"))
        try {
            Invoke-WebRequest -Uri $archiveUrl -OutFile $archivePath
            $SourceArchiveSha512 = (Get-FileHash -Path $archivePath -Algorithm SHA512).Hash.ToLowerInvariant()
        } finally {
            if (Test-Path $archivePath) {
                Remove-Item -Force $archivePath
            }
        }
    }

    $portfile = @"
if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    set($CMakeSharedOption ON)
else()
    set($CMakeSharedOption OFF)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO $GitHubRepository
    REF $SourceRef
    SHA512 $SourceArchiveSha512
    HEAD_REF $DefaultBranch
)

vcpkg_cmake_configure(
    SOURCE_PATH "`${SOURCE_PATH}"
    OPTIONS
        -DBUILD_SHARED_LIBS=$CMakeSharedOptionReference
        -D$CMakeDemoOption=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH $CMakeConfigPath)

file(REMOVE_RECURSE "`${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "`${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "`${CURRENT_PACKAGES_DIR}/share/`${PORT}")

vcpkg_install_copyright(FILE_LIST "`${SOURCE_PATH}/LICENSE")
"@
} else {
    # For local validation, point the generated port directly at the current
    # source tree instead of fetching from GitHub.
    $normalizedSourcePath = Normalize-PathForCMake -Path $SourcePath
    $portfile = @"
if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    set($CMakeSharedOption ON)
else()
    set($CMakeSharedOption OFF)
endif()

set(SOURCE_PATH "$normalizedSourcePath")

vcpkg_cmake_configure(
    SOURCE_PATH "`${SOURCE_PATH}"
    OPTIONS
        -DBUILD_SHARED_LIBS=$CMakeSharedOptionReference
        -D$CMakeDemoOption=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH $CMakeConfigPath)

file(REMOVE_RECURSE "`${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "`${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "`${CURRENT_PACKAGES_DIR}/share/`${PORT}")

vcpkg_install_copyright(FILE_LIST "`${SOURCE_PATH}/LICENSE")
"@
}

Set-Content -Path (Join-Path $portRoot "portfile.cmake") -Value $portfile -Encoding UTF8

$usage = @"
The package exports the CMake target:

    find_package($ProjectName CONFIG REQUIRED)
    target_link_libraries(your_target PRIVATE $ExportedTargetName)

The library links Direct2D and DirectWrite as part of the exported target.

Triplets control both library linkage and CRT linkage.

Examples:

    x86-windows             -> DLL + dynamic CRT
    x64-windows             -> DLL + dynamic CRT
    x86-windows-static      -> static library + static CRT
    x64-windows-static      -> static library + static CRT
    x86-windows-static-md   -> static library + dynamic CRT
    x64-windows-static-md   -> static library + dynamic CRT

If you need DLL + static CRT, use a custom triplet with:

    set(VCPKG_LIBRARY_LINKAGE dynamic)
    set(VCPKG_CRT_LINKAGE static)

Static-library triplets install both the C++ and C headers.
Shared-library triplets install the C wrapper header only:

    #include <$SharedHeaderName>
"@
Set-Content -Path (Join-Path $portRoot "usage") -Value $usage -Encoding UTF8

$baselinePath = Join-Path $versionsRoot "baseline.json"
$versionsPath = Join-Path $versionPrefixRoot ("{0}.json" -f $PortName)

# The baseline selects the default version of each port for consumers of this
# registry, while versions/<prefix>/<port>.json stores the full history for the
# specific port.
Update-BaselineFile -FilePath $baselinePath -PortName $PortName -Version $Version

if ($RegistryKind -eq "Git") {
    if (-not (Test-Path (Join-Path $RegistryPath ".git"))) {
        throw "The registry path must be a git repository before Git-registry metadata can be generated. Re-run with -InitializeGit or use an existing clone."
    }

    $gitTree = Get-GitTreeForPort -RegistryPath $RegistryPath -PortName $PortName
    Update-GitVersionsFile -FilePath $versionsPath -Version $Version -GitTree $gitTree
} else {
    Update-FilesystemVersionsFile -FilePath $versionsPath -Version $Version -PortName $PortName
}

$vcpkgPath = Find-Vcpkg

Write-Output ("Registry files generated in: {0}" -f (Resolve-Path $RegistryPath).Path)
Write-Output ("vcpkg executable: {0}" -f $vcpkgPath)
Write-Output ("port name: {0}" -f $PortName)
Write-Output ("version: {0}" -f $Version)
Write-Output ("registry kind: {0}" -f $RegistryKind)
if ($RegistryKind -eq "Git") {
    Write-Output ("git-tree: {0}" -f $gitTree)
}
if ($SourceMode -eq "GitHub") {
    Write-Output ("source ref: {0}" -f $SourceRef)
    Write-Output ("source sha512: {0}" -f $SourceArchiveSha512)
} else {
    Write-Output ("source path: {0}" -f (Resolve-Path $SourcePath).Path)
}
