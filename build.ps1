param([switch]$NoCache)
$ErrorActionPreference = 'Stop'
$docker = Get-Command docker -ErrorAction SilentlyContinue
if (-not $docker) { throw 'Open Docker Desktop, then open a new PowerShell window and retry.' }
if ((& $docker.Source info --format '{{.OSType}}') -ne 'linux') { throw 'Start Docker Desktop with Linux containers enabled.' }
Push-Location $PSScriptRoot
try {
    $args = @('build', '--progress=plain', '-t', 'nova2-build', '-f', 'portbase/Dockerfile.build')
    if ($NoCache) { $args += '--no-cache' }
    $args += 'portbase'
    & $docker.Source @args
    if ($LASTEXITCODE -ne 0) { throw 'Docker build environment failed.' }
    & $docker.Source run --rm --mount "type=bind,source=$PSScriptRoot,target=/src" -w /src nova2-build bash -lc 'make -j2 && make libs && bash package_portmaster.sh'
    if ($LASTEXITCODE -ne 0) { throw 'Port compilation or packaging failed.' }
    Write-Host "Built: $PSScriptRoot/build/nova2.zip"
} finally { Pop-Location }
