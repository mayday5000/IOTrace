** I O T R A C E **

Pin Tool el cual permite visualizar el Call Address y el Ret Address de las llamadas realizadas a funciones de IO de libc.
Muestra tanto las funciones en la imagen principal del programa como en las imagenes de otras librerias.

Actualmente a modo de prueba solo soportadas las siguientes funciones:
----------------------------------------------------------------------
fopen
fseek
fread
fgets
fclose
fwrite


Probado en:
-----------
Arquitectura: x86_64
OS: Linux kali 4.9.0-kali2-amd64 #1 SMP Debian 4.9.10-1kali1 (2017-02-20) x86_64 GNU/Linux


Compilacion de programa de prueba:
----------------------------------

g++ -g -o sort_match sort_match.cpp



pin -t iotrace.so -- ./sort_match

[+] Id: 18 - Nombre: damian

[+] Id: 2 - Nombre: ariel

[+] Id: 51 - Nombre: prueba



ls -lartF

total 2436

drwxrwxr-x+ 6 ec2-user ec2-user    4096 Jan 29 02:53 ../

-rwxr-xr-x+ 1 ec2-user ec2-user   21976 Jan 29 02:54 sort_match*

-rwxr-xr-x+ 1 ec2-user ec2-user 2433784 Jan 29 02:54 iotrace.so*

-rw-r--r--+ 1 ec2-user ec2-user    7078 Jan 29 02:57 sort_match.cpp

-rw-r-----+ 1 ec2-user ec2-user    6785 Jan 29 02:57 iotrace.cpp

-rw-r--r--+ 1 ec2-user ec2-user      56 Jan 29 03:05 in.txt

drwxrwxr-x+ 2 ec2-user ec2-user    4096 Jan 29 03:05 ./

-rw-rw-r--+ 1 ec2-user ec2-user    1475 Jan 29 03:05 filetrace.txt



cat filetrace.txt 

              Procedure               Image               Call Address              Ret Address         Ins Count        Ret Mnemonic
=========================================================================================================================================

         _IO_file_fopen           libc.so.6             0x7fad508427a0           0x7fad50842825                58            RET_NEAR
                  fseek           libc.so.6             0x7fad5083e4d0           0x7fad5083e5a2                39            RET_NEAR
                fopen64           libc.so.6             0x7fad50836850           0x7fad50836850                 2                 JMP
                  fgets           libc.so.6             0x7fad508365c0           0x7fad508366cc               384            RET_NEAR
                 fclose           libc.so.6             0x7fad50835e60           0x7fad50835f14                75            RET_NEAR
                 fclose          sort_match             0x561c9d75ba10           0x561c9d75ba10                 3                 JMP
                  fgets          sort_match             0x561c9d75b9c0           0x561c9d75b9c0                 8                 JMP
                  fopen          sort_match             0x561c9d75b9b0           0x561c9d75b9b0                 3                 JMP
                  fseek          sort_match             0x561c9d75b970           0x561c9d75b970                 3                 JMP
ec2-user@kali:~/RevEng/pin-3.4-97438-gf90d1f746-gcc-linux/source/tools/ManualExamples/MyPinTools/iotrace$ 

