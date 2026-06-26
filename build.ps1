#!/usr/bin/env pwsh
# NOTE: file is slop-ported from the .sh. Consider the .sh the source of truth.
# AI nonsense follows:
#
# Windows-native port of build.sh. Builds 32- and 64-bit DLLs using llvm-mingw
# and produces a zipped dist/.
#
# Prerequisites: meson, ninja, and llvm-mingw on PATH (or at $ToolchainBin
# below). No wine needed — tests that require it should be run separately.

# mon note: I use a Dev Drive for speed, you probably want to change these
$Build32 = 'X:\layeredfs_build32'
$Build64 = 'X:\layeredfs_build64'
$DistDir = 'X:\layeredfs_dist'

$ErrorActionPreference = 'Stop'

$Wipe = $false
$SetupExtra = @()
foreach ($a in $args) {
    if ($a -eq '--wipe') {
        $Wipe = $true
        $SetupExtra += '--wipe'
    } else {
        throw "Unknown argument: $a"
    }
}

$ToolchainBin = 'C:\llvm-mingw\bin'
if (Test-Path $ToolchainBin) {
    if (-not (($env:PATH -split ';') -contains $ToolchainBin)) {
        $env:PATH = "$ToolchainBin;$env:PATH"
    }
} else {
    Write-Warning "Toolchain directory '$ToolchainBin' not found; relying on PATH."
}

function Invoke-Checked {
    $cmd = $args[0]
    $cmdArgs = @($args[1..($args.Length - 1)])
    Write-Host "+ $cmd $($cmdArgs -join ' ')" -ForegroundColor DarkGray
    & $cmd @cmdArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed ($LASTEXITCODE): $cmd $($cmdArgs -join ' ')"
    }
}

$Cross32 = if ($env:CROSS_32) { $env:CROSS_32 } else { 'cross-clang-mingw-32-win.ini' }
$Cross64 = if ($env:CROSS_64) { $env:CROSS_64 } else { 'cross-clang-mingw-64-win.ini' }

if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }

# x86
if ($Wipe -or -not (Test-Path (Join-Path $Build32 'build.ninja'))) {
    Invoke-Checked meson setup --cross-file $Cross32 @SetupExtra $Build32
    Invoke-Checked meson setup --cross-file $Cross64 @SetupExtra $Build64
}
Invoke-Checked meson compile -C $Build32
Invoke-Checked meson compile -C $Build64

# TEMP: skip dist/install step; re-enable later.
exit 0

# without `--tags runtime`, the .a files are also installed
Invoke-Checked meson install -C $Build32 --destdir (Join-Path $DistDir '32bit') --tags 'runtime,doc'

# x86_64
Invoke-Checked meson install -C $Build64 --destdir (Join-Path $DistDir '64bit') --tags 'runtime,doc'

# docs
Copy-Item (Join-Path $PSScriptRoot 'README.md') $DistDir
# only copy MOD_README, I might have something in data_mods for testing
$DistDataMods = Join-Path $DistDir 'data_mods'
New-Item -ItemType Directory -Force -Path $DistDataMods | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'data_mods/MOD_README.txt') $DistDataMods

# while we wait for mesonbuild issue #4019 / PR #11954 to be solved, need to
# rename the special builds ourselves
Get-ChildItem -Path $DistDir -Recurse -Filter *.dll |
    Where-Object { $_.FullName -match '[\\/]special_builds[\\/]' } |
    ForEach-Object {
        $newName = $_.Name -replace '^ifs_hook.*\.dll$', 'ifs_hook.dll'
        if ($newName -ne $_.Name) {
            Move-Item -Force -Path $_.FullName -Destination (Join-Path $_.DirectoryName $newName)
        }
    }

$projInfo = meson introspect --projectinfo $Build32 | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw "meson introspect failed" }
$version = $projInfo.version

Push-Location $DistDir
try {
    $zipName = "ifs_layeredfs_$version.zip"
    if (Test-Path $zipName) { Remove-Item -Force $zipName }
    Compress-Archive -Path * -DestinationPath $zipName -CompressionLevel Optimal
} finally {
    Pop-Location
}
