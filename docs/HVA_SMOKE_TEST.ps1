Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win32Helper {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
}
public static class MouseSim2 {
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

function Select-RadioByName($parent, $name) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $name)
    $el = $parent.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
    if (-not $el) { return }
    try {
        $pattern = $el.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern)
        $pattern.Select()
    } catch {
        try {
            $invoke = $el.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
            $invoke.Invoke()
        } catch {}
    }
    Start-Sleep -Milliseconds 200
}

function Set-Foreground($el) {
    $handle = [IntPtr]$el.Current.NativeWindowHandle
    if ($handle -ne [IntPtr]::Zero) {
        [Win32Helper]::SetForegroundWindow($handle) | Out-Null
    }
}

function Click-Screen($x, $y) {
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point($x, $y)
    [MouseSim2]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    [MouseSim2]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
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

function Wait-LabelContains($win, $text, $timeoutSec) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
        $labels = Get-LabelTexts $win
        if ($labels.Contains($text)) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Try-SetEasyDifficulty($dialog) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::ComboBox)
    $combo = $dialog.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
    if (-not $combo) { return }
    try {
        $value = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        $value.SetValue('简单 - 快速思考')
    } catch {}
}

function Start-HvAGame($humanWhite) {
    $newGame = Find-ButtonByName $win @('新游戏')
    if ($newGame) { Invoke-Element $newGame }
    $dialog = Wait-ElementByName $root '新游戏' 6
    if (-not $dialog) { return $false }
    Select-RadioByName $dialog '人机对战'
    Try-SetEasyDifficulty $dialog
    if ($humanWhite) {
        Select-RadioByName $dialog '执白（后手）'
    }
    $ok = Find-ButtonByName $dialog @('OK', '确定')
    if (-not $ok) { return $false }
    Invoke-Element $ok
    Set-Foreground $win
    Start-Sleep -Milliseconds 400
    return $true
}

Start-Sleep -Seconds 2
$win = Wait-ElementByName $root '五子棋 - Gomoku' 10
if (-not $win) { Write-Output 'FAIL: MAIN WINDOW NOT FOUND'; Stop-Process -Id $p.Id -Force; exit 1 }
Set-Foreground $win
Write-Output 'STEP: WINDOW FOUND'

if (-not (Start-HvAGame $false)) {
    Write-Output 'FAIL: HVA GAME START BLACK FIRST'
    Stop-Process -Id $p.Id -Force
    exit 1
}
Write-Output 'STEP: HVA STARTED (HUMAN BLACK)'

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

$humanTurn1 = Wait-LabelContains $win '黑方回合' 10
Click-BoardCell 7 7
$backToHuman1 = Wait-LabelContains $win '黑方回合' 25
Click-BoardCell 8 7
$backToHuman2 = Wait-LabelContains $win '黑方回合' 25
$blackFirstPass = $humanTurn1 -and $backToHuman1 -and $backToHuman2
Write-Output ("HVA_BLACK_FIRST_PASS=" + $blackFirstPass)

if (-not (Start-HvAGame $true)) {
    Write-Output 'FAIL: HVA GAME START WHITE FIRST'
    Stop-Process -Id $p.Id -Force
    exit 1
}
Write-Output 'STEP: HVA STARTED (AI BLACK)'

$humanWhiteTurn = Wait-LabelContains $win '白方回合' 25
if ($humanWhiteTurn) {
    Click-BoardCell 8 8
}
$backToHumanWhite = Wait-LabelContains $win '白方回合' 25
$whiteSecondPass = $humanWhiteTurn -and $backToHumanWhite
Write-Output ("HVA_WHITE_SECOND_PASS=" + $whiteSecondPass)

Stop-Process -Id $p.Id -Force
Write-Output 'HVA_SMOKE_TEST_DONE'

if ($blackFirstPass -and $whiteSecondPass) {
    exit 0
} else {
    exit 1
}
