$root = Split-Path $PSScriptRoot -Parent
$prefix = Join-Path $root '.local\env'
$env:Path = "$prefix\Library\bin;$prefix\Scripts;$prefix\bin;$prefix;$env:Path"
& "$root\build-win\OcctQtStarter.exe"
if ($LASTEXITCODE -ne 0) { throw 'Application failed' }
