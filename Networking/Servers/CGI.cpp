#include "CGI.hpp"

CGI::CGI() {}

CGI::CGI(Parse::serverBlock server, std::vector<std::string> parsed, std::string index)
{
	_env["SERVER_PROTOCOL"] = parsed[2];					// correct
	_env["SERVER_NAME"] = server.listen[1];		// hostname, can be ip if hostname is blank
	_env["SERVER_PORT"] = server.listen[0];					// correct
	_env["REQUEST_METHOD"] = parsed[0];						// correct
	_env["PATH_INFO"] = parsed[1];							// check wiki
	std::string absolute_path = getcwd(NULL, 0);
	_env["PATH_TRANSLATED"] = absolute_path + parsed[1];    // 
	_env["SCRIPT_NAME"] = index;
	_env["QUERY_STRING"] = ""; //might need to add something
	// _env["REMOTE_HOST"] = maybe add later
	// _env["REMOTE_ADDR"] = maybe add later
	// _env["AUTH_TYPE"] = maybe add
	// _env["REMOTE_USER"] = maybe add
	// _env["REMOTE_IDENT"] = maybe add
	// _env["CONTENT_TYPE"] = maybe add
	// _env["CONTENT_LENGTH"] = maybe later
	// _env["HTTP_ACCEPT_LANGUAGE"] = maybe later
	_env["HTTP_USER_AGENT"] = _getUserAgent(parsed);
	// _env["HTTP_COOKIE"] = "";
	_env["SERVER_SOFTWARE"] = "WEBSERV/19.42"; 				// correct
	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); it++)
		std::cout << it->first << " = " << it->second << std::endl;
}

CGI::~CGI() {}

std::string     CGI::_getUserAgent(std::vector<std::string> parsed)
{
	std::string str;
	
	for (std::vector<std::string>::iterator it = parsed.begin(); it != parsed.end(); it++)
		if (*(it - 1) == "User-Agent:")
			for (; *it != "Accept:"; it++)
				str += *it + " ";
	str.resize(str.size() - 1);
	return str;
}

void CGI::runCGI()
{
	char *c_env[_env.size()];
	std::map<std::string, std::string>::iterator it = _env.begin();
	int i = 0;
	while (it != _env.end())
	{
		std::string tmp = it->first + '=' + it->second;
		c_env[i++] = (char *)tmp.c_str();
		it++;
	}
	c_env[i] = NULL;


	int fd_file = open("temp.txt", O_RDWR | O_CREAT | O_APPEND, 0666);
	pid_t pid = fork();
	if (pid == 0)
	{
		dup2(fd_file, STDOUT_FILENO);
		close(fd_file);
		char const *pathname = _env["SCRIPT_NAME"].c_str();
		char *placeholder = (char *)"19";
		execle(pathname, placeholder, c_env);
	}
	else
	{
		waitpid(pid, 0, 0);
		close(fd_file);
	}
}