[CmdletBinding()]
param(
    [ValidateSet('Configure','Assets','Build','Stage','Run','All')][string]$Action = 'All',
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [string]$Workspace = 'C:\ZeldaDev'
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$toolchain = Get-Content -LiteralPath (Join-Path $repo 'docs\LIVING-HYRULE-TOOLCHAIN.json') -Raw | ConvertFrom-Json
$build = Join-Path $Workspace 'build\living-hyrule-vs2022'
$runtime = Join-Path $Workspace 'runtime\development'
$python = Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312\python.exe'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio 2022 C++ build tools were not found.' }
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { throw 'The Visual Studio CMake component is missing.' }
if (-not (Test-Path -LiteralPath $python)) { throw 'Python 3.12 is missing. Update the Python path in this script.' }
$env:PATH = (Split-Path $python) + ';' + (Split-Path $cmake) + ';' + $env:PATH
$env:VCPKG_DISABLE_METRICS = '1'
$env:VCPKG_MAX_CONCURRENCY = '8'
# Limit concurrent compiler workers on this 16-thread laptop.
$env:_CL_ = '/MP4'

function Invoke-CMake {
    & $cmake @args
    if ($LASTEXITCODE -ne 0) { throw "CMake failed with exit code $LASTEXITCODE" }
}

if ($Action -in 'Configure','All') {
    Invoke-CMake -S $repo -B $build -G 'Visual Studio 17 2022' -T v143 -A x64 `
        "-DVCPKG_ROOT=$Workspace/tools/vcpkg" "-DVCPKG_PINNED_COMMIT=$($toolchain.vcpkgCommit)" "-DPython3_EXECUTABLE=$python" `
        "-DSOH_ROM_PATH=$Workspace/roms" "-DCMAKE_INSTALL_PREFIX=$runtime"
}
if ($Action -eq 'Assets') {
    Invoke-CMake --build $build --config $Configuration --target ExtractAssets --parallel 2
}
if ($Action -in 'Build','All') {
    # Reuse local game archives on ordinary edits; Assets explicitly refreshes them.
    $hasGameArchive = (Test-Path (Join-Path $build 'soh\oot.o2r')) -or (Test-Path (Join-Path $build 'soh\oot-mq.o2r'))
    $assetTarget = if ($hasGameArchive) { 'GenerateSohOtr' } else { 'ExtractAssets' }
    Invoke-CMake --build $build --config $Configuration --target $assetTarget --parallel 2
    Invoke-CMake --build $build --config $Configuration --target soh --parallel 2
}
if ($Action -in 'Stage','All') {
    $active = Get-Process soh -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq (Join-Path $runtime 'soh.exe') }
    if ($active) { throw 'Close the development game before staging a new build.' }
    Invoke-CMake --install $build --config $Configuration --prefix $runtime --component ship
    foreach ($name in 'oot.o2r','oot-mq.o2r') {
        $archive = Join-Path $build "soh\$name"
        if (Test-Path -LiteralPath $archive) { Copy-Item -LiteralPath $archive -Destination $runtime -Force }
    }
    Write-Host "Development runtime staged at $runtime"
}
if ($Action -eq 'Run') {
    Start-Process -FilePath (Join-Path $runtime 'soh.exe') -WorkingDirectory $runtime -WindowStyle Normal
}
