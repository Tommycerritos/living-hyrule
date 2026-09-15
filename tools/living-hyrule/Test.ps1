[CmdletBinding()]
param([string]$Workspace = 'C:\ZeldaDev')

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ build tools were not found.' }
$cmakeDir = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$cmake = Join-Path $cmakeDir 'cmake.exe'
$ctest = Join-Path $cmakeDir 'ctest.exe'
$testBuild = Join-Path $Workspace 'build\living-hyrule-tests'
$jsonInclude = Join-Path $Workspace 'tools\vcpkg\installed\x64-windows-static\include'

& $cmake -S (Join-Path $repo 'tests\living-hyrule') -B $testBuild -G 'Visual Studio 17 2022' -A x64 "-DLIVING_HYRULE_JSON_INCLUDE=$jsonInclude"
if ($LASTEXITCODE -ne 0) { throw 'Test configuration failed.' }
& $cmake --build $testBuild --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
& $ctest --test-dir $testBuild -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Living Hyrule tests failed.' }
