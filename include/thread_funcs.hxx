#ifndef _THREAD_FUNCS_HXX_
#define _THREAD_FUNCS_HXX_

#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <queue>
#include <json.hpp>
#include <comparators.hxx>
#include <sstream>
#include <cctype>

using json = nlohmann::json;

extern std::set<stucts::node*, stucts::alphabeticNodeComparator> english_dictionary;
extern std::set<stucts::node*, stucts::alphabeticNodeComparator> russian_dictionary;
extern std::set<stucts::node*, stucts::alphabeticNodeComparator> spanish_dictionary;
extern std::set<stucts::node*, stucts::alphabeticNodeComparator> german_dictionary;

std::pair<std::vector<std::string>, size_t> splitJson(const std::string&);
std::vector<std::string> splitAndClean(const std::string&);

class Client
{
	std::string incomplete_string_;
	std::queue<std::string> requests_queue_;
	int socket_;

	int size_of_data_;
	char* data_;
public:
	// constructor.
	Client(int sock) : socket_(sock)
	{
		size_of_data_ = 1024;
		data_ = new char[size_of_data_];
	}
	// destructor.
	~Client()
	{
		std::cout << "destroying client.\n";
		if (data_)
			delete data_;
	}

	Client(const Client&) = delete;
	Client operator= (Client) = delete;

	void send_str(const std::string& str)
	{
		ssize_t bytes_sent = send(socket_, str.c_str(), strlen(str.c_str()), 0);
		if (bytes_sent < 0) {
			std::cerr << "Error sending data" << std::endl;
			close(socket_);
			return;
    		}
	}
	
	std::string dequeue_request()
	{
		if (requests_queue_.size() == 0)
		{
			int receivedBytes = recv(socket_, data_, size_of_data_ - 1, 0);
			if (receivedBytes < 0)
			{
				std::cerr << "Can't receive data.\n";
				return "";
			}

			std::string str(data_, receivedBytes);
			str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
			str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

			std::pair<std::vector<std::string>, int> pair = splitJson(str);
			std::vector<std::string> vec = pair.first;
			size_t lst = pair.second;

			if (lst < str.size() - 1)
			{
				incomplete_string_ = str.substr(lst + 1, str.size() - lst - 1);
			}

			for (const auto& x : vec)
				requests_queue_.push(x);
		}

		if (requests_queue_.size() > 0)
		{
			std::string result(requests_queue_.front());
			requests_queue_.pop();
			return result;
		}
		else return "";
	}

	void handle()
	{
		while(1)
		{
			std::string request = dequeue_request();
			
			try
			{
				json requestJSON = json::parse(request);
				if (requestJSON.at("Command") == "Translate")
				{
					std::string sentence  = requestJSON.at("Sentence");
					std::string from_lang = requestJSON.at("From-Language");
					std::string to_lang   = requestJSON.at("To-Language");

					std::set<stucts::node*, stucts::alphabeticNodeComparator>* from_set;

					if (from_lang == "English")
						from_set = &english_dictionary;
					else if (from_lang == "Russian")
						from_set = &russian_dictionary;
					else if (from_lang == "Spanish")
						from_set = &spanish_dictionary;
					else if (from_lang == "German")
						from_set = &german_dictionary;
					
					std::string translated_string = "";
					std::vector<std::string> words = splitAndClean(sentence);
					for (auto x : words)
					{
						std::transform(x.begin(), x.end(), x.begin(), [](unsigned char c) { return std::tolower(c); });
						auto it = std::find_if(from_set->begin(), from_set->end(), [&](stucts::node* nd) {
									std::string nd_copy(nd->_word);
									std::transform(nd_copy.begin(), nd_copy.end(), nd_copy.begin(), [](unsigned char c) { return std::tolower(c); });
									return x == nd_copy;
								});
						if (it == from_set->end())
							translated_string += x;
						else
						{
							if (to_lang == "English") 
								translated_string += (*it)->english->_word;
							else if (to_lang == "Russian")
								translated_string += (*it)->russian->_word;
							else if (to_lang == "Spanish")
                                                                translated_string += (*it)->spanish->_word;
							else if (to_lang == "German")
                                                                translated_string += (*it)->german->_word;
						}
						translated_string += " ";
					}
					std::cout << translated_string << '\n';
					send_str(translated_string);
				}
			}
			catch(const json::parse_error& e)
			{
				std::cerr << "Parse error at byte : " << e.byte << " : " << e.what() << " ; String : " << request << '\n';
			}
			catch (const json::type_error& e)
			{
				std::cerr << "Type error : " << e.what() << " ; String : " << request <<'\n';
			}
			catch (const json::out_of_range& e)
			{
				std::cerr << "Out of range error : " << e.what() << " ; String : " << request << '\n';
			}
		}
	}
};

void handle_client(Client* client)
{
	client->handle();
	delete client;
}

// returns json-s and position of starting last incomplete json.
std::pair<std::vector<std::string>, size_t> splitJson(const std::string& input) {
	std::vector<std::string> jsonObjects;
	size_t last = 0;
	size_t start = 0;
	int braceCount = 0;

	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '{') {
			if (braceCount == 0) {
				start = i;
			}
			braceCount++;
		} else if (input[i] == '}') {
			braceCount--;
			if (braceCount == 0) {
				jsonObjects.push_back(input.substr(start, i - start + 1));
				last = i;
			}
		}
	}
	
	return std::pair<std::vector<std::string>, size_t>(jsonObjects, last);
}

// split string by space and remove all punctuation marks.
std::vector<std::string> splitAndClean(const std::string& input) {
	std::vector<std::string> words;
	std::istringstream stream(input);
	std::string word;

	while (stream >> word) {
		word.erase(std::remove_if(word.begin(), word.end(), [](unsigned char c)
					{
						return std::ispunct(c);
					}), word.end());

		if (!word.empty()) {
			words.push_back(word);
		}
	}
	
	return words;
}

#endif
