REM cl demo.cpp ftp.cpp sftp.cpp ssh.lib libeay32.lib ssleay32.lib zlib.lib /I"..\vcpkg\packages\libssh_x86-windows\include" /I"..\vcpkg\packages\openssl-windows_x86-windows\include" /I"..\vcpkg\packages\zlib_x86-windows\" /link /L"..\vcpkg\packages\libssh_x86-windows\lib" /L"..\vcpkg\packages\openssl-windows_x86-windows\lib" 
cl tosftp.cpp ftp.cpp sftp.cpp pbar.cpp user32.lib shell32.lib ssh.lib libeay32.lib ssleay32.lib zlib.lib /EHsc /I"..\vcpkg\packages\libssh_x86-windows\include" /I"..\vcpkg\packages\openssl-windows_x86-windows\include" /I"..\vcpkg\packages\zlib_x86-windows" -link -LIBPATH:..\vcpkg\packages\libssh_x86-windows\lib -LIBPATH:..\vcpkg\packages\openssl-windows_x86-windows\lib
