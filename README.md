## FalseControl

![FalseControl Preview][preview]

`FalseControl` is a small Windows utility for keeping your computer clean of unwanted services, files and programs. The program works by creating its own scheduled task that periodically executes a generated PowerShell script of doom on login and on interval. Finally the need to create a PowerShell script and creating a task for it manually is over. The program is divided into 5 different columns, each having its own purpose. By invoking the power of `NT AUTHORITY/SYSTEM`, everything inside these 5 columns will be destroyed. Since the input is fed into PowerShell, wildcard filtering is possible (be careful!). The input is case-insensitive. The columns are as follows:
* Processes - Process name. Killed. Don't put ".exe" at the end. Example:
    ```sh
    explorer
    7zFM
    ETDCtrl
    ETDCtrlHelper
    ```
* Services - Service name. Stopped and deleted. Example:
    ```sh
    HpTouchpointAnalyticsService
    HPSysInfoCap
    HPPrintScanDoctorService
    HPNetworkCap
    HPDiagsCap
    HPAppHelperCap
    ```
* Scheduled Tasks - Task name. Stopped and deleted. Example:
    ```sh
    MicrosoftEdge*
    ```
* Files/Folders - Full path. Deleted recursively. Example:
    ```sh
    C:\te* <- Target everything beginning with "te"
    C:\test\ <- Target entire folder, folder is deleted
    C:\test\* <- Target contents of the folder, folder is kept
    C:\WINDOWS\System32\DriverStore\FileRepository\hpcustom*
    C:\WINDOWS\System32\DriverStore\FileRepository\hpanalytics*
    C:\Windows\System32\CompatTelRunner.exe
    ```
* Registry Keys - Full path. Deleted recursively. Example:
    ```sh
    HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\HP
    HKEY_LOCAL_MACHINE\SOFTWARE\HP\Bridge
    HKEY_LOCAL_MACHINE\SOFTWARE\Hewlett-Packard
    ```

The order of the destruction follows the columns.

The config files are stored in `C:\Program Files\FalseControl\` to prevent regular users from modifying them. You can backup the `.cfg` file to transfer settings. To delete all traces of the program, click the `Uninstall` button first if you already have it installed, then delete the config folder.

There are also a few useful commands in the `Utility` menu.

This is not an anti-malware solution and will not remove actual viruses from your computer.

Currently available in English and Czech languages.

## Be extra careful!!!

**Please keep in mind how destructive this program can potentially be when using it. There is no way to undo. To prevent tears, click the _"Test"_ button before installing and vet the generated scripts for yourself before installing. The program doesn't prevent you from putting nonsense or something dangerous to the input boxes.**

Anyone wanna try out putting `*` in all 5 columns?

## Requirements

* Windows 11+ _(maybe 10? not tested)_
* [PowerShell 6.0+](https://learn.microsoft.com/en-us/powershell/scripting/install/install-powershell-on-windows)

## How to compile

```sh
windres -i resource.rc -o resource.o
g++ -c main.cpp -o main.o -std=c++23 -luuid -mwindows -municode -static -Os -s
g++ resource.o main.o -o "FalseControl.exe" -std=c++23 -luuid -mwindows -municode -static -Os -s
```

## Use cases

* HP users can finally delete zombie services that keep coming back on every update automatically. (sorry it won't fix the physical hinge problem [or complete lack of it problem])
* Clean your temporary folders periodically.
* You can install and immediately uninstall to do something only once, like killing some processes to free up resources.
* You are forced to use Windows because if you try to install Linux it suspiciously makes your computer unable to measure temperatures correctly when going to sleep mode causing it to get more and more hot without stopping to the point when touching the shell burns your fingers and it becomes an actual fire hazard. How convenient for Microsoft that it never happens on Windows.

## Limitations

* If something is invulnerable to the power of `NT AUTHORITY/SYSTEM`, this program won't help you. In that case, the only viable and humane solution to your problem is to throw your computer out of the window for peace of mind.
* You can't delete individual values in registry, only whole folders (keys).
* The program hangs for a second when it calls PowerShell. This is normal (sync coding).

## Known issues

* The program needs testing.
    * Does it work on Windows 10?
    * Only tested on the NTFS file system. Don't know how will it behave on others. Didn't test.
    * I didn't test if the script can hang on service or task stopping, it didn't happen for me.
* The code is bad. I hope newer PowerShell versions don't change much so it doesn't break.

_It's called FalseControl since it really only gives you a false sense of control. In reality, you shouldn't need this kind of program in the first place. But what can you do..._

[preview]: FalseControl.png