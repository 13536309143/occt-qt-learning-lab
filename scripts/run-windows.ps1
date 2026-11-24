$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$prefix = Join-Path $root '.local\env'
if (!(Test-Path "$root\build-win\OcctQtStarter.exe")) { throw 'Run scripts/setup-windows.ps1 and scripts/build-windows.ps1 first' }
$env:Path = "$prefix\Library\bin;$prefix\Scripts;$prefix\bin;$prefix;$env:Path"
$env:QT_PLUGIN_PATH = "$prefix\Library\lib\qt6\plugins"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$env:QT_PLUGIN_PATH\platforms"
& "$root\build-win\OcctQtStarter.exe"
if ($LASTEXITCODE -ne 0) { throw 'Application failed' }
