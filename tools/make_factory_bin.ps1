param(
    [string]$BuildDir = "build",
    [string]$OutFile = "release/smart86-ha-panel.factory.bin"
)

$ErrorActionPreference = "Stop"

function Resolve-Python {
    $candidates = @()
    if ($env:IDF_PYTHON_ENV_PATH) {
        $candidates += (Join-Path $env:IDF_PYTHON_ENV_PATH "Scripts\python.exe")
    }
    $candidates += "python"

    foreach ($candidate in $candidates) {
        if ($candidate -eq "python") {
            try {
                & python --version *> $null
                if ($LASTEXITCODE -eq 0) {
                    return "python"
                }
            }
            catch {
                # try next
            }
        } elseif (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Python not found. Activate ESP-IDF environment or set IDF_PYTHON_ENV_PATH."
}

function Resolve-EspToolPath {
    if (-not $env:IDF_PATH) {
        throw "IDF_PATH is not set. Run in ESP-IDF shell/environment."
    }
    $path = Join-Path $env:IDF_PATH "components\esptool_py\esptool\esptool.py"
    if (-not (Test-Path $path)) {
        throw "esptool.py not found at $path"
    }
    return $path
}

$buildPath = (Resolve-Path $BuildDir).Path
$flasherArgsPath = Join-Path $buildPath "flasher_args.json"
if (-not (Test-Path $flasherArgsPath)) {
    throw "Missing $flasherArgsPath. Run `idf.py build` first."
}

$flasher = Get-Content -Raw -Path $flasherArgsPath | ConvertFrom-Json

$pythonExe = Resolve-Python
$esptoolPath = Resolve-EspToolPath

$outPath = Join-Path (Get-Location) $OutFile
$outDir = Split-Path -Parent $outPath
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

$chip = $flasher.extra_esptool_args.chip
if ([string]::IsNullOrWhiteSpace($chip)) {
    $chip = "esp32p4"
}

$mergeArgs = @("--chip", $chip, "merge_bin", "-o", $outPath)
if ($flasher.write_flash_args) {
    foreach ($arg in $flasher.write_flash_args) {
        $mergeArgs += [string]$arg
    }
}

$parts = @()
foreach ($prop in $flasher.flash_files.PSObject.Properties) {
    $offset = [string]$prop.Name
    $relPath = [string]$prop.Value
    $absPath = Join-Path $buildPath $relPath
    if (-not (Test-Path $absPath)) {
        throw "Missing flash file: $absPath"
    }
    $offsetNum = [Convert]::ToInt64($offset, 16)
    $parts += [PSCustomObject]@{
        Offset = $offset
        OffsetNum = $offsetNum
        Path = $absPath
    }
}

$parts = $parts | Sort-Object OffsetNum
foreach ($part in $parts) {
    $mergeArgs += @($part.Offset, $part.Path)
}

Write-Host "Creating factory image:"
Write-Host "  Out: $outPath"
Write-Host "  Chip: $chip"
Write-Host "  Parts:"
foreach ($part in $parts) {
    Write-Host "    $($part.Offset)  $($part.Path)"
}

& $pythonExe $esptoolPath @mergeArgs
if ($LASTEXITCODE -ne 0) {
    throw "merge_bin failed with exit code $LASTEXITCODE"
}

Write-Host ""
Write-Host "Factory image created:"
Write-Host "  $outPath"
Write-Host ""
Write-Host "Flash with esptool at offset 0x0, example:"
Write-Host "  python -m esptool --chip $chip -p COM3 --before default_reset --after hard_reset write_flash 0x0 `"$outPath`""
