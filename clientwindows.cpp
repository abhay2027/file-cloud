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
    cerr << msg << ": " << WSAGetLastError() << endl;
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
        memset(menu, 0, sizeof(menu));
        cout << "-------------------------------------------\n";

        n = recv(sock, menu, sizeof(menu) - 1, 0);
        if (n <= 0)
        {
            cerr << "Error reading menu\n";
            return;
        }

        menu[n] = '\0';
        cout << menu;
        cout << "-------------------------------------------\n";
        cout << "Enter file name to download: " << endl;

        memset(filename, 0, sizeof(filename));
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\r\n")] = '\0';
        send(sock, filename, strlen(filename), 0);
    }
};

class fileoperator
{
public:
    virtual void execute(SOCKET clientsock) = 0;
};

class recvfile : public fileoperator, public fetch
{
public:
    void execute(SOCKET sock) override
    {
        getfilename(sock);
        filerecv(sock);
    }

    void filerecv(SOCKET sock)
    {
        long filesize = 0;
        int n = recv(sock, (char *)&filesize, sizeof(filesize), 0);
        if (n <= 0)
        {
            cerr << "Failed to receive file size.\n";
            return;
        }

        if (filesize == 0)
        {
            cerr << "File not found or empty on server.\n";
            return;
        }

        ofstream outfile(filename, ios::binary);
        if (!outfile)
        {
            cerr << "Failed to open output file.\n";
            return;
        }

        char buffer[1024];
        long totalreceived = 0;
        while (totalreceived < filesize)
        {
            n = recv(sock, buffer, sizeof(buffer), 0);
            if (n <= 0)
            {
                cerr << "File not received correctly.\n";
                break;
            }
            outfile.write(buffer, n);
            totalreceived += n;
        }

        outfile.close();
        cout << "File received: " << totalreceived << " bytes\n";
    }
};

class upload : public fileoperator
{
    char filename[80];
    int n;

public:
    void execute(SOCKET sock) override
    {
        selectfile(sock);
        sendfilefun(sock);
    }

    void selectfile(SOCKET sock)
    {
        cout << "Enter file name: ";
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\r\n")] = '\0';
        send(sock, filename, strlen(filename), 0);
        cout << "Selected file: " << filename << endl;
    }

    void sendfilefun(SOCKET sock)
    {
        ifstream file(filename, ios::binary | ios::ate);
        if (!file)
        {
            string msg = "ERROR";
            send(sock, msg.c_str(), msg.size(), 0);
            cerr << "File not found.\n";
            return;
        }

        long filesize = file.tellg();
        file.seekg(0, ios::beg);
        send(sock, (char *)&filesize, sizeof(filesize), 0);

        char buffer[1024];
        while (!file.eof())
        {
            file.read(buffer, sizeof(buffer));
            send(sock, buffer, (int)file.gcount(), 0);
        }

        file.close();
        cout << "File sent: " << filesize << " bytes\n";
    }
};

class menu
{
protected:
    int chose = 0;
    int chosebytes;

public:
    void login(SOCKET sock)
    {
        cout << "1. Create account\n"
                "2. Login\n";

        string input;
        getline(cin, input);
        chose = stoi(input);
        chosebytes = htonl(chose);
        send(sock, (char *)&chosebytes, sizeof(chosebytes), 0);
        cout << "Choice sent to server.\n";
    }

    void choice(SOCKET sock)
    {
        cout << "Select operation:\n"
                "1. Download file\n"
                "2. Upload file\n"
                "Enter choice: ";
        string input;
        getline(cin, input);
        chose = stoi(input);
        chosebytes = htonl(chose);
        send(sock, (char *)&chosebytes, sizeof(chosebytes), 0);
        cout << "Choice sent to server.\n";
    }
};

class details
{
private:
    char username[20];
    int password;
    string input;
    int bytes;
    char errorr[90];

public:
    void enterdetails(SOCKET sock)
    {
        cout << "Enter username: ";
        fgets(username, sizeof(username), stdin);
        send(sock, (char *)&username, sizeof(username), 0);

        cout << "Enter password: ";
        getline(cin, input);
        password = stoi(input);
        bytes = htonl(password);
        send(sock, (char *)&bytes, sizeof(bytes), 0);
    }

    void logindetails(SOCKET sock)
    {
        cout << "Enter username: ";
        fgets(username, sizeof(username), stdin);
        send(sock, (char *)&username, sizeof(username), 0);

        memset(errorr, 0, sizeof(errorr));
        int n = recv(sock, errorr, sizeof(errorr) - 1, 0);
        if (n > 0)
        {
            errorr[n] = '\0';
            cout << errorr << endl;
            closesocket(sock);
            exit(1);
        }
        else if (n == 0)
        {
            cerr << "Server closed connection.\n";
            closesocket(sock);
        }
        else
        {
            perror("recv");
        }

        cout << "Enter password: ";
        getline(cin, input);
        password = stoi(input);
        bytes = htonl(password);
        cout << "Account created, restart program.\n";
    }

    void checkdetails(SOCKET sock)
    {
        memset(errorr, 0, sizeof(errorr));
        int n = recv(sock, errorr, sizeof(errorr) - 1, 0);
        if (n > 0)
        {
            errorr[n] = '\0';
            cout << errorr << endl;

            if (strstr(errorr, "Invalid username or password"))
            {
                closesocket(sock);
                exit(1);
            }
        }
        else
        {
            cerr << "Failed to receive server response.\n";
        }
    }
};

class server : public menu
{
    int argc;
    char **argv;

public:
    server(int c, char *v[])
    {
        argc = c;
        argv = v;
    }

    void ser()
    {
        WSADATA wsaData;
        int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (wsaerr != 0)
        {
            cerr << "WSAStartup failed: " << wsaerr << endl;
            exit(1);
        }

        SOCKET sockfd;
        int portno;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        if (argc < 3)
        {
            cerr << "Usage: " << argv[0] << " hostname port\n";
            WSACleanup();
            exit(1);
        }

        portno = atoi(argv[2]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == INVALID_SOCKET)
            error("Error opening socket");

        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            cerr << "Error, no such host.\n";
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }

        memset((char *)&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Connection failed");

        fileoperator *op;
        upload uploadfile;
        details logindetails;

        login(sockfd);
        if (chose == 1)
        {
            logindetails.enterdetails(sockfd);
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }
        else if (chose == 2)
        {
            logindetails.enterdetails(sockfd);
            logindetails.checkdetails(sockfd);
            choice(sockfd);
        }

        if (chose == 1)
        {
            op = new recvfile();
            op->execute(sockfd);
        }
        else if (chose == 2)
        {
            op = new upload();
            op->execute(sockfd);
            cout << "Client disconnected.\n";
            closesocket(sockfd);
        }

        WSACleanup();
    }
};

int main(int argc, char *argv[])
{
    server s(argc, argv);
    s.ser();
    return 0;
}
