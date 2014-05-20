set ROOT=M:\src
set DIR=%ROOT%\mazinger\target
copy %ROOT%\mazingerHook\target\mazingerHook.dll %DIR%
rem start /min cmd /c "%ROOT%\mazinger\src\test\bin\debug" &
rem %DIR%\Mazinger.exe start %ROOT%\MazingerCompiler\target\test3.mzn
%DIR%\Mazinger.exe trace %ROOT%\MazingerCompiler\target\test3.mzn
