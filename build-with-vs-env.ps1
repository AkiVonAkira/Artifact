# Build script with Visual Studio environment
# This script sets up the VS environment before building

$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvarsPath = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"

if (Test-Path $vcvarsPath) {
    Write-Host "Setting up Visual Studio 2022 environment..." -ForegroundColor Cyan

    # Create a temporary batch file to run vcvars and then build
    $tempBatch = [System.IO.Path]::GetTempFileName() + ".bat"

    $batchContent = @"
@echo off
call "$vcvarsPath"
cd /d "C:\Projects\Unsorted\Artifact\build\Windows\Win64-Editor-Debug"
cmake --build . --target artifact_editor -j 10
"@

    Set-Content -Path $tempBatch -Value $batchContent

    Write-Host "Building with Visual Studio environment..." -ForegroundColor Yellow
    cmd.exe /c $tempBatch

    Remove-Item $tempBatch

    Write-Host "`nBuild complete!" -ForegroundColor Green
} else {
    Write-Host "Visual Studio 2022 not found at expected location" -ForegroundColor Red
    Write-Host "Please run this from a Visual Studio Developer Command Prompt" -ForegroundColor Yellow
}

