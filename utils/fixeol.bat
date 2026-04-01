@ECHO OFF
PUSHD "%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf ".github/**/*"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf ".gitignore"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf ".gitattributes"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf "LICENSE"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf "*.(slnx|vcxproj|vcxproj.user|vcxproj.filters|def|txt|md)"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf "(cmake|demo|include|scripts|src|vcpkg)/**/*.(h|cpp|rc|def|slnx|vcxproj|vcxproj.user|vcxproj.filters|ps1|json|cmake|in|txt)"
powershell -ExecutionPolicy Bypass -File "%~dp0eolconverter.ps1" crlf "vcpkg/**/usage"
POPD
