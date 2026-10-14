# Check if the Windows Driver Kit is installed.
# This script is intended to be run on Windows.

$solutionPath = Join-Path $PSScriptRoot 'src\KernelGuard.sln'
Write-Host "Solution path: $solutionPath"

$msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error 'MSBuild was not found in PATH. Install Visual Studio with the WDK and make sure MSBuild is available.'
    exit 1
}
Write-Host "MSBuild found at: $($msbuild.Path)"

$wdfIntegrationVisible = @(
    'C:\Program Files (x86)\Windows Kits\10',
    'C:\Program Files\Windows Kits\10',
    'C:\Program Files (x86)\Windows Kits\8.1',
    'C:\Program Files\Windows Kits\8.1'
)

$foundWdk = $false
foreach ($root in $wdfIntegrationVisible) {
    if (Test-Path $root) {
        Write-Host "Windows Kits root detected: $root"
        $wdkHeaders = Get-ChildItem -Path $root -Recurse -Include wdm.h, ntddk.h, ntimage.h -ErrorAction SilentlyContinue
        if ($wdkHeaders) {
            $foundWdk = $true
            Write-Host "Found required WDK-style headers under: $root"
            $wdkHeaders | ForEach-Object { Write-Host "  $($_.FullName)" }
        }
    }
}

if (-not $foundWdk) {
    Write-Error 'WDK headers not found. Install the Windows Driver Kit (WDK) and reopen the solution in Visual Studio.'
    exit 1
}

Write-Host 'WDK check passed.'
exit 0
