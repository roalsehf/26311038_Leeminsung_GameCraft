$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
& (Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe') (Join-Path $root '26311038_Leeminsung_GameCraft.sln') /t:Build /p:Configuration=Release /p:Platform=x64 /verbosity:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }
$finalPackage = Join-Path $root 'dist\GameCraft-f4'
$package = Join-Path $root ('.tools\package-stage-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $package -Force | Out-Null
$release = Join-Path $root 'bin\x64\Release'
Copy-Item -LiteralPath (Join-Path $release '26311038_Leeminsung_GameCraft.exe') -Destination $package
Get-ChildItem -LiteralPath $release -Filter '*.dll' -File | Copy-Item -Destination $package
$manifest = Get-Content -LiteralPath (Join-Path $root 'docs\ResourceManifest.json') -Raw | ConvertFrom-Json
foreach ($resource in $manifest.resources) {
    $destination = Join-Path $package $resource.path
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $resource.path) -Destination $destination
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $resource.sha256) {
        throw ('Stale resource manifest: ' + $resource.path)
    }
}
Copy-Item -LiteralPath (Join-Path $root 'docs\PLAY.md') -Destination (Join-Path $package 'PLAY.md')
Copy-Item -LiteralPath (Join-Path $root 'docs\ResourceList.md') -Destination (Join-Path $package 'RESOURCE_SOURCES.md')
$licenses = Join-Path $package 'licenses'
New-Item -ItemType Directory -Path $licenses -Force | Out-Null
foreach ($name in @('LICENSE.txt','NOTICE.md')) {
    Copy-Item -LiteralPath (Join-Path $root ('packages\Microsoft.DXSDK.D3DX.9.29.952.8\' + $name)) -Destination $licenses
}
$workspacePrefix = [IO.Path]::GetFullPath($root) + [IO.Path]::DirectorySeparatorChar
$backupRoot = Join-Path $root '.tools\package-backups'
$backup = Join-Path $backupRoot ('GameCraft-f4-' + [Guid]::NewGuid().ToString('N'))
foreach ($candidate in @($package,$finalPackage,$backup)) {
    if (![IO.Path]::GetFullPath($candidate).StartsWith($workspacePrefix,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Package path escapes the workspace.'
    }
}
New-Item -ItemType Directory -Path (Split-Path $finalPackage -Parent) -Force | Out-Null
if (Test-Path -LiteralPath $finalPackage) {
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    Move-Item -LiteralPath $finalPackage -Destination $backup
}
Move-Item -LiteralPath $package -Destination $finalPackage
$archive = Join-Path $root 'dist\GameCraft-f4.zip'
Compress-Archive -Path (Join-Path $finalPackage '*') -DestinationPath $archive -Force
Write-Output ('Release package: ' + $archive)
