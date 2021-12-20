#include "TestServer.hpp"

TestServer::TestServer()
: Server(AF_INET, SOCK_STREAM, 0, 80, INADDR_ANY, 10) { launch(); }

void    TestServer::_accept()
{
    struct sockaddr_in address = getSocket()->getAddress();
    int addrlen = sizeof(address);
    _newSocket = accept(getSocket()->getSock(), (struct sockaddr *)&address, (socklen_t *)&addrlen);
    read(_newSocket, _buffer, 30000);
}

void    TestServer::_handle()
{
    std::cout << _buffer << std::endl;
}

void    TestServer::_response()
{
    std::istringstream iss(_buffer);
    std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

    std::string content;
    int errorCode = 200;

    if (parsed.size() >= 3 && parsed[0] == "GET")
    {
        if (parsed[1].size() != 1)
        {
            std::ifstream f("./" + parsed[1]);
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
                errorCode = 200;
            }
            f.close();
        }
        else
        {
            std::ifstream f("./default.html");
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
                errorCode = 200;
            }
            f.close();
        }
    }

    std::ostringstream oss;
    oss << "HTTP/1.1 " << errorCode << " OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-type: text/html\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "\r\n";
    oss << content;

    std::string output = oss.str();
    int size = output.size();

    sendToClient(output.c_str(), size);
}

void    TestServer::sendToClient(const char *msg, int len)
{
    write(_newSocket, msg, len);
    close(_newSocket);
}

void    TestServer::launch()
{
    while (1)
    {
        std::cout << "======= WAITING ======\n";
        _accept();
        _handle();
        _response();
        std::cout << "======= DONE ======\n";
    }
}