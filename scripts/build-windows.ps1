param([switch]$SkipGui)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
# Import the installed Visual Studio x64 compiler environment into this process.
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path $vswhere)) { throw 'Install Visual Studio 2022 Desktop development with C++ first' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'MSVC x64 tools were not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    $vars = & cmd.exe /s /c "`"`"$vcvars`" >nul && set`""
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize MSVC' }
    foreach ($line in $vars) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process') }
    }
}
$prefix = Join-Path $root '.local\env'
if (!(Test-Path $prefix)) { throw 'Run scripts/setup-windows.ps1 first' }
$env:Path = "$prefix\Library\bin;$prefix\Scripts;$prefix\bin;$prefix;$env:Path"
$cmake = (Get-ChildItem $prefix -Recurse -Filter cmake.exe | Select-Object -First 1).FullName
$ctest = (Get-ChildItem $prefix -Recurse -Filter ctest.exe | Select-Object -First 1).FullName
$qt = (Get-ChildItem $prefix -Recurse -Filter Qt6Config.cmake | Select-Object -First 1).DirectoryName
$occt = (Get-ChildItem $prefix -Recurse -Filter OpenCASCADEConfig.cmake | Select-Object -First 1).DirectoryName
if (!$cmake -or !$ctest -or !$qt -or !$occt) { throw 'Incomplete dependency environment' }
$gui = if ($SkipGui) { 'OFF' } else { 'ON' }
New-Item -ItemType Directory -Force reports\windows-local | Out-Null
& $cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release "-DQt6_DIR=$qt" "-DOpenCASCADE_DIR=$occt" "-DCMAKE_PREFIX_PATH=$prefix\Library;$prefix" "-DOCCT_ENABLE_GUI_TESTS=$gui" 2>&1 | Tee-Object reports\windows-local\configure.log
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
& $cmake --build build-win --parallel 4 2>&1 | Tee-Object reports\windows-local\build.log
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
& $ctest --test-dir build-win --output-on-failure --output-junit "$root\reports\windows-local\ctest.xml" 2>&1 | Tee-Object reports\windows-local\ctest.log
if ($LASTEXITCODE -ne 0) { throw 'Tests failed; see reports/windows-local and build-win/*-artifacts' }
Write-Host 'PASS. Launch using scripts/run-windows.ps1; keep this environment for runtime DLLs.'
