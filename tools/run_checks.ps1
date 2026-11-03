param([switch]$Visual)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$testFolder = Join-Path $root '.tools\checks'
New-Item -ItemType Directory -Path $testFolder -Force | Out-Null
[xml]$project = Get-Content -LiteralPath (Join-Path $root '26311038_Leeminsung_GameCraft.vcxproj') -Raw
$ns = [Xml.XmlNamespaceManager]::new($project.NameTable)
$ns.AddNamespace('m', $project.DocumentElement.NamespaceURI)
foreach ($node in @($project.SelectNodes('//m:ClCompile[@Include] | //m:ClInclude[@Include] | //m:None | //m:Target', $ns))) {
    [void]$node.ParentNode.RemoveChild($node)
}
foreach ($node in $project.SelectNodes('//m:Import', $ns)) {
    if ($node.Project.StartsWith('packages\')) {
        $node.Project = Join-Path $root $node.Project
        $node.RemoveAttribute('Condition')
    }
}
$namespace = $project.DocumentElement.NamespaceURI
$group = $project.CreateElement('ItemGroup', $namespace)
$sources = @('GamePlayer.cpp','GameStage.cpp','GameSession.cpp','GameResources.cpp')
if ($Visual) { $sources += @('GameView.cpp','tests\VisualChecks.cpp') }
else { $sources += 'tests\GameplayTests.cpp' }
foreach ($source in $sources) {
    $item = $project.CreateElement('ClCompile', $namespace)
    $item.SetAttribute('Include', (Join-Path $root $source))
    [void]$group.AppendChild($item)
}
[void]$project.DocumentElement.AppendChild($group)
$definitions = $project.CreateElement('ItemDefinitionGroup', $namespace)
$compile = $project.CreateElement('ClCompile', $namespace)
$include = $project.CreateElement('AdditionalIncludeDirectories', $namespace)
$include.InnerText = $root + ';%(AdditionalIncludeDirectories)'
[void]$compile.AppendChild($include)
[void]$definitions.AppendChild($compile)
[void]$project.DocumentElement.AppendChild($definitions)
$projectName = if ($Visual) { 'GameplayVisual' } else { 'GameplayChecks' }
foreach ($node in $project.SelectNodes('//m:IntDir', $ns)) {
    $node.InnerText = '$(ProjectDir)obj\$(MSBuildProjectName)\$(Platform)\$(Configuration)\'
}
$testProject = Join-Path $testFolder ($projectName + '.vcxproj')
$project.Save($testProject)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild $testProject /t:Build /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw 'Gameplay test build failed.' }
if (!$Visual) {
    & (Join-Path $testFolder 'bin\x64\Debug\GameplayChecks.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Gameplay tests failed.' }
}
