#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
using namespace std;
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

class fetch
{
protected:
    int n;
    char filename[200];
    char menu[500];

public:
    void getfilename(int sock)
    {
        bzero(menu, 500);
        cout << "-------------------------------------------\n";
        n = read(sock, menu, 500);
        if (n <= 0)
        {
            cerr << "Error reading menu\n";
            return;
        }

        menu[n] = '\0';
        cout << menu;
        cout << "-------------------------------------------\n";
        cout << "enter file name to download" << endl;
        bzero(filename, 200);
        fgets(filename, 200, stdin);
        filename[strcspn(filename, "\r\n")] = '\0';
        n = write(sock, filename, strlen(filename));
    }
};

class fileoperator
{
public:
    virtual void execute(int clientsock) = 0;
};

class recvfile : public fileoperator, public fetch
{
public:
    void execute(int sock) override
    {
        getfilename(sock);
        filerecv(sock);
    }
    void filerecv(int sock)
    {
        long filesize;
        int n = recv(sock, &filesize, sizeof(filesize), 0);
        if (n < 0)
        {
            cerr << "failed to recieve file size";
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
        }
        char buffer[1024];
        long totalrecieved = 0;
        while (totalrecieved < filesize)
        {
            n = recv(sock, buffer, sizeof(buffer), 0);
            if (n < 0)
            {
                cerr << "file not recived";
                exit(1);
            }
            outfile.write(buffer, n);
            totalrecieved += n;
        }
        outfile.close();
        cout << "file recieved: " << totalrecieved << " bytes" << endl;
    }
};

class upload: public fileoperator
{
    char filename[80];
    int n;

public:
   void execute(int sock) override
    {
        selectfile(sock);
        sendfilefun(sock);
    }
    void selectfile(int sock)
    {
        cout << "enter file name" << endl;
        fgets(filename, 80, stdin);
        n = write(sock, filename, strlen(filename));
        filename[strcspn(filename, "\r\n")] = '\0';
        cout << "Uploaded file: " << filename << endl;
    }

    void sendfilefun(int sock)
    {

        ifstream file(filename, ios::binary | ios::ate);
        if (!file)
        {
            string msg = "ERROR";
            send(sock, msg.c_str(), msg.size(), 0);
            cerr << "file not found" << endl;
            return;
        }
        cout << filename;
        long filesize = file.tellg();
        file.seekg(0, ios::beg);
        n = send(sock, &filesize, sizeof(filesize), 0);

        char buffer[1024];
        while (!file.eof())
        {
            file.read(buffer, sizeof(buffer));
            send(sock, buffer, file.gcount(), 0);
        }
        file.close();
        cout << "file sent: " << filesize << " bytes" << endl;
    }
};

class menu
{
protected:
    int n;
    int chose = 0;
    int chosebytes;

public:
    void login(int sock)
    {
        cout << "1. create account\n"
                "2. login\n";

        string input;
        getline(cin, input);
        chose = stoi(input);
        chosebytes = htonl(chose);
        write(sock, (char *)&chosebytes, sizeof(chosebytes));
        cout << "Choice sent to server.\n";
    }
    void choice(int sock)
    {
        cout << "select the operation you want to perform\n"
                "1. download file \n"
                "2. upload file\n"
                "Enter a choice :";
        string input;
        getline(cin, input);
        chose = stoi(input);
        chosebytes = htonl(chose);
        write(sock, (char *)&chosebytes, sizeof(chosebytes));
        cout << "Choice sent to server.\n";
    }
};

class details
{
private:
    char username[20];
    int password;
    string input;
    int n;
    int bytes;
    char errorr[90];

public:
    void enterdetails(int sock)
    {
        cout << "enter username :";
        fgets(username, 20, stdin);
        write(sock, (char *)&username, sizeof(username));

        cout << "enter a password :";

        getline(cin, input);
        password = stoi(input);
        bytes = htonl(password);
        write(sock, (char *)&bytes, sizeof(bytes));
    }
    void logindetails(int sock)
    {
        cout << "enter username :";
        fgets(username, 20, stdin);
        write(sock, (char *)&username, sizeof(username));
        bzero(errorr, sizeof(errorr));

        int n = recv(sock, &errorr, sizeof(errorr), 0);
        if (n > 0)
        {
            errorr[n] = '\0';
            cout << errorr << std::endl;
            close(sock);
            exit(1);
        }
        else if (n == 0)
        {
            std::cerr << "Server closed connection\n";
            close(sock);
        }
        else
        {
            perror("read");
        }
        cout << "enter a password :";
        getline(cin, input);
        password = stoi(input);
        bytes = htonl(password);
        cout << "account created restart\n";
    }
    void checkdetails(int sock)
    {
        bzero(errorr, sizeof(errorr));
        int n = read(sock, errorr, sizeof(errorr) - 1);
        if (n > 0)
        {
            cout << errorr << endl;

            if (strstr(errorr, "Invalid username or password"))
            {
                close(sock);
                exit(1);
            }
        }
        else
        {
            cerr << "Failed to receive server response." << endl;
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
        int sockfd, portno, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        if (argc < 3)
        {
            cout << "stderr" << argv[0] << "hostname port" << endl;
            exit(1);
        }
        portno = atoi(argv[2]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("error opening socket");

        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            fprintf(stderr, "Error, no such host");
        }
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("connection failed");
        fileoperator *op; ////-----------------

        upload uploadfile;
        menu choose;
        details logindetails;
        login(sockfd);
        if (chose == 1)
        {
            logindetails.enterdetails(sockfd);
            exit(1);
            close(sockfd);
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

                cout << "Client disconnected" << endl;
                close(sockfd);
            }
    }
};

int main(int argc, char *argv[])
{
    server s(argc, argv);
    s.ser();
    return 0;
}

