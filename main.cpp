#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "resource.h"

std::wstring rootDirectory;
std::wstring filePathScript;
std::wstring filePathSettings;
std::wstring filePathInstall;

std::vector<std::wstring> tempFiles;

std::wstring loadString(int id, int bufferSize = 512) {
	wchar_t buffer[bufferSize];
	LoadStringW(GetModuleHandle(NULL), id, buffer, sizeof(buffer));
	return std::wstring(buffer);
}

std::wstring createTempFile() {
	DWORD dwRet = 0;
	UINT uRet = 0;
	wchar_t tmpFileName[MAX_PATH];
	wchar_t tmpPathBuffer[MAX_PATH + 1];
	
	dwRet = GetTempPathW(MAX_PATH, tmpPathBuffer);
	if (dwRet == 0 || dwRet > MAX_PATH) return L"";
	
	uRet = GetTempFileNameW(tmpPathBuffer, L"STR", 0, tmpFileName);
	if (uRet == 0) return L"";
	
	// GetTempFileName returns and creates a .tmp file, so delete that and create a new .txt instead so system can open it with text editor after
	// efficient ;)
	// Why is it called "GetTempFileName" when it does more than that anyway?
	std::wstring fileName = std::wstring(tmpFileName);
	std::filesystem::path path = std::filesystem::path(fileName);
	std::filesystem::remove(path);
	fileName.pop_back();
	fileName.pop_back();
	fileName.pop_back();
	fileName.push_back(L't');
	fileName.push_back(L'x');
	fileName.push_back(L't');
	FILE* fp = _wfopen(fileName.c_str(), L"a+");
	std::fclose(fp);
	tempFiles.push_back(fileName);
	return fileName;
}

void deleteTempFiles() {
	if (!tempFiles.empty()) {
		for (int i = 0; i < tempFiles.size(); i++) {
			std::filesystem::path path = std::filesystem::path(tempFiles[i]);
			std::filesystem::remove(path);
		}
		tempFiles.clear();
	}
}

bool openLastTempFileInSystemEditor() {
	if (tempFiles.size() > 0) {
		ShellExecuteW(NULL, NULL, tempFiles[tempFiles.size() - 1].c_str(), NULL, NULL, SW_SHOW);
		return true;
	}
	else {
		return false;
	}
}

bool openTempStringInSystemEditor(std::wstring str) {
	std::wstring fileName = createTempFile();
	std::wofstream file = std::wofstream(fileName.c_str(), std::ios::binary);
	file.imbue(std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>));
	if (file.is_open()) {
		file << str;
		file.close();
		ShellExecuteW(NULL, NULL, fileName.c_str(), NULL, NULL, SW_SHOW);
		return true;
	}
	return false;
}

