#!/usr/bin/env pwsh
# NOTE: file is slop-ported from the .sh. Consider the .sh the source of truth.
# AI nonsense follows:

$ErrorActionPreference = 'Stop'

# mon note: I use a Dev Drive for speed, you probably want to change these
$Build32 = 'X:\layeredfs_build32'
$Build64 = 'X:\layeredfs_build64'

meson test -C $Build32 --print-errorlogs @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

meson test -C $Build64 --print-errorlogs @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
