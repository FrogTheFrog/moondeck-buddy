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
      if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonDeckBuddy', 'UninstallString', NsisUninstaller) then
      begin
        NsisUninstaller := RemoveQuotes(NsisUninstaller)
        if Exec(NsisUninstaller, Format('/S _?=%s', [ExtractFileDir(NsisUninstaller)]), '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
        begin
          if (ResultCode = 0) then
          begin
            DeleteFile(NsisUninstaller);
          end
          else begin
            MsgBox('Failed to uninstall older version. Please do so manually!', mbError, MB_OK);
            Abort();
          end;
        end
        else begin
          MsgBox('Failed to uninstall older version. Please do so manually!', mbError, MB_OK);
          Abort();
        end;
      end;
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    // Files to always delete:
    DeleteFile(ExpandConstant('{app}\bin\moondeckbuddy.log'));
    DeleteFile(ExpandConstant('{app}\bin\moondeckstream.log'));

    if MsgBox('Do you want to delete configuration files?', mbConfirmation, MB_YESNO) = IDYES
    then begin
      DeleteFile(ExpandConstant('{app}\bin\clients.json'));
      DeleteFile(ExpandConstant('{app}\bin\settings.json'));
    end;
  end;
end;