bool openTempStringInSystemEditorWrap(std::wstring str) {
	if (!openTempStringInSystemEditor(str)) {
		MessageBeep(MB_ICONERROR);
		MessageBoxW(NULL, loadString(IDS_ERR_TEMPOPEN).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
		return false;
	}
	return true;
}

std::wstring getControlValue(HWND hwnd, int id) {
	std::wstring output = L"";
	int len = GetWindowTextLengthW(GetDlgItem(hwnd, id)) + 1;
	if (len > 0) {
		wchar_t buffer[len];
		GetDlgItemTextW(hwnd, id, buffer, len);
		output = std::wstring(buffer);
	}
	return output;
}

void setControlValue(HWND hwnd, int id, std::wstring data) {
	SetDlgItemTextW(hwnd, id, data.c_str());
}

std::vector<std::wstring> getSelectedListBoxItems(HWND hwnd, int id) {
	int count = SendMessage(GetDlgItem(hwnd, id), LB_GETCOUNT, 0, 0);
	std::vector<std::wstring> items;
	for (int i = 0; i < count; i++) {
		if (SendMessage(GetDlgItem(hwnd, id), LB_GETSEL, i, 0) > 0) {
			wchar_t txt[SendMessage(GetDlgItem(hwnd, id), LB_GETTEXTLEN, i, 0) + 1];
			SendMessage(GetDlgItem(hwnd, id), LB_GETTEXT, i, (LPARAM)txt);
			items.push_back(std::wstring(txt));
		}
	}
	return items;
}

std::vector<std::wstring> getListBoxItems(HWND hwnd, int id) {
	int count = SendMessage(GetDlgItem(hwnd, id), LB_GETCOUNT, 0, 0);
	std::vector<std::wstring> items;
	for (int i = 0; i < count; i++) {
		wchar_t txt[SendMessage(GetDlgItem(hwnd, id), LB_GETTEXTLEN, i, 0) + 1];
		SendMessage(GetDlgItem(hwnd, id), LB_GETTEXT, i, (LPARAM)txt);
		items.push_back(std::wstring(txt));
	}
	return items;
}

void updateListBoxHorizontalScroll(HWND hwnd, int id) {
	std::vector<std::wstring> current = getListBoxItems(hwnd, id);
	if (!current.empty()) {
		int max = 0;
		for (int i = 0; i < current.size(); i++) {
			RECT rect;
			ZeroMemory(&rect, sizeof(rect));
			
			HDC hdc1 = GetDC(hwnd);
			HDC hdc2 = CreateCompatibleDC(hdc1);
			
			HFONT font1 = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
			HGDIOBJ font2 = SelectObject(hdc2, font1);
			
			DrawTextW(hdc2, current[i].data(), -1, &rect, DT_CALCRECT | DT_SINGLELINE | DT_NOCLIP);
			
			SelectObject(hdc2, font2);
			DeleteDC(hdc2);
			ReleaseDC(hwnd, hdc1);
			
			int out = (rect.right - rect.left) + (2 * GetSystemMetrics(SM_CXEDGE));
			if (out > max) max = out;
		}
		SendMessage(GetDlgItem(hwnd, id), LB_SETHORIZONTALEXTENT, (WPARAM)max, 0);
	}
	else {
		SendMessage(GetDlgItem(hwnd, id), LB_SETHORIZONTALEXTENT, (WPARAM)0, 0);
	}
}

void addListBoxItem(HWND hwnd, int id, std::wstring data) {
	if (data != L"ACTIVE" && data != L"INACTIVE" && data != L"INTERVAL") {
		std::vector<std::wstring> current = getListBoxItems(hwnd, id);
		if (!(!current.empty() && std::find(current.begin(), current.end(), data) != current.end())) {
			SendDlgItemMessageW(hwnd, id, LB_ADDSTRING, 0, (WPARAM)data.c_str());
		}
		updateListBoxHorizontalScroll(hwnd, id);
	}
}

void removeSelectedListBoxItems(HWND hwnd, int id) {
	int count = SendMessage(GetDlgItem(hwnd, id), LB_GETCOUNT, 0, 0);
	for (int i = count; i > -1; i--) {
		if (SendMessage(GetDlgItem(hwnd, id), LB_GETSEL, i, 0) > 0) {
			SendMessage(GetDlgItem(hwnd, id), LB_DELETESTRING, i, 0);
		}
	}
	updateListBoxHorizontalScroll(hwnd, id);
}

bool isChecked(HWND hwnd, int id) {
	int checked = SendMessage(GetDlgItem(hwnd, id), BM_GETCHECK, 0, 0);
	if (checked == BST_CHECKED) {
		return true;
	}
	return false;
}

std::wstring escapeDoubleQuotes(std::wstring in) {	
	std::wstring out = in;
	for (int i = 0; i < out.length(); i++) {
		if (out[i] == '"') {
			out.replace(i, 1, L"`\"");
			i++;
		}
	}
	return out;
}

std::wstring generatePowerShellScript(HWND hwnd) {
	std::wstring str = L"#Requires -RunAsAdministrator\n#Requires -Version 6.0\n\n";
	if (isChecked(hwnd, IDC_PROCESS_ACTIVE)) {
		std::vector<std::wstring> processes = getListBoxItems(hwnd, IDC_PROCESS_LISTBOX);
		if (processes.size() > 0) {
			for (int i = 0; i < processes.size(); i++) {
				std::wstring process = escapeDoubleQuotes(processes[i]);
				str += L"Get-Process -Name \"" + process + L"\" | Stop-Process -Force\n";
			}
		}
	}
	if (isChecked(hwnd, IDC_SERVICE_ACTIVE)) {
		std::vector<std::wstring> services = getListBoxItems(hwnd, IDC_SERVICE_LISTBOX);
		if (services.size() > 0) {
			str += L"$Services = Get-Service | ?{ ";
			for (int i = 0; i < services.size(); i++) {
				std::wstring service = escapeDoubleQuotes(services[i]);
				str += L"$_.Name -Like \"" + service + L"\" ";
				if (i < services.size() - 1) {
					str += L"-Or ";
				}
			}
			str += L"};\n";
			str += L"foreach ($Service in $Services) {\n";
			str += L"\tStop-Service -InputObject $Service -Force -NoWait;\n";
			str += L"\tRemove-Service -InputObject $Service;\n";
			str += L"}\n";
		}
	}
	if (isChecked(hwnd, IDC_TASK_ACTIVE)) {
		std::vector<std::wstring> tasks = getListBoxItems(hwnd, IDC_TASK_LISTBOX);
		if (tasks.size() > 0) {
			str += L"$Tasks = Get-ScheduledTask | ?{ ";
			for (int i = 0; i < tasks.size(); i++) {
				std::wstring task = escapeDoubleQuotes(tasks[i]);
				str += L"$_.TaskName -Like \"" + task + L"\" ";
				if (i < tasks.size() - 1) {
					str += L"-Or ";
				}
			}
			str += L"};\n";
			str += L"foreach ($Task in $Tasks) {\n";
			str += L"\tStop-ScheduledTask -InputObject $Task;\n";
			str += L"\tUnregister-ScheduledTask -InputObject $Task -Confirm:$false;\n";
			str += L"}\n";
		}
	}
	if (isChecked(hwnd, IDC_PATH_ACTIVE)) {
		std::vector<std::wstring> paths = getListBoxItems(hwnd, IDC_PATH_LISTBOX);
		if (paths.size() > 0) {
			str += L"Get-ChildItem -Path ";
			for (int i = 0; i < paths.size(); i++) {
				std::wstring path = escapeDoubleQuotes(paths[i]);
				str += L"\"" + path + L"\"";
				if (i < paths.size() - 1) {
					str += L", ";
				}
			}
			str += L" | Remove-Item -Recurse -Force;\n";
		}
	}
	if (isChecked(hwnd, IDC_REGISTRY_ACTIVE)) {
		std::vector<std::wstring> keys = getListBoxItems(hwnd, IDC_REGISTRY_LISTBOX);
		if (keys.size() > 0) {
			str += L"Get-Item -Path ";
			for (int i = 0; i < keys.size(); i++) {
				std::wstring key = escapeDoubleQuotes(keys[i]);
				str += L"\"Registry::" + key + L"\"";
				if (i < keys.size() - 1) {
					str += L", ";
				}
			}
			str += L" | Remove-Item -Recurse -Force;\n";
		}
	}
	return str;
}

bool generatePowerShellFile(HWND hwnd) {
	std::wofstream file = std::wofstream(filePathScript.c_str(), std::ios::binary);
	file.imbue(std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>));
	if (file.is_open()) {
		std::wstring str = generatePowerShellScript(hwnd);
		file << str;
		file.close();
		return true;
	}
	return false;
}

