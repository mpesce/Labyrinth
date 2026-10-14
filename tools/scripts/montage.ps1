# montage.ps1 -- tile snapshot images into one labelled PNG.
#
#   montage.ps1 out.png cols cellwidth file1 file2 ...
param([string]$out, [int]$cols, [int]$cell, [Parameter(ValueFromRemainingArguments=$true)][string[]]$files)

Add-Type -AssemblyName System.Drawing
$imgs = @()
foreach ($f in $files) { $imgs += [System.Drawing.Image]::FromFile((Resolve-Path $f)) }
$aspect = $imgs[0].Height / $imgs[0].Width
$ch = [int]($cell * $aspect) + 16
$rows = [math]::Ceiling($imgs.Count / $cols)
$bmp = New-Object System.Drawing.Bitmap(($cols * $cell), ($rows * $ch))
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::DimGray)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$font = New-Object System.Drawing.Font("Segoe UI", 8)
for ($i = 0; $i -lt $imgs.Count; $i++) {
    $x = ($i % $cols) * $cell; $y = [math]::Floor($i / $cols) * $ch
    $g.DrawImage($imgs[$i], $x, $y + 16, $cell, $ch - 16)
    $name = [System.IO.Path]::GetFileNameWithoutExtension($files[$i])
    $g.DrawString($name, $font, [System.Drawing.Brushes]::White, $x + 2, $y + 1)
}
$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
foreach ($im in $imgs) { $im.Dispose() }
Write-Output "wrote $out"
