#include <iostream>
#include <curl/curl.h>
#include <string.h>
#include <vector>
#include <sstream>

// TEMPORARY SOURCE DIRECTORY
const std::string SRC_DIRECTORY = "C:\\Users\\luked\\Pi Cloud\\Documents\\CodeMan\\sftp\\testFile.txt";

void getInput(const std::string prompt, std::string * response) {
    // Ask the prompt 
    std::cout << prompt;

    std::cin.clear();

    // Get the response
    std::getline(std::cin, *response);
}

/// @brief 
/// @param curl 
void uploadFile(CURL *curl) {
    // Construct the remote file path based on the current URL
    char *effective_url;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    std::string remotePath = effective_url; // Get the current remote directory

    // If the remote path doesn't end with '/', append the file name
    if (remotePath.back() != '/') {
        size_t lastSlash = remotePath.find_last_of('/');
        remotePath = remotePath.substr(0, lastSlash + 1); // Ensure it ends with '/'
    }

    // Assuming you want to upload SRC_DIRECTORY as 'file' or 'directory'
    remotePath += SRC_DIRECTORY.substr(SRC_DIRECTORY.find_last_of('/') + 1); // Get the filename from SRC_DIRECTORY

    // Set the SFTP upload options
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, ("sftp://" + remotePath).c_str());
    
    // Open the file for reading
    FILE *hd_src = fopen(SRC_DIRECTORY.c_str(), "rb");
    if (hd_src) {
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

        // Perform the upload
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        fclose(hd_src);
    } else {
        std::cerr << "Failed to open file: " << SRC_DIRECTORY << std::endl;
    }
}

int main() {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    
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

            // Declare variables to hold the current and previous URLs
            std::string currentUrl; // Holds the current URL
            std::string previousUrl; // Holds the previous URL
            currentUrl = effective_url; // Set the initial current URL
            previousUrl = currentUrl; // Set the previous URL as the initial one
            // cd command
            if (!args.empty() && args[0] == "cd") {
                // Check if 'cd' has the correct number of arguments
                if (args.size() == 2) {
                    // Retrieve the new directory
                    std::string newDir = args[1];
                    std::string newUrl;

                    // Check if newDir is an absolute path (starts with '/')
                    if (newDir[0] == '/') {
                        // Absolute path: directly append to sftp://pi
                        newUrl = "sftp://pi" + newDir; 
                    } else {
                        currentUrl = (currentUrl[currentUrl.size()-1] == '/') ? currentUrl : currentUrl + '/';

                        // Relative path: append to the current URL
                        size_t lastSlash = currentUrl.find_last_of('/');
                        if (lastSlash != std::string::npos) {
                            // Construct the new URL using the current URL and the newDir
                            newUrl = currentUrl.substr(0, lastSlash + 1) + newDir; // Append to the current directory
                        } else {
                            // If at root, just append the newDir
                            newUrl = "sftp://pi/" + newDir; // Set the new directory
                        }
                    }

                    // Remove any duplicate slashes
                    if (newUrl.find("//", 7) != std::string::npos) {
                        // Adjusts after the protocol (sftp://)
                        newUrl.erase(newUrl.find("//", 7), 1);
                    }


                    // Set the new URL
                    curl_easy_setopt(curl, CURLOPT_URL, newUrl.c_str());

                    // Try to list the contents of the new directory to check if it exists
                    CURLcode res = curl_easy_perform(curl);

                    // Check if the directory exists
                    if (res == CURLE_OK) {
                        previousUrl = currentUrl; // Update previous URL only if successful
                        currentUrl = newUrl; // Update the current URL
                    } else {
                        std::cerr << "Failed to change directory. Directory may not exist: " 
                                << curl_easy_strerror(res) << std::endl;

                        // Revert to the previous valid URL
                        curl_easy_setopt(curl, CURLOPT_URL, previousUrl.c_str());
                        currentUrl = previousUrl; // Revert current URL to previous
                    }
                } else if (args.size() > 2) {
                    // Too many arguments
                    std::cerr << "Too many arguments for 'cd' command." << std::endl;
                } else {
                    // Too few arguments
                    std::cerr << "'cd' command requires a directory argument." << std::endl;
                }
            }

            // HELP command
            if (args[0] == "help") {
                std::cout << "SFTP transfer help dialogue" << std::endl << std::endl;
                std::cout << "  help        : Displays this dialogue" << std::endl;
                std::cout << "  cd [path]   : Change the working remote directory" << std::endl;
                std::cout << "  quit        : Exit the program without uploading" << std::endl;
                std::cout << "  put         : Initiate transfer of file end exit program." << std::endl;
                std::cout << std::endl;

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
