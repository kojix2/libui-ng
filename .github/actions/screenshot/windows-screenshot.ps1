param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputFile
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class LibuiScreenshotWin32 {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxCount);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int command);

    [DllImport("dwmapi.dll")]
    public static extern int DwmGetWindowAttribute(
        IntPtr hWnd, int attribute, out RECT value, int size);

    public const int SW_RESTORE = 9;
    public const int DWMWA_EXTENDED_FRAME_BOUNDS = 9;
}
"@

$process = $null
$bitmap = $null
$graphics = $null

try {
    $absoluteAppPath = Join-Path $env:GITHUB_WORKSPACE $AppPath
    if (!(Test-Path -LiteralPath $absoluteAppPath)) {
        throw "Application not found: $absoluteAppPath"
    }

    $process = Start-Process -FilePath $absoluteAppPath -PassThru
    $processId = [uint32]$process.Id
    $mainWindow = [IntPtr]::Zero
    $fallbackWindow = [IntPtr]::Zero

    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        $callback = [LibuiScreenshotWin32+EnumWindowsProc]{
            param($hWnd, $lParam)

            $windowProcessId = [uint32]0
            [void][LibuiScreenshotWin32]::GetWindowThreadProcessId(
                $hWnd, [ref]$windowProcessId)
            if ($windowProcessId -ne $processId -or
                ![LibuiScreenshotWin32]::IsWindowVisible($hWnd)) {
                return $true
            }

            $title = New-Object System.Text.StringBuilder 256
            [void][LibuiScreenshotWin32]::GetWindowText($hWnd, $title, 256)
            if ($title.Length -eq 0) {
                return $true
            }

            if ($title.ToString() -notlike '*.exe') {
                $script:mainWindow = $hWnd
                return $false
            }
            if ($script:fallbackWindow -eq [IntPtr]::Zero) {
                $script:fallbackWindow = $hWnd
            }
            return $true
        }

        [void][LibuiScreenshotWin32]::EnumWindows(
            $callback, [IntPtr]::Zero)
        if ($mainWindow -ne [IntPtr]::Zero) {
            break
        }
        Start-Sleep -Milliseconds 500
    }

    if ($mainWindow -eq [IntPtr]::Zero) {
        $mainWindow = $fallbackWindow
    }
    if ($mainWindow -eq [IntPtr]::Zero) {
        throw "Could not find a visible window for PID $processId"
    }

    [void][LibuiScreenshotWin32]::ShowWindow(
        $mainWindow, [LibuiScreenshotWin32]::SW_RESTORE)
    [void][LibuiScreenshotWin32]::SetForegroundWindow($mainWindow)
    Start-Sleep -Seconds 1

    $rect = New-Object 'LibuiScreenshotWin32+RECT'
    $dwmResult = [LibuiScreenshotWin32]::DwmGetWindowAttribute(
        $mainWindow,
        [LibuiScreenshotWin32]::DWMWA_EXTENDED_FRAME_BOUNDS,
        [ref]$rect,
        [System.Runtime.InteropServices.Marshal]::SizeOf($rect))
    if ($dwmResult -ne 0 -and
        ![LibuiScreenshotWin32]::GetWindowRect($mainWindow, [ref]$rect)) {
        throw "Could not determine window bounds"
    }

    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "Invalid window bounds"
    }

    Add-Type -AssemblyName System.Drawing
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen(
        $rect.Left, $rect.Top, 0, 0,
        [System.Drawing.Size]::new($width, $height))
    $bitmap.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Png)

    if (!(Test-Path -LiteralPath $OutputFile)) {
        throw "Screenshot was not created: $OutputFile"
    }
}
catch {
    Write-Error $_.Exception.Message
    exit 1
}
finally {
    if ($graphics -ne $null) {
        $graphics.Dispose()
    }
    if ($bitmap -ne $null) {
        $bitmap.Dispose()
    }
    if ($process -ne $null -and !$process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
}
