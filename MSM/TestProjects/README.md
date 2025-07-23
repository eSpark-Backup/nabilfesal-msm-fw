Created a kit in QT Creator

Name : LicheePi QT Embedded
Device type : Generic Linux Device
Device: LicheePi-Zero(default for Generic Linux)
sysroot: /home/esparkdev/buildroot/output/target
compiler: 
    C: LinaroGCC
    C++ : LinaroG++
    Note: These are the names set in compilers tab in kit
QT version: Qt 5.15.14(host)



COMPILERS TAB:
NOTE: In buildroot menuconfig the toolchain is set to externel toolchain and Linaro ARM 2018.05 which gets downloaded and installed when 'make' in buildroot dir
Could be setup manually by downloading it and setting path when configuring kit in qtcreator
Manual
    C++
        Name : LinaroG++
        Compiler Path : /home/esparkdev/buildroot/output/host/bin/arm-linux-gnueabihf-g++
    C
        Name : LinaroGCC
        Compiler Path : /home/esparkdev/buildroot/output/host/bin/arm-linux-gnueabihf-gcc


DEBUGGERS :
    (defaults)

QBS :
    (defaults)
    Kit : LicheePi QT Embedded

CMake : 
    (defaults)