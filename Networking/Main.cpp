#include <iostream>
#include <string>

#include<queue>

#include<boost/thread.hpp>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
#include<boost/asio/ip/tcp.hpp>
#include<boost/algorithm/string.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<std::string> string_ptr;
typedef boost::shared_ptr<std::queue<string_ptr> > messageQueue_ptr;

boost::asio::io_service service;
messageQueue_ptr messageQueue(new std::queue<string_ptr>);
boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 8001);
const int INPUTSIZE = 265;
string_ptr promptCpy;

bool isOwnMessage(string_ptr);
void displayLoop(socket_ptr);
void inboundLoop(socket_ptr, string_ptr);
void writeLoop(socket_ptr, string_ptr);
std::string* buildPrompt();

int main(int argc, char** argv)
{
	try
	{
		boost::thread_group threads;
		socket_ptr sock(new boost::asio::ip::tcp::socket(service));

		string_ptr prompt(buildPrompt());
		promptCpy = prompt;

		sock->connect(ep);

		std::cout << "Welcome to the ChatApplication\nType \"exit\" to quit" << std::endl;

		threads.create_thread(boost::bind(displayLoop, sock));
		threads.create_thread(boost::bind(inboundLoop, sock, prompt));
		threads.create_thread(boost::bind(writeLoop, sock, prompt));

		threads.join_all();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}



	system("pause");
	return 0;
}

std::string* buildPrompt()
{
	char inputBuf[INPUTSIZE] = { 0 };
	char nameBuf[INPUTSIZE] = { 0 };
	std::string* prompt = new std::string(": ");
	std::cout << "Please input a new username: ";
	std::cin.getline(nameBuf, INPUTSIZE);
	*prompt = (std::string)nameBuf + *prompt;
	boost::algorithm::to_lower(*prompt);
	return prompt;
}

void inboundLoop(socket_ptr sock, string_ptr prompt)
{
	int bytesRead = 0;
	char readBuf[1024] = { 0 };
	for (;;)
	{
		if (sock->available())
		{
			bytesRead = sock->read_some(boost::asio::buffer(readBuf, INPUTSIZE));
			string_ptr msg(new std::string(readBuf, bytesRead));
			messageQueue->push(msg);
		}
		boost::this_thread::sleep(boost::posix_time::millisec(1000));
	}
}

void writeLoop(socket_ptr sock, string_ptr prompt)
{
	char inputBuf[INPUTSIZE] = { 0 };
	std::string inputMsg;
	for (;;)
	{
		std::cin.getline(inputBuf, INPUTSIZE);
		inputMsg = *prompt + (std::string)inputBuf + '\n';
		if (!inputMsg.empty())
		{
			sock->write_some(boost::asio::buffer(inputMsg, INPUTSIZE));
		}
		// The string for quitting the application
		// On the server-side there is also a check for "quit" to terminate the TCP socket
		if (inputMsg.find("exit") != std::string::npos)
			exit(1); // Replace with cleanup code if you want but for this tutorial exit is enough
		inputMsg.clear();
		memset(inputBuf, 0, INPUTSIZE);
	}
}

void displayLoop(socket_ptr sock)
{
	for (;;)
	{
		if (!messageQueue->empty())
		{
			// Can you refactor this code to handle multiple users with the same prompt?
			if (!isOwnMessage(messageQueue->front()))
			{
				std::cout << "\n" + *(messageQueue->front());
			}
			messageQueue->pop();
		}
		boost::this_thread::sleep(boost::posix_time::millisec(1000));
	}
}

bool isOwnMessage(string_ptr message)
{
	if (message->find(*promptCpy) != std::string::npos)
		return true;
	else
		return false;
}