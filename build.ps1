[CmdletBinding()]
Param(
    #[switch]$CustomParam,
    [Parameter(Position=0,Mandatory=$false,ValueFromRemainingArguments=$true)]
    [string[]]$BuildArguments
)

Write-Output "Windows PowerShell $($Host.Version)"

Set-StrictMode -Version 2.0; $ErrorActionPreference = "Stop"; $ConfirmPreference = "None"; trap { $host.SetShouldExit(1) }
$PSScriptRoot = Split-Path $MyInvocation.MyCommand.Path -Parent

###########################################################################
# CONFIGURATION
###########################################################################

$BuildProjectFile = "$PSScriptRoot\build\_build.csproj"
$TempDirectory = "$PSScriptRoot\\.tmp"

$DotNetGlobalFile = "$PSScriptRoot\\global.json"
$DotNetInstallUrl = "https://raw.githubusercontent.com/dotnet/cli/master/scripts/obtain/dotnet-install.ps1"
$DotNetReleasesUrl = "https://raw.githubusercontent.com/dotnet/core/master/release-notes/releases.json"

$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = 1
$env:DOTNET_CLI_TELEMETRY_OPTOUT = 1
$env:NUGET_XMLDOC_MODE = "skip"

###########################################################################
# EXECUTION
###########################################################################

function ExecSafe([scriptblock] $cmd) {
    & $cmd
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}

# If global.json exists, load expected version
if (Test-Path $DotNetGlobalFile) {
    $DotNetVersion = $(Get-Content $DotNetGlobalFile | Out-String | ConvertFrom-Json).sdk.version
}

# If dotnet is installed locally, and expected version is not set or installation matches the expected version
if ((Get-Command "dotnet" -ErrorAction SilentlyContinue) -ne $null -and `
     (!(Test-Path variable:DotNetVersion) -or $(& dotnet --version) -eq $DotNetVersion)) {
    $env:DOTNET_EXE = (Get-Command "dotnet").Path
}
else {
    $DotNetDirectory = "$TempDirectory\dotnet-win"
    $env:DOTNET_EXE = "$DotNetDirectory\dotnet.exe"

    # If expected version is not set, get latest version
    if (!(Test-Path variable:DotNetVersion)) {
        $DotNetVersion = $(Invoke-WebRequest -UseBasicParsing $DotNetReleasesUrl | ConvertFrom-Json)[0]."version-sdk"
    }

    # Download and execute install script
    $DotNetInstallFile = "$TempDirectory\dotnet-install.ps1"
    md -force $TempDirectory > $null
    (New-Object System.Net.WebClient).DownloadFile($DotNetInstallUrl, $DotNetInstallFile)
    ExecSafe { & $DotNetInstallFile -InstallDir $DotNetDirectory -Version $DotNetVersion -NoPath }
}

Write-Output "Microsoft (R) .NET Core SDK version $(& $env:DOTNET_EXE --version)"

ExecSafe { & $env:DOTNET_EXE run --project $BuildProjectFile -- $BuildArguments }
