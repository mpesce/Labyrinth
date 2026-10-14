# makeicon.ps1 <in.ico> <out.ico> -- add pixel-doubled sizes (PNG entries) to a 32x32 icon
param([string]$in, [string]$out)
Add-Type -AssemblyName System.Drawing
$src = (New-Object System.Drawing.Icon($in, 32, 32)).ToBitmap()
$sizes = 16, 24, 32, 48, 64, 96, 128, 256
$pngs = @()
foreach ($n in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap $n, $n, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    # whole multiples stay pixel-sharp; the in-between sizes are smoothed
    if ($n % 32 -eq 0) { $g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half' }
    else { $g.InterpolationMode = 'HighQualityBicubic'; $g.PixelOffsetMode = 'HighQuality' }
    $g.DrawImage($src, 0, 0, $n, $n)
    $g.Dispose()
    $ms = New-Object IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += ,($ms.ToArray())
    $bmp.Dispose()
}
$ms = New-Object IO.MemoryStream
$w = New-Object IO.BinaryWriter $ms
$w.Write([UInt16]0); $w.Write([UInt16]1); $w.Write([UInt16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $n = $sizes[$i]; $b = if ($n -ge 256) { 0 } else { $n }
    $w.Write([byte]$b); $w.Write([byte]$b); $w.Write([byte]0); $w.Write([byte]0)
    $w.Write([UInt16]1); $w.Write([UInt16]32)
    $w.Write([UInt32]$pngs[$i].Length); $w.Write([UInt32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $w.Write($p) }
$w.Flush()
[IO.File]::WriteAllBytes($out, $ms.ToArray())
"wrote $out ($($sizes -join ', '))"
