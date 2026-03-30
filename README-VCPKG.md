# Vcpkg Registry Maintenance

This document describes how to test and publish `guiterminal` into the public registry at `https://github.com/mxmauro/vcpkg-registry`.

## Prerequisites

- A local clone of this repository.
- A local clone of the registry repository at `C:\Fuentes\VSNet\VCPkgRegistry`.
- Git installed and authenticated for the repositories you will push.
- For automated publishing from GitHub Actions, a secret named `VCPKG_REGISTRY_PAT` with write access to `mxmauro/vcpkg-registry`.
  - Minimum for the current workflow: repository `Contents: Read and write`.
  - If you later switch the workflow to open PRs instead of pushing directly, also grant `Pull requests: Read and write`.

## Dry-run test flow

Use this flow to test the script without touching the real registry clone.

1. Clone the registry into a temporary folder.

```powershell
$tempRegistry = Join-Path $env:TEMP ("vcpkgregistry-test-" + [guid]::NewGuid().ToString("N"))
git clone C:\Fuentes\VSNet\VCPkgRegistry $tempRegistry
```

2. Generate registry files against the temporary clone from local source.

```powershell
.\scripts\build-vcpkg-registry.ps1 `
  -RegistryPath $tempRegistry `
  -RegistryKind Git `
  -SourceMode LocalSource `
  -Version 1.0.0 `
  -SourcePath C:\Fuentes\VSNet\Libraries\GuiTerminal
```

3. Inspect the generated files.

```powershell
git -C $tempRegistry status
Get-Content (Join-Path $tempRegistry "versions\baseline.json")
Get-Content (Join-Path $tempRegistry "versions\g-\guiterminal.json")
```

4. Remove the temporary clone when finished.

```powershell
Remove-Item -Recurse -Force $tempRegistry
```

## Manual release flow

1. Push the source repository and tag the release.

```powershell
git push origin main
git tag v1.0.0
git push origin v1.0.0
```

2. Generate registry metadata in the real registry clone using the pushed tag.

```powershell
.\scripts\build-vcpkg-registry.ps1 `
  -RegistryPath C:\Fuentes\VSNet\VCPkgRegistry `
  -RegistryKind Git `
  -SourceMode GitHub `
  -Version 1.0.0 `
  -SourceRef v1.0.0 `
  -GitHubRepository mxmauro/GuiTerminal
```

3. Review the registry changes.

```powershell
git -C C:\Fuentes\VSNet\VCPkgRegistry status
git -C C:\Fuentes\VSNet\VCPkgRegistry diff
```

4. Commit and push the registry update.

```powershell
git -C C:\Fuentes\VSNet\VCPkgRegistry add ports versions
git -C C:\Fuentes\VSNet\VCPkgRegistry commit -m "Add guiterminal 1.0.0"
git -C C:\Fuentes\VSNet\VCPkgRegistry push origin main
```

## Tag notes

GitHub Actions tag filters use glob patterns, not full regex, so the workflow triggers on tags beginning with `v` and then validates the tag in PowerShell with:

```regex
^v\d+\.\d+\.\d+(-[a-zA-Z0-9][a-zA-Z0-9.\-]*)?$
```

Accepted examples:

- `v1.2.3`
- `v1.2.3-rc1`
- `v1.2.3-beta.2`

The workflow strips the leading `v` and passes the remainder as the vcpkg `version-string`.
