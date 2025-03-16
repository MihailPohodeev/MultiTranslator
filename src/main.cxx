#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <set>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>

#include <csv.h>
#include <thread_funcs.hxx>
#include <comparators.hxx>

#define PORT 53535

std::vector<Client*> clients;

std::set<structs::node*, structs::alphabeticNodeComparator> english_dictionary;
std::set<structs::node*, structs::alphabeticNodeComparator> russian_dictionary;
std::set<structs::node*, structs::alphabeticNodeComparator> spanish_dictionary;
std::set<structs::node*, structs::alphabeticNodeComparator> german_dictionary;

std::mutex sets_guard;
std::mutex dictionary_guard;

int main()
{
	io::CSVReader<4> in("dictionary.csv");
	in.read_header(io::ignore_extra_column, "English", "Russian", "Spanish", "German");
	std::string english;
	std::string russian;
	std::string spanish;
	std::string german;
	while(in.read_row(english, russian, spanish, german)){
		structs::node* current_english_node = new structs::node;
		current_english_node->_word = english;

		structs::node* current_russian_node = new structs::node;
		current_russian_node->_word = russian;

		structs::node* current_spanish_node = new structs::node;
		current_spanish_node->_word = spanish;

		structs::node* current_german_node = new structs::node;
		current_german_node->_word = german;

		current_english_node->english = current_english_node;
		current_english_node->russian = current_russian_node;
		current_english_node->spanish = current_spanish_node;
		current_english_node->german  = current_german_node;

		current_russian_node->english = current_english_node;
		current_russian_node->russian = current_russian_node;
		current_russian_node->spanish = current_spanish_node;
		current_russian_node->german  = current_german_node;

		current_spanish_node->english = current_english_node;
		current_spanish_node->russian = current_russian_node;
		current_spanish_node->spanish = current_spanish_node;
		current_spanish_node->german  = current_german_node;

		current_german_node->english = current_english_node;
		current_german_node->russian = current_russian_node;
		current_german_node->spanish = current_spanish_node;
		current_german_node->german  = current_german_node;

		english_dictionary.insert(current_english_node);
		russian_dictionary.insert(current_russian_node);
		spanish_dictionary.insert(current_spanish_node);
		german_dictionary.insert(current_german_node);
	}

	struct sockaddr_in serverAddress;
	int listen_socket;
	if ( (listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		std::cerr << "socket creation failed.\n";
		exit(-1);
	}

	int turnOn = 1;
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1)
	{
		std::cerr << "setsockopt failed.\n";
		exit(-1);
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("0.0.0.0");
	serverAddress.sin_port = htons(PORT);

	if (bind( listen_socket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
	{
		std::cerr << "bind failed.\n";
		exit(-1);
	}

	if (listen(listen_socket, 1000) == -1)
	{
		std::cerr << "listen failed.\n";
		exit(-1);
	}
	
	while(1)
	{
		int client_socket;
		client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket < 0)
			std::cerr << "accept failed.\n";
		Client* client = new Client(client_socket);
		std::cout << "client created.\n";
		std::thread th(&handle_client, client);
		th.detach();
	}

	// clean memory.
	for (structs::node* x : english_dictionary)
		delete x;
	for (structs::node* x : russian_dictionary)
		delete x;
	for (structs::node* x : spanish_dictionary)
		delete x;
	for (structs::node* x : german_dictionary)
		delete x;

	return 0;
}
