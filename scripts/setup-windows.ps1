$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$bin = Join-Path $root '.local\bin'
New-Item -ItemType Directory -Force $bin | Out-Null
$mm = Join-Path $bin 'micromamba.exe'
if (!(Test-Path $mm)) {
    Invoke-WebRequest 'https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-win-64' -OutFile $mm
}
& $mm create -y --no-rc -r "$root\.local\mamba" -p "$root\.local\env" -c conda-forge 'occt=7.9.3' 'qt6-main=6.8.4' 'cmake>=3.21' ninja
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed' }
Write-Host 'Next: .\scripts\build-windows.ps1'
