Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$exe = 'D:\gomoku\build\Gomoku.exe'
$p = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 2

$root = [System.Windows.Automation.AutomationElement]::RootElement
$nameCond = New-Object System.Windows.Automation.PropertyCondition(
    [System.Windows.Automation.AutomationElement]::NameProperty,
    '五子棋 - Gomoku')

$win = $null
for ($i = 0; $i -lt 30; $i++) {
    $win = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $nameCond)
    if ($win) { break }
    Start-Sleep -Milliseconds 300
}

if (-not $win) {
    Write-Output 'NO_WINDOW'
    Stop-Process -Id $p.Id -Force
    exit 1
}

$r = $win.Current.BoundingRectangle
Write-Output ("WIN_CLASS=" + $win.Current.ClassName)
Write-Output ("WIN_RECT=" + [int]$r.X + "," + [int]$r.Y + "," + [int]$r.Width + "," + [int]$r.Height)

$all = $win.FindAll(
    [System.Windows.Automation.TreeScope]::Descendants,
    [System.Windows.Automation.Condition]::TrueCondition)

foreach ($el in $all) {
    try {
        $cr = $el.Current.BoundingRectangle
        Write-Output (
            "ELEM|" +
            $el.Current.ControlType.ProgrammaticName + "|" +
            $el.Current.ClassName + "|" +
            $el.Current.Name + "|" +
            [int]$cr.X + "," + [int]$cr.Y + "," + [int]$cr.Width + "," + [int]$cr.Height)
    } catch {}
}

Stop-Process -Id $p.Id -Force
