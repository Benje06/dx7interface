OutFile "Dx7interface.exe"
InstallDir "$PROGRAMFILES64\Dx7interface"
VIProductVersion "1.0.0.0"

Section "Installer"
  SetOutPath $INSTDIR
  File /r "d:\gxinterface\package\gxinterface\*.*"
  CreateShortCut "$SMPROGRAMS\Dx7interface.lnk" "$INSTDIR\bin\gxinterface.exe" "-m $INSTDIR \share\dx7interface\1.0.0\dx7interface-0.0.1.la"
SectionEnd