std::wstring generateTaskInstallScript(HWND hwnd) {
	std::wstring str;	
	std::wstring intervalString = getControlValue(hwnd, IDC_INTERVAL);
	std::wstring minutes;
	if (intervalString == loadString(IDS_INTERVAL1)) minutes = L"5";
	if (intervalString == loadString(IDS_INTERVAL2)) minutes = L"10";
	if (intervalString == loadString(IDS_INTERVAL3)) minutes = L"15";
	if (intervalString == loadString(IDS_INTERVAL4)) minutes = L"20";
	if (intervalString == loadString(IDS_INTERVAL5)) minutes = L"25";
	if (intervalString == loadString(IDS_INTERVAL6)) minutes = L"30";
	if (intervalString == loadString(IDS_INTERVAL7)) minutes = L"40";
	if (intervalString == loadString(IDS_INTERVAL8)) minutes = L"50";
	if (intervalString == loadString(IDS_INTERVAL9)) minutes = L"60";
	if (intervalString == loadString(IDS_INTERVAL10)) minutes = L"90";
	if (intervalString == loadString(IDS_INTERVAL11)) minutes = L"120";
	if (intervalString == loadString(IDS_INTERVAL12)) minutes = L"150";
	if (intervalString == loadString(IDS_INTERVAL13)) minutes = L"180";
	if (intervalString == loadString(IDS_INTERVAL14)) minutes = L"240";
	if (intervalString == loadString(IDS_INTERVAL15)) minutes = L"320";
	if (intervalString == loadString(IDS_INTERVAL16)) minutes = L"400";
	if (intervalString == loadString(IDS_INTERVAL17)) minutes = L"480";
	if (intervalString == loadString(IDS_INTERVAL18)) minutes = L"560";
	str += L"Stop-ScheduledTask -TaskName \"FalseControlTask\"\n";
	str += L"Unregister-ScheduledTask -TaskName \"FalseControlTask\" -Confirm:$false;\n";
	str += L"$a = New-ScheduledTaskAction -Execute \"pwsh.exe\" -Argument \'-ExecutionPolicy Bypass -File \"" + filePathScript + L"\"';\n";
	str += L"$t = New-ScheduledTaskTrigger -AtLogon;\n";
	str += L"$t.Repetition = (New-ScheduledTaskTrigger -Once -At \"12am\" -RepetitionInterval (New-TimeSpan -Minutes " + minutes + L")).repetition;\n";
	str += L"$s = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries;\n";
	str += L"$r = Register-ScheduledTask -TaskName \"FalseControlTask\" -Trigger $t -Action $a -User \"NT AUTHORITY\\SYSTEM\" -Settings $s;\n";
	str += L"Start-ScheduledTask \"FalseControlTask\";";
	return str;
}

