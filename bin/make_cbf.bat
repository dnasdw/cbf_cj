PUSHD "%~dp0"
CHCP 65001
RD /S /Q temp
MD temp
FOR /R font %%I IN (*.bcfnt) DO (
tools\bcfnt2charset "%%~I" "temp\%%~nI.txt" || PAUSE
tools\charset2xlor "temp\%%~nI.txt" "tools\ctr_FontConverter\xlor\%%~nI.xlor" || PAUSE
tools\ctr_FontConverter\ctr_FontConverterConsole.exe -i bcfnt -if "%%~I" -o image -of "temp\%%~nI.bmp" -oo "tools\ctr_FontConverter\xlor\%%~nI.xlor" || PAUSE
tools\ctr_FontConverter\ctr_FontConverterConsole.exe -i image -if "temp\%%~nI.bmp" -io "tools\ctr_FontConverter\xlor\%%~nI.xlor" -ic A4 -o bcfnt -of "temp\%%~nI.bcfnt" -os 512 -oa  || PAUSE
DEL "tools\ctr_FontConverter\xlor\%%~nI.xlor"
)
tools\mergecharset temp\cbf_cj.txt temp\cbf_std.txt temp\cbf_zh-Hans-CN.txt temp\cbf_zh-Hant-TW.txt || PAUSE
Charset2Bitmap temp\cbf_cj.txt temp\cbf_cj.bmp tools\XHei_Microsoft-Mono.TTC 0 14 0 0.4 0 0 0 0 1 || PAUSE
MergeResizeBitmap temp\cbf_cj.txt temp\cbf_cj.bmp temp\cbf_std.txt temp\cbf_std.bmp temp\cbf_zh-Hans-CN.txt temp\cbf_zh-Hans-CN.bmp temp\cbf_zh-Hant-TW.txt temp\cbf_zh-Hant-TW.bmp || PAUSE
tools\charset2xlor temp\cbf_cj.txt tools\ctr_FontConverter\xlor\cbf_cj.xlor || PAUSE
tools\ctr_FontConverter\ctr_FontConverterConsole.exe -i image -if temp\cbf_cj.bmp -io tools\ctr_FontConverter\xlor\cbf_cj.xlor -ic A4 -o bcfnt -of temp\cbf_cj.bcfnt -os 512 -oa  || PAUSE
DEL tools\ctr_FontConverter\xlor\cbf_cj.xlor
RD /S /Q cj
MD cj
MOVE temp\cbf_cj.bcfnt cj\cbf_cj.bcfnt
MOVE temp\cbf_std.bcfnt cj\cbf_std.bcfnt
MOVE temp\cbf_zh-Hans-CN.bcfnt cj\cbf_zh-Hans-CN.bcfnt
RD /S /Q temp
POPD
