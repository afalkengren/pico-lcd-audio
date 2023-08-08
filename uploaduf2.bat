set picotool="C:\picotool\build\picotool.exe"
set uf2file="%~dp0\build\main.uf2"
%picotool% reboot -f -u
timeout /t 2
%picotool% load -u -v -x %uf2file%