@echo off
::This file was created automatically by CrossIDE to compile with C51.
C:
cd "\Users\pamel\OneDrive\Desktop\School\CrossIDE\09_Capacitor_Reader_MK\"
"C:\Program Files\CrossIDE\Call51\Bin\c51.exe" --use-stdout  "C:\Users\pamel\OneDrive\Desktop\School\CrossIDE\09_Capacitor_Reader_MK\main.c"
if not exist hex2mif.exe goto done
if exist main.ihx hex2mif main.ihx
if exist main.hex hex2mif main.hex
:done
echo done
echo Crosside_Action Set_Hex_File C:\Users\pamel\OneDrive\Desktop\School\CrossIDE\09_Capacitor_Reader_MK\main.hex
