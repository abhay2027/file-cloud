# Requirements 
g++ must be installed 
## To install g++ on Android 
1. Install termux from play store and run the command inside termux
   ```bash
   termux-setup-storage
2. Install g++
   ```bash
   pkg install clang

# Linux distros or Android 
1. Clone the repository:
   ```bash
   git clone https://github.com/abhay2027/file-cloud.git 
 
2. Change into the project directory:
   ```bash
   cd file-cloud

3. Compile
   ```bash
   g++ client.cpp -o client

4. Run
    ```bash
    ./client 182.65.204.249 2324

# Windows 
1. Install zip file 

2.  Change into the project directory:

3. Compile

   ```bash
    g++ clientwindows.cpp -o clientwindows.exe -lws2_32
4. Run
   ```bash  
   .\clientwindows.exe 182.65.204.249 2324
