OutFile "Dx7interface.exe"
InstallDir "$PROGRAMFILES64\Dx7interface"
VIProductVersion "1.0.0.0"

!define APP_ICON "D:\gxinterface\package\usr\share\dx7interface\1.0.0\data\images\dx7interface.ico"
Icon "${APP_ICON}"

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "Installer"
  SetOutPath $INSTDIR

  WriteUninstaller "$INSTDIR\uninstall.exe"
  File /r /x ucrt64 "d:\gxinterface\package\*.*"

  CreateDirectory "$SMPROGRAMS\Dx7interface"
  CreateDirectory "$LOCALAPPDATA\gxinterface\"
  CreateShortcut "$SMPROGRAMS\Dx7interface\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  File "${APP_ICON}"
  CreateShortCut "$SMPROGRAMS\Dx7interface\Dx7interface.lnk" "$INSTDIR\usr\bin\gxinterface.exe" "-m usr\share\dx7interface\1.0.0\dx7interface-0.0.1.la" "$INSTDIR\dx7interface.ico" 0
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Dx7interface"
SectionEnd