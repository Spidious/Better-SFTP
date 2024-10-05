#include <iostream>
#include <curl/curl.h>
#include <string.h>
#include <vector>
#include <sstream>

void getInput(const std::string prompt, std::string * response) {
    // Ask the prompt 
    std::cout << prompt;

    std::cin.clear();

    // Get the response
    std::getline(std::cin, *response);
}

int main() {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    // TEMPORARY SOURCE DIRECTORY
    std::string SRC_DIRECTORY = "C:\\Users\\luked\\Pi Cloud\\Documents\\CodeMan\\sftp\\testFile.txt";
    
    if(curl) {
        // Set the URL of the SFTP server
        curl_easy_setopt(curl, CURLOPT_URL, "sftp://pi/."); // Connect to home directory without listing
    
        // Suppress listing or fetching data
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        // Provide user credentials
        curl_easy_setopt(curl, CURLOPT_USERPWD, "luke:");  // Username with no password
        
        // Specify the private key file for authentication
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, "C:\\Users\\luked\\.ssh\\id_ed25519");

        // Perform the SFTP connection without listing the directory
        res = curl_easy_perform(curl);

        // String for response
        std::string * response = new std::string;;

        // Break response into array
        std::vector<std::string> args;

        // Ask for command
        do {
            // Get the current url
            char *effective_url;
            res = curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

            // Prompt for user input
            getInput(effective_url + std::string(" > "), response);

            // Clear the args vector for new input
            std::vector<std::string> args;
            std::istringstream iss(*response);
            std::string arg;

            // Extract words by spaces
            while (iss >> arg) {
                args.push_back(arg);
            }

            // Check if there are any commands entered
            if (args.empty()) {
                continue; // Skip if nothing is entered
            }

            // cd command
            if (!args.empty() && args[0] == "cd") {
                // Check cd has args
                if (args.size() == 2) {

                    // Retrieve directory
                    std::string newDir = args[1];

                    // Get the current URL and construct the new URL
                    char *effective_url;
                    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
                    std::string currentUrl = effective_url;

                    // Change to args[1]
                } else if (args.size() > 2) {
                    // Too many arguments
                } else {
                    // Too few arguments
                }
                // Add your code to change the remote directory here
            }

            // Base case, break with final command
            if (args[0] == "quit") {
                std::cout << "exiting..." << std::endl;
                break;
            }

            // Base case, break with final command
            if (args[0] == "put") {
                std::cout << "put <host> <dest>" << std::endl;
                break;
            }

        } while (true);
        
        // Error handling
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