bool generateTaskInstallFile(HWND hwnd) {
	std::wofstream file = std::wofstream(filePathInstall.c_str(), std::ios::binary);
	file.imbue(std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>));
	if (file.is_open()) {
		std::wstring str = generateTaskInstallScript(hwnd);
		file << str;
		file.close();
		return true;
	}
	return false;
}

std::wstring saveSection(HWND hwnd, int active, int listbox, int interval = -1) {
	std::wstring str;
	if (interval != -1) {
		std::wstring intervalString = getControlValue(hwnd, interval);
		std::wstring value;
		if (intervalString == loadString(IDS_INTERVAL1)) value = L"1";
		if (intervalString == loadString(IDS_INTERVAL2)) value = L"2";
		if (intervalString == loadString(IDS_INTERVAL3)) value = L"3";
		if (intervalString == loadString(IDS_INTERVAL4)) value = L"4";
		if (intervalString == loadString(IDS_INTERVAL5)) value = L"5";
		if (intervalString == loadString(IDS_INTERVAL6)) value = L"6";
		if (intervalString == loadString(IDS_INTERVAL7)) value = L"7";
		if (intervalString == loadString(IDS_INTERVAL8)) value = L"8";
		if (intervalString == loadString(IDS_INTERVAL9)) value = L"9";
		if (intervalString == loadString(IDS_INTERVAL10)) value = L"10";
		if (intervalString == loadString(IDS_INTERVAL11)) value = L"11";
		if (intervalString == loadString(IDS_INTERVAL12)) value = L"12";
		if (intervalString == loadString(IDS_INTERVAL13)) value = L"13";
		if (intervalString == loadString(IDS_INTERVAL14)) value = L"14";
		if (intervalString == loadString(IDS_INTERVAL15)) value = L"15";
		if (intervalString == loadString(IDS_INTERVAL16)) value = L"16";
		if (intervalString == loadString(IDS_INTERVAL17)) value = L"17";
		if (intervalString == loadString(IDS_INTERVAL18)) value = L"18";
		return L"INTERVAL\n" + value;
	}
	else {
		if (isChecked(hwnd, active)) {
			str += L"ACTIVE\n";
		}
		else {
			str += L"INACTIVE\n";
		}
		std::vector<std::wstring> items = getListBoxItems(hwnd, listbox);
		if (items.size() > 0) {
			for (int i = 0; i < items.size(); i++) {
				str += items[i] + L"\n";
			}
		}
	}
	return str;
}

