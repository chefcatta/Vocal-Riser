; Installer di Chef Riser per Windows (Inno Setup 6)
; Mette il VST3 nella cartella standard C:\Program Files\Common Files\VST3,
; quella che tutte le DAW controllano, e toglie le versioni vecchie.
; Si compila dalla CI con:  ISCC /DMyVersion=x.y.z /DSrcDir=<cartella Chef Riser.vst3> ChefRiser.iss

#ifndef MyVersion
  #define MyVersion "0.0.0"
#endif

[Setup]
AppId={{CBC9F12D-36DF-58A1-B690-A00543FB84C4}
AppName=Chef Riser
AppVersion={#MyVersion}
AppVerName=Chef Riser {#MyVersion}
AppPublisher=Chef Catta
VersionInfoVersion={#MyVersion}
VersionInfoCompany=Chef Catta
VersionInfoDescription=Chef Riser - installer
DefaultDirName={autopf}\Chef Catta\Chef Riser
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=ChefRiser-windows-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Chef Riser
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
it.FinishedLabel=Chef Riser e installato in C:\Program Files\Common Files\VST3.%n%nApri la DAW e fai una nuova scansione dei plugin.
en.FinishedLabel=Chef Riser has been installed in C:\Program Files\Common Files\VST3.%n%nOpen your DAW and rescan your plugins.

[InstallDelete]
; versioni precedenti (anche con il vecchio nome)
Type: filesandordirs; Name: "{commoncf64}\VST3\Chef Riser.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Vocal Riser.vst3"

[Files]
Source: "{#SrcDir}\*"; DestDir: "{commoncf64}\VST3\Chef Riser.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Chef Riser.vst3"
