#include "Response.hpp"

Response::Response() {}

void    Response::_handler(int clientSocket, struct Parse::serverBlock server)
{
    std::string output;
    int size;
    std::string type;
    char buffer[BUFF_SIZE] = {0};
    int n;

    if ((n = recv(clientSocket, buffer, sizeof(buffer), 0)) <= 0)
    {
        if (n == -1)
            std::cerr << RED << "recv\n" << RESET;
        return ;
    }
	std::cout << buffer << std::endl;
    std::istringstream iss(buffer);
    std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

	////////////////////////
	for (std::vector<std::string>::iterator it = parsed.begin(); it != parsed.end(); it++)
	{
		if ((*it).find("multipart/form-data") != size_t(-1))
		{
			std::string response = "HTTP/1.1 100 CONTINUE\r\n\r\n";
			size = response.size();
			std::cout << size << "\n";
			_sendall(clientSocket, response.c_str(), &size);
		}
	}
	////////////////////////

    type = parsed[1].substr(parsed[1].rfind(".") + 1, parsed[1].size() - parsed[1].rfind("."));
    _setDefaultData(parsed[1]);
    _setBlockData(parsed, server, &type);
    output = _getClientData(type, parsed, server);
    size = output.size();
    if (_sendall(clientSocket, output.c_str(), &size) == -1)
    {
        std::cerr << RED << "sendall\n" << RESET;
        std::cout << YELLOW << "Only " << size << " bytes sended because of the error\n" << RESET;
    }
}

void    Response::_setDefaultData(std::string location)
{
    _method.clear();
    _error_page.clear();
    _redirect.clear();

    _root = "/www/";
    _index = location;
    _max_size = 1048576;
    _errorCode = 200;
    _method.push_back("GET");
    _autoindex = "off";
}

void    Response::_setBlockData(std::vector<std::string> parsed, struct Parse::serverBlock server, std::string *type)
{
    int mark = 0;

    if (!server.client_max_body_size.empty())
        _max_size = atoi(server.client_max_body_size.c_str());
    if (!server.error_page.empty())
        for (std::map<int, std::string>::iterator it = server.error_page.begin(); it != server.error_page.end(); it++)
            _error_page[it->first] = it->second;
    if (!server.location.empty())
    {
        for (std::vector<Parse::locationBlock>::iterator it = server.location.begin(); it != server.location.end(); it++)
        {
            if (it->name == parsed[1])
            {
                if (!it->root.empty())
                    _root = it->root;
                if (!it->index.empty())
                {
                    size_t i = 0;
                    for (i = 0; i < it->index.size(); i++)
                    {
                        std::ifstream f("." + _root + it->index[i]);
                        if (f.good())
                        {
                            f.close();
                            break;
                        }
                        f.close();
                    }
                    if (i == it->index.size())
                        i--;
                    _index = it->index[i];
                    *type = _index.substr(_index.rfind(".") + 1, _index.size() - _index.rfind("."));
                }
                if (!it->method.empty())
                {
                    _method.pop_back();
                    for (size_t i = 0; i < it->method.size(); i++)
                        _method.push_back(it->method[i]);
                }
                if (!it->autoindex.empty())
                    _autoindex = it->autoindex;
                if (!it->redirect.empty())
                    _redirect.insert(std::pair<int, std::string>(atoi(it->redirect[0].c_str()), it->redirect[1]));
            }
        }
    }
    for (std::vector<std::string>::iterator it = parsed.begin(); it != parsed.end(); it++)
        if (it->find("Content-Length:") != std::string::npos)
            if (atoi((it + 1)->c_str()) > _max_size)
                _errorCode = 413;
    for (std::vector<std::string>::iterator it = _method.begin(); it != _method.end(); it++)
        if (parsed[0] == *it)
            mark = 1;
    if (mark == 0)
        _errorCode = 405;
}

std::string     Response::_getClientData(std::string type, std::vector<std::string> parsed, struct Parse::serverBlock server)
{
    std::ostringstream oss;
    std::string content;
	int status = 0;

	if (_typeIsPy(type))
	{
		CGI cgi(server, parsed, _index);
		status = cgi.runCGI();
		std::string path = getcwd(NULL, 0);
		path.append("/temp.txt");
		if (status == 1)
		{
			remove(path.c_str());
			_errorCode = 404; // this has to change to 400 error code?
			content = _getContent(parsed, &type);
			_createHeader(oss, _errorCode, type, content.size());
			oss << content;
  			return oss.str();
		}
		std::ifstream f(path);
		if (f.is_open())
		{
			std::stringstream buffer;
			buffer << f.rdbuf();
			content = buffer.str();
			f.close();
			remove(path.c_str());
		}
	}
	else
	{
    	content = _getContent(parsed, &type);
		_createHeader(oss, _errorCode, type, content.size());
	}
    oss << content;
    return oss.str();
}

std::string     Response::_getContent(std::vector<std::string> parsed, std::string *type)
{
    std::string content;
    std::string url = "http://" + parsed[4] + parsed[1];
    std::string path = "./www" + parsed[1];

    if (_errorCode >= 400 && _errorCode <= 511)
        content = _getErrorPage(type);
    else
    {
        if (_index != "/")
        {
            if (_autoindex == "off")
            {
                std::ifstream f("." + _root + _index);
                if (!f.good())
                {
                    _errorCode = 404;
                    content = _getErrorPage(type);
                }
                else
                    content = _getFile(&f);
                f.close();
            }
            else
                content = _getAutoindexHtml(path, url, type);
        }
        else
        {
            if (_autoindex == "off")
                content = _getDefaultFile(type);
            else
                content = _getAutoindexHtml("./www", url, type);
        }
    }
    return content;
}

void Response::_createHeader(std::ostringstream &oss, int _errorCode, std::string type, size_t content_length)
{
    if (_redirect.empty())
    {
        oss << "HTTP/1.1 " << _errorCode << _getStatus(_errorCode);
	    oss << _getCacheControl();
        oss << _getContentType(type);
	    oss << _getContentLength(content_length);
    }
    else
    {
        std::map<int, std::string>::iterator it = _redirect.begin();
        oss << "HTTP/1.1 " << it->first << _getStatus(it->first);
        oss << _getLocation(it->second);
    }
	oss << "\r\n";
}

bool Response::_typeIsPy(std::string type)
{
	int pos = type.find("py?");
	if (pos != -1)
		type = type.substr(0, pos + 2);
	else
	{
		pos = type.find("py/");
		if (pos != -1)
			type = type.substr(0, pos + 2);
	}
	if (type == "py")
		return true;
	else
		return false;
}