bool saveSettings(HWND hwnd) {
	std::wstring str;
	str += saveSection(hwnd, IDC_PROCESS_ACTIVE, IDC_PROCESS_LISTBOX);
	str += saveSection(hwnd, IDC_SERVICE_ACTIVE, IDC_SERVICE_LISTBOX);
	str += saveSection(hwnd, IDC_TASK_ACTIVE, IDC_TASK_LISTBOX);
	str += saveSection(hwnd, IDC_PATH_ACTIVE, IDC_PATH_LISTBOX);
	str += saveSection(hwnd, IDC_REGISTRY_ACTIVE, IDC_REGISTRY_LISTBOX);
	str += saveSection(hwnd, -1, -1, IDC_INTERVAL);
	std::wofstream file = std::wofstream(filePathSettings.c_str(), std::ios::binary);
	file.imbue(std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>));
	if (file.is_open()) {
		file << str;
		file.close();
		return true;
	}
	return false;
}

bool loadSettings(HWND hwnd) {
	std::wifstream file = std::wifstream(filePathSettings.c_str(), std::ios::binary);
	std::wstringstream buffer;
	std::wstring str;
	file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
	if (file.is_open()) {
		buffer << file.rdbuf();
		file.close();
		str = buffer.str();
		
		int col = 0;
		std::wstring tmp = L"";
		for (int i = 0; i <= str.length(); i++) {
			if (str[i] == L'\n' || i == str.length()) {
				if (col == 6) {
					SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_SETCURSEL, (WPARAM)(std::stoi(tmp) - 1), 0);
				}
				if (tmp == L"ACTIVE" || tmp == L"INACTIVE" || tmp == L"INTERVAL") {
					col++;
				}
				
				int currentActive;
				int currentListBox;
				if (col == 1) {
					currentActive = IDC_PROCESS_ACTIVE;
					currentListBox = IDC_PROCESS_LISTBOX;
				}
				else if (col == 2) {
					currentActive = IDC_SERVICE_ACTIVE;
					currentListBox = IDC_SERVICE_LISTBOX;
				}
				else if (col == 3) {
					currentActive = IDC_TASK_ACTIVE;
					currentListBox = IDC_TASK_LISTBOX;
				}
				else if (col == 4) {
					currentActive = IDC_PATH_ACTIVE;
					currentListBox = IDC_PATH_LISTBOX;
				}
				else if (col == 5) {
					currentActive = IDC_REGISTRY_ACTIVE;
					currentListBox = IDC_REGISTRY_LISTBOX;
				}
				
				if (tmp == L"ACTIVE") {
					SendMessage(GetDlgItem(hwnd, currentActive), BM_SETCHECK, BST_CHECKED, 0);
				}
				else if (tmp == L"INACTIVE") {
					SendMessage(GetDlgItem(hwnd, currentActive), BM_SETCHECK, BST_UNCHECKED, 0);
				}
				else if (col != 6) {
					addListBoxItem(hwnd, currentListBox, tmp);
				}
				
				tmp = L"";
			}
			else {
				tmp += str[i];
			}
		}
		return true;
	}
	else {
		return false;
	}
}

