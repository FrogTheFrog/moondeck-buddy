[Run]
Filename: "{app}\bin\MoonDeckBuddy.exe"; Description: "Enable autostart"; Flags: postinstall; Parameters: "--enable-autostart"
Filename: "{app}\bin\MoonDeckBuddy.exe"; Description: "Launch application"; Flags: postinstall nowait

[UninstallRun]
Filename: "{app}\bin\MoonDeckBuddy.exe"; Parameters: "--disable-autostart"; RunOnceId: "Cleanup"

[UninstallDelete]
Type: files; Name: "{app}\bin\moondeckbuddy.log"
Type: files; Name: "{app}\bin\moondeckstream.log"
