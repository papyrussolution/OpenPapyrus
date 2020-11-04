if not exist local.dl6 goto no_local
..\bin\dl600c-xp.exe /ob local.dl6
move local.bin ..\bin\ppexp.bin
:no_local
