#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void error(const char *msg)
{
    cerr << msg << " (WSA error code: " << WSAGetLastError() << ")\n";
    exit(1);
}

class fetch
{
protected:
    int n;
    char filename[200];
    char menu[500];

public:
    void getfilename(SOCKET sock)
    {
        n = recv(sock, menu, sizeof(menu) - 1, 0);
        if (n <= 0)
        {
            cerr << "Error reading menu\n";
            return;
        }
        menu[n] = '\0';
        cout << menu;
        memset(filename, 0, sizeof(filename));
        fgets(filename, 200, stdin);
        filename[strcspn(filename, "\r\n")] = '\0';
        send(sock, filename, strlen(filename), 0);
    }
};

class recvfile : public fetch
{
public:
    void filerecv(SOCKET sock)
    {
        long filesize;
        int n = recv(sock, (char *)&filesize, sizeof(filesize), 0);
        if (n < 0)
        {
            cerr << "failed to receive file size";
            exit(1);
        }
        if (filesize == 0)
        {
            cerr << "File not found or empty on server." << endl;
            return;
        }

        ofstream outfile(filename, ios::binary);
        if (!outfile)
        {
            cerr << "failed to open output file";
            return;
        }

        char buffer[1024];
        long totalrecieved = 0;
        while (totalrecieved < filesize)
        {
            n = recv(sock, buffer, sizeof(buffer), 0);
            if (n <= 0)
                break;

            outfile.write(buffer, n);
            totalrecieved += n;
        }
        outfile.close();
        cout << "file received: " << totalrecieved << " bytes" << endl;
    }
};

class upload
{
    char f
