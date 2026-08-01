param(
    [string]$Destination = (Join-Path $PSScriptRoot "..\out\dependencies")
)

$ErrorActionPreference = "Stop"

$dependencies = @(
    @{
        Name = "SDL3"
        Archive = "SDL3-devel-3.2.22-VC.zip"
        Url = "https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-devel-3.2.22-VC.zip"
        Sha256 = "093821FCD2B0EAFEDC86E93713687136872A6556966DB036FEBA2672F58586ED"
        Directory = "SDL3-3.2.22"
    },
    @{
        Name = "SDL3_image"
        Archive = "SDL3_image-devel-3.2.4-VC.zip"
        Url = "https://github.com/libsdl-org/SDL_image/releases/download/release-3.2.4/SDL3_image-devel-3.2.4-VC.zip"
        Sha256 = "76141D9535B77B1D6561368DE934B3797D87F834906016E1087940C85A8DAB85"
        Directory = "SDL3_image-3.2.4"
    }
)

$destinationRoot = [System.IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null

$prefixes = foreach ($dependency in $dependencies) {
    $archivePath = Join-Path $destinationRoot $dependency.Archive
    $extractRoot = Join-Path $destinationRoot $dependency.Name
    $packageRoot = Join-Path $extractRoot $dependency.Directory

    if (-not (Test-Path -LiteralPath $packageRoot -PathType Container)) {
        Invoke-WebRequest -Uri $dependency.Url -OutFile $archivePath
        $actualHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
        if ($actualHash -ne $dependency.Sha256) {
            throw "SHA-256 mismatch for $($dependency.Archive): $actualHash"
        }

        Expand-Archive -LiteralPath $archivePath -DestinationPath $extractRoot -Force
    }

    if (-not (Test-Path -LiteralPath $packageRoot -PathType Container)) {
        throw "The $($dependency.Name) archive did not contain $($dependency.Directory)"
    }

    $packageRoot
}

$cmakePrefixes = @($prefixes)
if ($env:CMAKE_PREFIX_PATH) {
    $cmakePrefixes += $env:CMAKE_PREFIX_PATH
}
$cmakePrefixPath = $cmakePrefixes -join ";"
if ($env:GITHUB_ENV) {
    "CMAKE_PREFIX_PATH=$cmakePrefixPath" | Add-Content -LiteralPath $env:GITHUB_ENV
}
if ($env:GITHUB_PATH) {
    foreach ($prefix in $prefixes) {
        (Join-Path $prefix "lib\x64") | Add-Content -LiteralPath $env:GITHUB_PATH
    }
}

$cmakePrefixPath
