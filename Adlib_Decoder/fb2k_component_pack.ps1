$ErrorActionPreference = "Stop"
$ComponentName = "foo_input_adlib_opl"
$DllName       = "$ComponentName.dll"
$X86Dll = Join-Path $PSScriptRoot "Release\$DllName"
$X64Dll = Join-Path $PSScriptRoot "x64\Release\$DllName"
if (-not (Test-Path $X86Dll)) { throw "找不到 x86 DLL: $X86Dll`n请先编译 Release | x86`nx86 DLL not found: $X86Dll`nPlease build Release | x86 first" }
$OutDir   = Join-Path $PSScriptRoot "dist"
$ZipPath  = Join-Path $OutDir "$ComponentName.zip"
$Fb2kPath = Join-Path $OutDir "$ComponentName.fb2k-component"
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$Stage = Join-Path ([IO.Path]::GetTempPath()) ("fb2kpack_" + [Guid]::NewGuid().ToString("N"))
try {
    New-Item -ItemType Directory -Path (Join-Path $Stage "x64") -Force | Out-Null
    Copy-Item $X86Dll $Stage -Force
    Copy-Item $X64Dll (Join-Path $Stage "x64\$DllName") -Force
    if (Test-Path $ZipPath)  { Remove-Item $ZipPath  -Force }
    if (Test-Path $Fb2kPath) { Remove-Item $Fb2kPath -Force }
    Compress-Archive -Path (Join-Path $Stage "*") -DestinationPath $ZipPath -Force
    Rename-Item -Path $ZipPath -NewName ([IO.Path]::GetFileName($Fb2kPath))
    Write-Host "OK: $Fb2kPath"
}
finally {
    if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
}