# labdrive.ps1 -- drive a running Labyrinth by posting window messages.
#
#   labdrive.ps1 "action; action; ..."
#
# actions:
#   command <id> [menu]     WM_INITMENUPOPUP + WM_COMMAND (menu id)
#   click <child> x y       left button down/up in child window n (0 view, 1 info, 2 nav)
#   down <child> x y [keys] / up <child> x y [keys]
#   move <child> x y [keys] mouse move (keys: wParam MK_ flags, 4 shift, 8 control)
#   key <vk>                WM_KEYDOWN to the main window
#   sleep <ms>
#   capture <file.png>      PrintWindow of the main window
#   status                  list the child windows
param([string]$actions)

Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class W {
    [DllImport("user32.dll", CharSet=CharSet.Ansi)] public static extern IntPtr FindWindowA(string c, IntPtr t);
    [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr h, uint m, IntPtr w, IntPtr l);
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc f, IntPtr p);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    public static List<IntPtr> Children(IntPtr h) {
        var list = new List<IntPtr>();
        EnumChildWindows(h, (x, p) => { list.Add(x); return true; }, IntPtr.Zero);
        return list;
    }
}
"@
Add-Type -AssemblyName System.Drawing

$main = [W]::FindWindowA("InterSpace", [IntPtr]::Zero)
for ($i = 0; $i -lt 50 -and $main -eq [IntPtr]::Zero; $i++) {
    Start-Sleep -Milliseconds 200
    $main = [W]::FindWindowA("InterSpace", [IntPtr]::Zero)
}
if ($main -eq [IntPtr]::Zero) { Write-Output "no Labyrinth window"; exit 1 }
$kids = [W]::Children($main)

function MakeLParam([int]$x, [int]$y) { return [IntPtr](($y -shl 16) -bor ($x -band 0xffff)) }
function Keys($w, [int]$def) { if ($w.Count -gt 4) { return [IntPtr][int]$w[4] } return [IntPtr]$def }

foreach ($act in $actions.Split(';')) {
    $w = $act.Trim().Split(' ', [StringSplitOptions]::RemoveEmptyEntries)
    if ($w.Count -eq 0) { continue }
    switch ($w[0]) {
        "status" {
            foreach ($k in $kids) {
                $r = New-Object W+RECT; [void][W]::GetWindowRect($k, [ref]$r)
                Write-Output ("{0} {1},{2} {3}x{4}" -f $k, $r.L, $r.T, ($r.R - $r.L), ($r.B - $r.T))
            }
        }
        "command" {
            $menu = 0; if ($w.Count -gt 2) { $menu = [int]$w[2] }
            [void][W]::PostMessageA($main, 0x117, [IntPtr]::Zero, [IntPtr]$menu)
            [void][W]::PostMessageA($main, 0x111, [IntPtr][int]$w[1], [IntPtr]::Zero)
        }
        "key" { [void][W]::PostMessageA($main, 0x100, [IntPtr][int]$w[1], [IntPtr]::Zero) }
        "move" { [void][W]::PostMessageA($kids[[int]$w[1]], 0x200, (Keys $w 0), (MakeLParam $w[2] $w[3])) }
        "down" { [void][W]::PostMessageA($kids[[int]$w[1]], 0x201, (Keys $w 1), (MakeLParam $w[2] $w[3])) }
        "up"   { [void][W]::PostMessageA($kids[[int]$w[1]], 0x202, (Keys $w 0), (MakeLParam $w[2] $w[3])) }
        "click" {
            [void][W]::PostMessageA($kids[[int]$w[1]], 0x201, [IntPtr]1, (MakeLParam $w[2] $w[3]))
            Start-Sleep -Milliseconds 30
            [void][W]::PostMessageA($kids[[int]$w[1]], 0x202, [IntPtr]0, (MakeLParam $w[2] $w[3]))
        }
        "sleep" { Start-Sleep -Milliseconds ([int]$w[1]) }
        "capture" {
            $r = New-Object W+RECT; [void][W]::GetWindowRect($main, [ref]$r)
            $bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
            $g = [System.Drawing.Graphics]::FromImage($bmp)
            $dc = $g.GetHdc()
            [void][W]::PrintWindow($main, $dc, 2)
            $g.ReleaseHdc($dc); $g.Dispose()
            $bmp.Save($w[1], [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Output "captured $($w[1])"
        }
    }
}