std::wstring runProgram(std::wstring cmd, bool output, int variation = 0) {
	/* TODO: This entire function is garbage, it shouldn't need any temp files in the first place.
			 It also really likes to break a lot for some various cryptic reasons.
			 It's just taped together at this point. */
	std::wstring out = L"";
	STARTUPINFOW si = {sizeof(si)};
	PROCESS_INFORMATION pi;

	std::wstring fileName = createTempFile();
	std::wstring cmdString;
	if (variation == 0) {
		cmdString = L"cmd.exe /C " + cmd;
		if (output) {
			cmdString += L" > \"" + fileName + L"\"";
		}
	}
	else if (variation == 1) {
		cmdString = L"pwsh.exe -Command $PSDefaultParameterValues['Out-File:Width'] = 2000; " + cmd;
		if (output) {
			cmdString += L" | Out-File -Encoding 'utf8' -FilePath \"" + fileName + L"\"";
		}
	}
	else if (variation == 2) {
		cmdString = L"pwsh.exe -Command \"" + cmd;
		if (output) {
			cmdString += L" | Out-File -Encoding 'utf8' -NoNewline -FilePath '" + fileName + L"'";
		}
		cmdString += L"\"";
	}
	else if (variation == 3) {
		cmdString = cmd;
	}
	if (CreateProcessW(NULL, cmdString.data(), NULL, NULL, false, CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		WaitForSingleObject(pi.hProcess, 5000);
		if (output) {
			std::wifstream file = std::wifstream(fileName.c_str(), std::ios::binary);
			std::wstringstream buffer;
			if (file.is_open()) {
				buffer << file.rdbuf();
				file.close();
				out = buffer.str();
			}
			else {
				MessageBoxW(NULL, loadString(IDS_ERR_TEMPOPEN).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
			}
		}
	}
	else {
		MessageBoxW(NULL, loadString(IDS_ERR_CREATEPROCESS).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
	}	

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return out;
}

std::wstring runPowerShell(std::wstring cmd, bool output) {
	return runProgram(cmd, output, 1);
}

std::wstring runPowerShell2(std::wstring cmd, bool output) {
	return runProgram(cmd, output, 2);
}

bool isTaskInstalled() {
	// TODO: Improve this awful abomination, it also sometimes doesn't return like anything at all whyyyyyy
	std::wstring installed = runPowerShell2(L"Get-ScheduledTask -TaskName \"FalseControlTask\" | Measure-Object | Format-List -Property \"Count\" | Out-String | % { $_.Trim() }", true);
	if (installed == L"Count : 1") {
		return true;
	}
	else {
		return false;
	}
}

bool updateInstalledStatus(HWND hwnd) {	
	if (isTaskInstalled()) {
		SendMessage(GetDlgItem(hwnd, IDC_STATUS), WM_SETTEXT, 0, (LPARAM)loadString(IDS_INSTALLED).c_str());
		return true;
	}
	else {
		SendMessage(GetDlgItem(hwnd, IDC_STATUS), WM_SETTEXT, 0, (LPARAM)loadString(IDS_UNINSTALLED).c_str());
		return false;
	}
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_INITDIALOG: {
			SendMessage(GetDlgItem(hwnd, IDC_PROCESS_EDIT), EM_SETLIMITTEXT, (WPARAM)1024, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SERVICE_EDIT), EM_SETLIMITTEXT, (WPARAM)1024, 0);
			SendMessage(GetDlgItem(hwnd, IDC_TASK_EDIT), EM_SETLIMITTEXT, (WPARAM)1024, 0);
			SendMessage(GetDlgItem(hwnd, IDC_PATH_EDIT), EM_SETLIMITTEXT, (WPARAM)1024, 0);
			SendMessage(GetDlgItem(hwnd, IDC_REGISTRY_EDIT), EM_SETLIMITTEXT, (WPARAM)1024, 0);
			
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL1).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL2).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL3).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL4).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL5).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL6).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL7).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL8).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL9).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL10).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL11).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL12).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL13).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL14).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL15).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL16).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL17).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_INTERVAL18).c_str());
			
			SendMessage(GetDlgItem(hwnd, IDC_INTERVAL), CB_SETCURSEL, (WPARAM)5, 0);
			loadSettings(hwnd);
			updateInstalledStatus(hwnd);
			return 1;
			break;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				
				/*
					1st column - Process names
				*/
				
				case IDC_PROCESS_ADD: {
					std::wstring str = getControlValue(hwnd, IDC_PROCESS_EDIT);
					if (str.length() > 0) {
						addListBoxItem(hwnd, IDC_PROCESS_LISTBOX, str);
						setControlValue(hwnd, IDC_PROCESS_EDIT, L"");
					}
					break;
				}
				case IDC_PROCESS_REMOVE: {
					removeSelectedListBoxItems(hwnd, IDC_PROCESS_LISTBOX);
					break;
				}
				
				/*
					2nd column - Service names
				*/
				
				case IDC_SERVICE_ADD: {
					std::wstring str = getControlValue(hwnd, IDC_SERVICE_EDIT);
					if (str.length() > 0) {
						addListBoxItem(hwnd, IDC_SERVICE_LISTBOX, str);
						setControlValue(hwnd, IDC_SERVICE_EDIT, L"");
					}
					break;
				}
				case IDC_SERVICE_REMOVE: {
					removeSelectedListBoxItems(hwnd, IDC_SERVICE_LISTBOX);
					break;
				}
				
				/*
					3rd column - Scheduled tasks
				*/
				
				case IDC_TASK_ADD: {
					std::wstring str = getControlValue(hwnd, IDC_TASK_EDIT);
					if (str.length() > 0) {
						addListBoxItem(hwnd, IDC_TASK_LISTBOX, str);
						setControlValue(hwnd, IDC_TASK_EDIT, L"");
					}
					break;
				}
				case IDC_TASK_REMOVE: {
					removeSelectedListBoxItems(hwnd, IDC_TASK_LISTBOX);
					break;
				}
				
				/*
					4th column - File/Folder paths
				*/
				
				case IDC_PATH_ADD: {
					std::wstring str = getControlValue(hwnd, IDC_PATH_EDIT);
					if (str.length() > 0) {
						addListBoxItem(hwnd, IDC_PATH_LISTBOX, str);
						setControlValue(hwnd, IDC_PATH_EDIT, L"");
					}
					break;
				}
				case IDC_PATH_REMOVE: {
					removeSelectedListBoxItems(hwnd, IDC_PATH_LISTBOX);
					break;
				}
				
				/*
					5th column - Registry keys
				*/
				
				case IDC_REGISTRY_ADD: {
					std::wstring str = getControlValue(hwnd, IDC_REGISTRY_EDIT);
					if (str.length() > 0) {
						addListBoxItem(hwnd, IDC_REGISTRY_LISTBOX, str);
						setControlValue(hwnd, IDC_REGISTRY_EDIT, L"");
					}
					break;
				}
				case IDC_REGISTRY_REMOVE: {
					removeSelectedListBoxItems(hwnd, IDC_REGISTRY_LISTBOX);
					break;
				}
				
				/*
					Bottom bar
				*/
				
				case IDC_TEST: {
					openTempStringInSystemEditorWrap(generatePowerShellScript(hwnd));
					openTempStringInSystemEditorWrap(generateTaskInstallScript(hwnd));
					break;
				}
				
				case IDC_INSTALL: {
					bool failed = false;
					if (std::FILE* fp = _wfopen(filePathSettings.c_str(), L"r")) { 
						std::fclose(fp);
					}
					else {
						MessageBoxW(NULL, loadString(IDS_FIRSTWARNING).c_str(), loadString(IDS_WARNING).c_str(), MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SYSTEMMODAL);
						FILE* fp2 = _wfopen(filePathSettings.c_str(), L"a+");
						std::fclose(fp2);
						break;
					}
					if (saveSettings(hwnd)) {
						if (generatePowerShellFile(hwnd)) {
							if (generateTaskInstallFile(hwnd)) {
								runProgram(L"pwsh.exe -ExecutionPolicy Bypass -File \"" + filePathInstall + L"\"", false, 3);
								if (updateInstalledStatus(hwnd)) {
									MessageBoxW(NULL, loadString(IDS_INSTALL_SUCCESS).c_str(), loadString(IDS_SUCCESS).c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SYSTEMMODAL);
								}
								else {
									failed = true;
								}
							}
							else {
								failed = true;
							}
						}
						else {
							failed = true;
						}	
					}
					else {
						failed = true;
					}
					if (failed) {
						MessageBoxW(NULL, loadString(IDS_INSTALL_FAIL).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
					}
					break;
				}
				
				case IDC_UNINSTALL: {
					runPowerShell(L"Stop-ScheduledTask -TaskName \"FalseControlTask\"; Unregister-ScheduledTask -TaskName \"FalseControlTask\" -Confirm:$false;", false);
					if (!updateInstalledStatus(hwnd)) {
						MessageBoxW(NULL, loadString(IDS_UNINSTALL_SUCCESS).c_str(), loadString(IDS_SUCCESS).c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SYSTEMMODAL);
					}
					else {
						MessageBoxW(NULL, loadString(IDS_UNINSTALL_SUCCESS).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
					}
					break;
				}
				
				/*
					Menu
				*/
				
				case IDR_ABOUT: {
					MessageBoxW(NULL, loadString(IDS_ABOUT_TEXT).c_str(), loadString(IDS_ABOUT_HEADER).c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SYSTEMMODAL);
					break;
				}
				case IDR_LIST_TASKS: {
					runPowerShell(L"Get-ScheduledTask | Format-Table -AutoSize", true);
					openLastTempFileInSystemEditor();
					break;
				}
				case IDR_LIST_SERVICES: {
					runPowerShell(L"Get-Service | Format-Table -AutoSize", true);
					openLastTempFileInSystemEditor();
					break;
				}
			}
			return 1;
			break;
		}
		case WM_CLOSE: {
			deleteTempFiles();
			DestroyWindow(hwnd);
			return 1;
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 1;
			break;
		}
	}
	return 0;
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, INT nCmdShow) {
	MSG msg;
	BOOL bRet;
	
	std::wstring powershell = runProgram(L"pwsh.exe -v", true);
	if (powershell.length() == 0) {
		MessageBoxW(NULL, loadString(IDS_ERR_POWERSHELL).c_str(), loadString(IDS_ERROR).c_str(), MB_OK | MB_ICONSTOP | MB_TOPMOST | MB_SYSTEMMODAL);
		deleteTempFiles();
		return 1;
	}
	
	PWSTR pathProgramFiles;
	SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &pathProgramFiles);
	rootDirectory = std::wstring(pathProgramFiles) + L"\\FalseControl\\";
	std::filesystem::create_directory(rootDirectory);
	
	filePathSettings = rootDirectory + L"FalseControl.cfg";
	filePathScript = rootDirectory + L"FalseControl.ps1";
	filePathInstall = rootDirectory + L"FalseControlInstall.ps1";
	
	HWND hwnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
	ShowWindow(hwnd, nCmdShow);
	
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) { 
		if (bRet == -1) {
			return 1;
		}
		else if (!TranslateAccelerator(hwnd, NULL, &msg) && !IsDialogMessage(hwnd, &msg)) { 
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} 
	return 0;
}