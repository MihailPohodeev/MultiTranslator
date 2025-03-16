#ifndef _THREAD_FUNCS_HXX_
#define _THREAD_FUNCS_HXX_

#include <string>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <queue>
#include <json.hpp>
#include <comparators.hxx>
#include <sstream>
#include <cctype>
#include <mutex>

using json = nlohmann::json;

extern std::set<structs::node*, structs::alphabeticNodeComparator> english_dictionary;
extern std::set<structs::node*, structs::alphabeticNodeComparator> russian_dictionary;
extern std::set<structs::node*, structs::alphabeticNodeComparator> spanish_dictionary;
extern std::set<structs::node*, structs::alphabeticNodeComparator> german_dictionary;

extern std::mutex sets_guard;
extern std::mutex dictionary_guard;

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
		
			if (receivedBytes == 1)
			{
				std::cerr << "Received empty string!\n";
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
			{
				std::cout << x << '\n';
				requests_queue_.push(x);
			}
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
			if (request == "")
				continue;
			
			try
			{
				json requestJSON = json::parse(request);
				if (requestJSON.at("Command") == "Translate")
				{
					std::string sentence  = requestJSON.at("Sentence");
					std::string from_lang = requestJSON.at("From-Language");
					std::string to_lang   = requestJSON.at("To-Language");

					std::set<structs::node*, structs::alphabeticNodeComparator>* from_set;

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
						auto it = std::find_if(from_set->begin(), from_set->end(), [&](structs::node* nd) {
									std::string nd_copy(nd->_word);
									std::transform(nd_copy.begin(), nd_copy.end(), nd_copy.begin(), [](unsigned char c) { return std::tolower(c); });
									return x == nd_copy;
								});
						if (it == from_set->end())
							translated_string += "NoNe";
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

					json responseJSON;
					responseJSON["Command"] 	= "Response";
					responseJSON["Sentence"]	= translated_string;
					responseJSON["From-Language"] 	= requestJSON.at("From-Language");
					responseJSON["To-Language"]	= requestJSON.at("To-Language");
					send_str(responseJSON.dump() + "\n");
				}
				else if (requestJSON.at("Command") == "Add-New-Word")
				{
					std::string english = requestJSON.at("English");
					std::string russian = requestJSON.at("Russian");
					std::string spanish = requestJSON.at("Spanish");
					std::string german  = requestJSON.at("German");

					std::transform(english.begin(), english.end(), english.begin(), [](unsigned char c) { return std::tolower(c); });
					auto it = std::find_if(english_dictionary.begin(), english_dictionary.end(), [&](structs::node* nd) {
						std::string nd_copy(nd->_word);
						std::transform(nd_copy.begin(), nd_copy.end(), nd_copy.begin(), [](unsigned char c) { return std::tolower(c); });
						return english == nd_copy;
					});

					if (it != english_dictionary.end())
					{
						json responseJSON;
						responseJSON["Command"] = "Response";
						responseJSON["Status"] = "Failed";
						responseJSON["Message"] = "This word already exist!";
						send_str(responseJSON.dump() + "\n");
						continue;
					}

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

					{
						std::lock_guard<std::mutex> lock(sets_guard);
						english_dictionary.insert(current_english_node);
						russian_dictionary.insert(current_russian_node);
						spanish_dictionary.insert(current_spanish_node);
						german_dictionary.insert(current_german_node);
					}

					std::string new_words = english + ", " + russian + ", " + spanish + ", " + german + '\n';
					
					{
						std::lock_guard<std::mutex> lock(dictionary_guard);
						std::ofstream outfile;
						outfile.open("dictionary.csv", std::ios::app);
						outfile << new_words;
						outfile.close();
					}

					json responseJSON;
					responseJSON["Command"] = "Response";
					responseJSON["Status"] = "Success";
					responseJSON["Message"] = "OK";
					send_str(responseJSON.dump() + "\n");
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
