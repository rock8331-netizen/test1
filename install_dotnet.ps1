[Net.ServicePointManager]::SecurityProtocol = 'Tls12'
$out = Join-Path $env:TEMP 'dotnet-install.ps1'
(New-Object System.Net.WebClient).DownloadFile('https://dot.net/v1/dotnet-install.ps1', $out)
Write-Host "Downloaded to: $out"
# SDK 8.0 설치
& $out -Channel '8.0' -Quality 'GA'
Write-Host "DOTNET_INSTALL_DONE"
