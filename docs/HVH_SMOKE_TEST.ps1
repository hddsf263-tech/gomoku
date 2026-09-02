Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class MouseSim {
    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
}
"@

$exe = 'D:\gomoku\build\Gomoku.exe'
$p = Start-Process -FilePath $exe -PassThru
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Wait-ElementByName($parent, $name, $timeoutSec) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $name)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
        $el = $parent.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
        if ($el) { return $el }
        Start-Sleep -Milliseconds 200
    }
    return $null
}

function Wait-ElementByClass($parent, $className, $timeoutSec) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ClassNameProperty, $className)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
        $el = $parent.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
        if ($el) { return $el }
        Start-Sleep -Milliseconds 200
    }
    return $null
}

function Find-ButtonByName($parent, [string[]]$names) {
    foreach ($n in $names) {
        $cond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, $n)
        $el = $parent.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
        if ($el) { return $el }
    }
    return $null
}

function Invoke-Element($el) {
    $pattern = $el.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()
    Start-Sleep -Milliseconds 300
}

function Click-Screen($x, $y) {
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point($x, $y)
    [MouseSim]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    [MouseSim]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 250
}

function Get-LabelTexts($win) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Text)
    $els = $win.FindAll([System.Windows.Automation.TreeScope]::Descendants, $cond)
    $names = @()
    foreach ($el in $els) {
        $n = $el.Current.Name
        if ($n) { $names += $n }
    }
    return ($names -join ' | ')
}

Start-Sleep -Seconds 2
$win = Wait-ElementByName $root '五子棋 - Gomoku' 10
if (-not $win) { Write-Output 'FAIL: MAIN WINDOW NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
Write-Output 'STEP: WINDOW FOUND'

$newGame = Find-ButtonByName $win @('新游戏')
if (-not $newGame) { Write-Output 'FAIL: NEW GAME BUTTON NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
Invoke-Element $newGame

$dialog = Wait-ElementByName $root '新游戏' 6
if (-not $dialog) { Write-Output 'FAIL: GAME MODE DIALOG NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
$ok = Find-ButtonByName $dialog @('OK', '确定')
if (-not $ok) { Write-Output 'FAIL: DIALOG OK NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
Invoke-Element $ok
Write-Output 'STEP: NEW GAME STARTED'

$board = Wait-ElementByClass $win 'BoardWidget' 6
if (-not $board) { Write-Output 'FAIL: BOARD WIDGET NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
$r = $board.Current.BoundingRectangle
$cell = [int]([Math]::Min($r.Width, $r.Height) - 40) / 14
$margin = 20

function Click-BoardCell($row, $col) {
    $x = $r.X + $margin + $col * $cell
    $y = $r.Y + $margin + $row * $cell
    Click-Screen $x $y
}

$moves = @(
    @(7, 7), @(8, 8), @(7, 8), @(8, 9), @(7, 9),
    @(8, 10), @(7, 10), @(8, 11), @(7, 11)
)

foreach ($m in $moves) {
    Click-BoardCell $m[0] $m[1]
    Start-Sleep -Milliseconds 300
    $labels = Get-LabelTexts $win
    Write-Output ("MOVE " + $m[0] + "," + $m[1] + " -> " + $labels)
}

$winMsg = Wait-ElementByName $root '游戏结束' 6
if ($winMsg) {
    $okMsg = Find-ButtonByName $winMsg @('OK', '确定')
    if ($okMsg) { Invoke-Element $okMsg }
    Write-Output 'STEP: WIN DIALOG CLOSED'
} else {
    Write-Output 'WARN: WIN DIALOG NOT DETECTED'
}

$afterWin = Get-LabelTexts $win
Write-Output ("AFTER_WIN_LABELS=" + $afterWin)

$newGame2 = Find-ButtonByName $win @('新游戏')
if ($newGame2) { Invoke-Element $newGame2 }
$dialog2 = Wait-ElementByName $root '新游戏' 6
if ($dialog2) {
    $ok2 = Find-ButtonByName $dialog2 @('OK', '确定')
    if ($ok2) { Invoke-Element $ok2 }
}
Write-Output 'STEP: RESTARTED'

Click-BoardCell 0 0
Start-Sleep -Milliseconds 300
$afterRestart = Get-LabelTexts $win
Write-Output ("AFTER_RESTART_MOVE=" + $afterRestart)

Click-BoardCell 0 0
Start-Sleep -Milliseconds 500
$warn = Wait-ElementByName $root '提示' 5
if ($warn) {
    $okWarn = Find-ButtonByName $warn @('OK', '确定')
    if ($okWarn) { Invoke-Element $okWarn }
    Write-Output 'STEP: OCCUPIED WARNING CLOSED'
} else {
    Write-Output 'WARN: OCCUPIED WARNING NOT DETECTED'
}

Stop-Process -Id $p.Id -Force
Write-Output 'HVH_SMOKE_TEST_DONE'
