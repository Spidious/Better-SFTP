#include <iostream>
#include <curl/curl.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <filesystem>

#define USER "luke"
#define PASSWORD ""
#define REMOTE "pi"
#define KEYPATH "C:\\Users\\luked\\.ssh\\id_ed25519"
#define DIRECTORY "."

// TEMPORARY SOURCE DIRECTORY
std::string SRC_DIRECTORY;
std::string SRC_FILENAME;

// Progress callback function
int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (ultotal > 0) {
        std::cout << "\rUpload progress: " << (ulnow * 100 / ultotal) << "% complete" << std::flush;
    }
    return 0; // Return 0 to continue the transfer
}

void getInput(const std::string prompt, std::string * response) {
    // Ask the prompt 
    std::cout << prompt;

    std::cin.clear();

    // Get the response
    std::getline(std::cin, *response);
}

/// @brief Upload file to server
/// @param curl curl variable
void uploadFile(CURL *curl) {
    // Construct the remote file path based on the current URL
    char *effective_url;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    
    // Check if effective_url is valid
    if (!effective_url) {
        std::cerr << "Failed to retrieve effective URL." << std::endl;
        return;
    }

    // Get the current remote directory
    std::string remoteDir = effective_url; // Get the current remote directory
    std::string fileName = SRC_DIRECTORY.substr(SRC_DIRECTORY.find_last_of('/') + 1); // Get the filename

    // Construct the full remote URL for uploading the file
    // std::string remotePath = remoteDir + "/" + SRC_FILENAME; TESTING TESTING

    // Set the SFTP upload options
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, (remoteDir + std::string("/") + SRC_FILENAME).c_str());

    // Open the file for reading
    FILE *fd = fopen((SRC_DIRECTORY + "\\" + SRC_FILENAME).c_str(), "rb");
    if (fd) {
        curl_easy_setopt(curl, CURLOPT_READDATA, fd);

        // Enable progress meter
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

        // Perform the upload
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        fclose(fd);
    } else {
        std::cerr << "Failed to open file: " << SRC_DIRECTORY << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Check if a file path is provided
    if (argc < 2) {
        std::cerr << "No file path provided!" << std::endl;
        return 1;
    }

    // Get the file path from command line arguments
    std::string filePath = argv[1];

    // Extract the file name from the path
    std::filesystem::path pathObj(filePath);
    SRC_FILENAME = pathObj.filename().string(); // Get the file name
    SRC_DIRECTORY = pathObj.parent_path().string(); // Get the directory path

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    
    if(curl) {
        // Set the URL of the SFTP server
        curl_easy_setopt(curl, CURLOPT_URL, "sftp://REMOTE/DIRECTORY"); // Connect to home directory without listing
    
        // Suppress listing or fetching data
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        // Provide user credentials
        curl_easy_setopt(curl, CURLOPT_USERPWD, "USER:PASSWORD");  // Username with no password
        
        // Specify the private key file for authentication
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, KEYPATH);

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
                continue;
            }

            // HELP command
            if (args[0] == "help") {
                std::cout << "SFTP transfer help dialogue" << std::endl << std::endl;
                std::cout << "  help        : Displays this dialogue" << std::endl;
                std::cout << "  cd [path]   : Change the working remote directory" << std::endl;
                std::cout << "  quit        : Exit the program without uploading" << std::endl;
                std::cout << "  put         : Initiate transfer of file end exit program." << std::endl;
                std::cout << std::endl;
                continue;
            }

            // Base case, break with final command
            if (args[0] == "quit") {
                std::cout << "exiting..." << std::endl;
                break;
            }

            // Base case, break with final command
            if (args[0] == "put") {
                std::cout << std::endl << "Uploading File..." << std::endl;
                uploadFile(curl);
                std::cout << "Done!" << std::endl << std::endl;
                break;
            }

            std::cout << "Unknown command \"" << args[0] << "\"" << std::endl; 

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
