[Run]
Filename: "{app}\bin\MoonDeckBuddy.exe"; Description: "Enable autostart"; Flags: postinstall; Parameters: "--enable-autostart"
; Sunshine, Apollo and Vibepollo keep their apps in Program Files, so this needs admin rights
Filename: "{app}\bin\MoonDeckBuddy.exe"; Description: "Add MoonDeckStream to Sunshine, Apollo or Vibepollo"; Flags: postinstall shellexec runhidden waituntilterminated; Verb: "runas"; Parameters: "--register-stream-app"; Check: IsStreamHostInstalled
Filename: "{app}\bin\MoonDeckBuddy.exe"; Description: "Launch application"; Flags: postinstall nowait

[UninstallRun]
Filename: "{app}\bin\MoonDeckBuddy.exe"; Parameters: "--disable-autostart"; RunOnceId: "Cleanup"

[UninstallDelete]
Type: files; Name: "{app}\bin\moondeckbuddy.log"
Type: files; Name: "{app}\bin\moondeckstream.log"
