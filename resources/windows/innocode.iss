function GetPreviousAppDir(): string;
var InstallPath: string;
var RegPath: string;
begin
  RegPath := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonDeckBuddy_is1';
  // Check the HKCU (user-level) registry for a previous installation
  if RegQueryStringValue(HKCU, RegPath, 'Inno Setup: App Path', InstallPath) then
  begin
    Result := RemoveQuotes(InstallPath);
  end
  // If not found, check the HKLM (system-level) registry for previous installation with admin rights
  else if RegQueryStringValue(HKLM, RegPath, 'Inno Setup: App Path', InstallPath) then
  begin
    Result := RemoveQuotes(InstallPath);
  end
  else
  begin
    // If neither registry key is found, return an empty string
    Result := '';
  end;
end;

function InitializeSetup(): Boolean;
var PreviousAppDir: string;
var ResultCode: Integer;
begin
  PreviousAppDir := GetPreviousAppDir();
  if PreviousAppDir <> '' then
  begin
    // Return code does not matter as the AppMutex be a fallback
    Exec(Format('%s\bin\MoonDeckBuddy.exe', [PreviousAppDir]), '--close-all', '', SW_HIDE, ewWaitUntilTerminated, ResultCode)
  end;
  
  Result := True
end; 

procedure CurStepChanged(CurStep: TSetupStep);
var Version: String;
var ResultCode: Integer;
var NsisUninstaller: string;
begin
  if CurStep = ssInstall then
  begin
    if (GetVersionNumbersString(ExpandConstant('{app}\bin\MoonDeckBuddy.exe'), Version)) then
    begin
      Log(Format('Installed version: %s', [Version]));
    end
    else begin
      if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonDeckBuddy', 'UninstallString', NsisUninstaller) then
      begin
        NsisUninstaller := RemoveQuotes(NsisUninstaller)
        if Exec(NsisUninstaller, Format('/S _?=%s', [ExtractFileDir(NsisUninstaller)]), '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
        begin
          if (ResultCode = 0) then
          begin
            DeleteFile(NsisUninstaller);
          end
          else
          begin
            MsgBox('Failed to uninstall older version. Please do so manually!', mbError, MB_OK);
            Abort();
          end;
        end
        else
        begin
          MsgBox('Failed to uninstall older version. Please do so manually!', mbError, MB_OK);
          Abort();
        end;
      end;
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var ResultCode: Integer;
begin
  if CurUninstallStep = usAppMutexCheck then
  begin
    // Return code does not matter as the AppMutex be a fallback
    Exec(ExpandConstant('{app}\bin\MoonDeckBuddy.exe'), '--close-all', '', SW_HIDE, ewWaitUntilTerminated, ResultCode)
  end
  else if CurUninstallStep = usUninstall then
  begin
    if SuppressibleMsgBox('Do you want to remove settings?', mbConfirmation, MB_YESNO, IDYES) = IDYES then
    begin
      DeleteFile(ExpandConstant('{app}\bin\clients.json'));
      DeleteFile(ExpandConstant('{app}\bin\settings.json'));
    end;
  end;
end;