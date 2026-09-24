QNX GAME SERVER
===============

This project is a real-time multiplayer game server developed in C for QNX Neutrino 8.0. The server is designed to run on a Raspberry Pi 4 using the AArch64 architecture.


1. PROJECT STRUCTURE
--------------------

The project contains the following important directories and files:

QNX_GAME_SERVER/

    src/
        Main server source code and other server-related source files.

    build/
        aarch64/
            QNX_GAME_SERVER
            Compiled server binary.

    Makefile
        Build configuration used to compile the complete server.

    README.md
        Project documentation.


2. SOURCE CODE
--------------

The main source code of the server is located inside the "src" directory.

All the main server functionalities and code modules are maintained inside this directory.

If you need to modify the server functionality, the required source files should be edited inside the "src" directory.

Example:

QNX_GAME_SERVER/src/


3. SERVER BINARY
----------------

After compiling the project, the executable server binary will be generated inside the build directory.

The binary location is:

QNX_GAME_SERVER/build/aarch64/QNX_GAME_SERVER

The file "QNX_GAME_SERVER" inside the "aarch64" directory is the compiled executable binary of the server.


4. IMPORTANT: WORKING DIRECTORY
-------------------------------

When compiling the complete project, you must be inside the main QNX_GAME_SERVER directory.

You should enter the project directory first:

cd QNX_GAME_SERVER

Only after entering this directory should you execute the make commands.


5. CLEANING THE PREVIOUS BUILD
------------------------------

Before compiling the project, always clean the previous build.

Use the following command:

make clean

Cleaning the previous build is important because an older compiled binary may otherwise be used.

Whenever you make changes to the source code, it is recommended to clean the previous build before compiling again.


6. COMPILING THE COMPLETE PROJECT
---------------------------------

After cleaning the previous build, compile the complete project using:

make

The "make" command reads the Makefile and compiles the required source files.

The compiled binary will be generated at:

build/aarch64/QNX_GAME_SERVER


7. RECOMMENDED BUILD PROCESS
----------------------------

Every time you modify the server source code, follow these steps:

Step 1:
Enter the QNX_GAME_SERVER directory.

cd QNX_GAME_SERVER

Step 2:
Clean the previous build.

make clean

Step 3:
Compile the complete project.

make

This ensures that the latest source-code changes are included in the newly generated binary.


8. LOCATING THE SERVER BINARY
-----------------------------

After successfully compiling the project, the server binary can be found at:

QNX_GAME_SERVER/build/aarch64/QNX_GAME_SERVER

The "QNX_GAME_SERVER" file in this location is the executable binary.


9. RUNNING THE SERVER
---------------------

To execute the server, navigate to the directory containing the binary.

From the QNX_GAME_SERVER directory, use:

cd build/aarch64

Then execute the binary using "./":

./QNX_GAME_SERVER

The "./" prefix is required to execute a program located in the current directory.

Therefore, the complete process is:

cd QNX_GAME_SERVER
make clean
make
cd build/aarch64
./QNX_GAME_SERVER


10. ADDING NEW SOURCE CODE
--------------------------

If you add a new source-code file to the server project, the new file must also be added to the Makefile.

For example, suppose you create a new source file:

src/player_manager.c

Adding the file to the src directory alone is not sufficient.

The new source file must be included in the source-file list in the Makefile.


11. UPDATING THE MAKEFILE
-------------------------

The Makefile contains a source-file section, commonly represented using a variable such as:

SOURCE =

All required source files must be included in this source list.

For example:

SOURCE = 
    src/main.c 
    src/server.c 
    src/network.c 
    src/player_manager.c

If a new source file is added to the project, make sure it is added to this source list.

After updating the Makefile, clean and rebuild the project:

make clean
make


12. IMPORTANT RULES
-------------------

The following rules should always be followed when working with the QNX Game Server:

1. Always run the make commands from the QNX_GAME_SERVER directory.

2. Always clean the previous build before compiling:

   make clean

3. Compile the complete project using:

   make

4. Make sure the newly generated binary is being executed.

5. The compiled binary is located at:

   build/aarch64/QNX_GAME_SERVER

6. Use "./" before the binary name when executing it:

   ./QNX_GAME_SERVER

7. If a new source file is added to the project, add that file to the appropriate source list in the Makefile.

8. After making source-code or Makefile changes, clean and rebuild the project before running the server.


13. COMPLETE WORKFLOW
---------------------

The complete development and execution workflow is:

1. Open the QNX_GAME_SERVER project.

2. Make the required changes to the source code inside the src directory.

3. If a new source file is created, add it to the Makefile.

4. Open a terminal in the QNX_GAME_SERVER directory.

5. Clean the previous build:

   make clean

6. Compile the complete project:

   make

7. Go to the directory containing the compiled binary:

   cd build/aarch64

8. Run the server:

   ./QNX_GAME_SERVER


14. QUICK COMMAND REFERENCE
---------------------------

Enter the project:

cd QNX_GAME_SERVER

Clean the previous build:

make clean

Compile the complete project:

make

Go to the binary directory:

cd build/aarch64

Run the server:

./QNX_GAME_SERVER


15. COMPLETE COMMAND SEQUENCE
-----------------------------

For normal development, the complete command sequence is:

cd QNX_GAME_SERVER
make clean
make
cd build/aarch64
./QNX_GAME_SERVER


16. DEVELOPMENT ENVIRONMENT
---------------------------

Operating System:
QNX Neutrino 8.0

Target Hardware:
Raspberry Pi 4

Architecture:
AArch64

Development IDE:
QNX Momentics

Programming Language:
C

Build System:
Make / QNX Makefile

Project Type:
Real-Time Multiplayer Game Server


17. FINAL NOTE
--------------

The source code is maintained inside the src directory.

The Makefile controls which source files are compiled.

The compiled executable is generated inside:

build/aarch64/QNX_GAME_SERVER

Always use the following sequence after making changes:

make clean
make

Then execute the newly compiled binary:

./QNX_GAME_SERVER

Following this workflow ensures that the server is compiled from the latest source code and that the latest binary is executed.
