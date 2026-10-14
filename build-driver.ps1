Param(
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64'
)

$solutionPath = Join-Path $PSScriptRoot 'src\KernelGuard.sln'
Write-Host "Building solution: $solutionPath"
Write-Host "Configuration: $Configuration, Platform: $Platform"

$checkScript = Join-Path $PSScriptRoot 'check-wdk.ps1'
if (-not (Test-Path $checkScript)) {
    Write-Error "Missing check-wdk.ps1 script."
    exit 1
}

Write-Host 'Checking WDK installation...'
& $checkScript
if ($LASTEXITCODE -ne 0) {
    Write-Error 'WDK check failed. Aborting build.'
    exit $LASTEXITCODE
}

$msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error 'MSBuild was not found. Install Visual Studio with MSBuild.'
    exit 1
}

Write-Host "Using MSBuild: $($msbuild.Path)"
$buildArgs = @(
    $solutionPath,
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    '/m'
)

& $msbuild @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error 'MSBuild reported errors.'
    exit $LASTEXITCODE
}

$sysRoot = Join-Path $PSScriptRoot 'src\KernelGuard'
$sysFiles = Get-ChildItem -Path $sysRoot -Recurse -Filter '*.sys' -ErrorAction SilentlyContinue
if (-not $sysFiles) {
    Write-Error 'Build completed but no .sys files were found under src\KernelGuard'
    exit 1
}

Write-Host 'Build succeeded. .sys file(s) found:'
$sysFiles | ForEach-Object { Write-Host "  $($_.FullName)" }
exit 0
