#include "RunServer.hpp"

RunServer::RunServer(int server_fd, struct sockaddr_in address)
{
    while (1)
    {
        _accept(server_fd, address);
        _handler(server_fd);
    }
}

void    RunServer::_accept(int server_fd, struct sockaddr_in address)
{
    int addrlen = sizeof(address);
    _newSocket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    testConnection(_newSocket, "new socket");
}

void    RunServer::_handler(int server_fd)
{
    if (!fork())
    {
        close(server_fd);
        read(_newSocket, _buffer, 30000);
        std::cout << _buffer << std::endl;
        std::istringstream iss(_buffer);
        std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
        _errorCode = 200;

        if (parsed.size() >= 3 && parsed[0] == "GET")
        {
            if (parsed[1].size() != 1)
            {
                std::ifstream f("./Pages/" + parsed[1]);
                if (!f.good())
                {
                    std::ifstream f2("./Pages/error.html");
                    if (f2.good())
                    {
                        std::string str((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());
                        _content = str;
                        _errorCode = 404;
                    }
                    f2.close();
                }
                else
                {
                    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    _content = str;
                    _errorCode = 200;
                }
                f.close();
            }
            else
            {
                std::ifstream f("./Pages/default.html");
                if (f.good())
                {
                    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    _content = str;
                    _errorCode = 200;
                }
                f.close();
            }
        }
        _sendToClient();
    }
    close(_newSocket);
    
}

void    RunServer::_sendToClient()
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << _errorCode << " OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-type: text/html\r\n";
    oss << "Content-Length: " << _content.size() << "\r\n";
    oss << "\r\n";
    oss << _content;

    std::string output = oss.str();
    int size = output.size();

    int bytes_sending = send(_newSocket, output.c_str(), size, 0);
    testConnection(bytes_sending, "send");
    close(_newSocket);
    exit(0);
}