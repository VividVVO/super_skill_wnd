param(
    [string]$ProcessName = "MapleStory",
    [string]$PathContains = "mxd",
    [string]$OutFile = "",
    [int]$IntervalSeconds = 5,
    [int]$DurationMinutes = 240
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutFile)) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutFile = Join-Path (Join-Path $PSScriptRoot "..\build") "perf_capture_$stamp.csv"
}

$outDir = Split-Path -Parent $OutFile
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class GuiResources {
    [DllImport("user32.dll")]
    public static extern int GetGuiResources(IntPtr hProcess, int uiFlags);
}
"@

function Get-ProcessPathSafe {
    param([System.Diagnostics.Process]$Process)
    try {
        return $Process.MainModule.FileName
    } catch {
        return ""
    }
}

function Select-TargetProcess {
    $candidates = Get-Process -ErrorAction SilentlyContinue |
        Where-Object {
            $_.ProcessName -like "*$ProcessName*" -or
            (Get-ProcessPathSafe $_) -like "*$PathContains*"
        }

    $withPath = foreach ($p in $candidates) {
        [PSCustomObject]@{
            Process = $p
            Path = Get-ProcessPathSafe $p
        }
    }

    $preferred = $withPath |
        Where-Object { $_.Path -like "*$PathContains*" } |
        Sort-Object { $_.Process.StartTime } -Descending |
        Select-Object -First 1

    if ($preferred) {
        return $preferred
    }

    return $withPath |
        Sort-Object { $_.Process.StartTime } -Descending |
        Select-Object -First 1
}

"timestamp,pid,name,path,uptimeSec,cpuTotalSec,cpuDeltaSec,cpuPctOneCore,cpuPctTotal,workingSetMB,privateMB,virtualMB,handles,threads,gdiObjects,userObjects" |
    Out-File -LiteralPath $OutFile -Encoding utf8 -Force

$endAt = (Get-Date).AddMinutes($DurationMinutes)
$lastCpu = $null
$lastTime = $null

while ((Get-Date) -lt $endAt) {
    $selected = Select-TargetProcess
    if (-not $selected) {
        Start-Sleep -Seconds $IntervalSeconds
        continue
    }

    $p = Get-Process -Id $selected.Process.Id -ErrorAction SilentlyContinue
    if (-not $p) {
        Start-Sleep -Seconds $IntervalSeconds
        continue
    }

    $now = Get-Date
    $cpu = if ($p.CPU -ne $null) { [double]$p.CPU } else { 0.0 }
    $elapsed = if ($lastTime) { [Math]::Max(0.001, ($now - $lastTime).TotalSeconds) } else { 0.0 }
    $cpuDelta = if ($lastCpu -ne $null) { [Math]::Max(0.0, $cpu - $lastCpu) } else { 0.0 }
    $cpuPctOneCore = if ($elapsed -gt 0) { ($cpuDelta / $elapsed) * 100.0 } else { 0.0 }
    $cpuPctTotal = $cpuPctOneCore / [Environment]::ProcessorCount
    $lastCpu = $cpu
    $lastTime = $now

    $path = Get-ProcessPathSafe $p
    $gdi = [GuiResources]::GetGuiResources($p.Handle, 0)
    $user = [GuiResources]::GetGuiResources($p.Handle, 1)
    $uptime = 0
    try {
        $uptime = ($now - $p.StartTime).TotalSeconds
    } catch {
        $uptime = 0
    }

    $line = [PSCustomObject]@{
        timestamp = $now.ToString("yyyy-MM-dd HH:mm:ss")
        pid = $p.Id
        name = $p.ProcessName
        path = $path
        uptimeSec = [Math]::Round($uptime, 1)
        cpuTotalSec = [Math]::Round($cpu, 3)
        cpuDeltaSec = [Math]::Round($cpuDelta, 3)
        cpuPctOneCore = [Math]::Round($cpuPctOneCore, 2)
        cpuPctTotal = [Math]::Round($cpuPctTotal, 2)
        workingSetMB = [Math]::Round($p.WorkingSet64 / 1MB, 2)
        privateMB = [Math]::Round($p.PrivateMemorySize64 / 1MB, 2)
        virtualMB = [Math]::Round($p.VirtualMemorySize64 / 1MB, 2)
        handles = $p.HandleCount
        threads = $p.Threads.Count
        gdiObjects = $gdi
        userObjects = $user
    }

    $line | Export-Csv -LiteralPath $OutFile -NoTypeInformation -Append -Encoding utf8
    Start-Sleep -Seconds $IntervalSeconds
}

Write-Output "Wrote $OutFile"
