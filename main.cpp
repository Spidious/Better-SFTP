#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <filesystem>
#include <curl/curl.h>

std::string const USER = "USER";
std::string const PASSWORD = "PASSWORD";
std::string const REMOTE = "SERVER.local";
std::string const KEYPATH = "KEYDIR";
std::string const DIRECTORY = "LOGINDIR";

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
#include <iostream>
#include <curl/curl.h>

/// @brief Upload file to server
/// @param curl curl variable
#include <iostream>
#include <curl/curl.h>

/// @brief Upload file to server
/// @param curl curl variable
void uploadFile(CURL *curl) {
    // Get the effective base URL from the CURL object
    char *effective_url;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

    if (!effective_url) {
        std::cerr << "Failed to retrieve effective URL." << std::endl;
        return;
    }

    // Extract the filename from the local path
    std::string fileName = SRC_FILENAME;  // e.g., "Beetlejuice-A1 T00.mp4"

    // Encode the filename (for spaces or special characters)
    char *encodedFileName = curl_easy_escape(curl, fileName.c_str(), 0);

    // Construct the final remote path
    std::string remotePath = std::string(effective_url) + "/" + encodedFileName;

    // Open the local file for reading
    FILE *fd = fopen((SRC_DIRECTORY + "\\" + SRC_FILENAME).c_str(), "rb");
    if (!fd) {
        std::cerr << "Failed to open file locally." << std::endl;
        curl_free(encodedFileName);
        return;
    }

    // Configure CURL for file upload
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, remotePath.c_str());
    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);

    // Perform the upload
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cout << "File uploaded successfully!" << std::endl;
    }

    // Clean up
    fclose(fd);
    curl_free(encodedFileName);
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
        curl_easy_setopt(curl, CURLOPT_URL, ("sftp://"+REMOTE+"/"+DIRECTORY).c_str()); // Connect to home directory without listing
    
        // Suppress listing or fetching data
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        // Provide user credentials
        curl_easy_setopt(curl, CURLOPT_USERPWD, (USER+":"+PASSWORD).c_str());  // Username with no password
        
        // Specify the private key file for authentication
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, KEYPATH.c_str());

        // Perform the SFTP connection without listing the directory
        res = curl_easy_perform(curl);

        // String for response
        std::string * response = new std::string;;

        // Break response into array
        std::vector<std::string> args;

        // Error handling
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

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
                        newUrl = "sftp://" + REMOTE + newDir; 
                    } else {
                        currentUrl = (currentUrl[currentUrl.size()-1] == '/') ? currentUrl : currentUrl + '/';

                        // Relative path: append to the current URL
                        size_t lastSlash = currentUrl.find_last_of('/');
                        if (lastSlash != std::string::npos) {
                            // Construct the new URL using the current URL and the newDir
                            newUrl = currentUrl.substr(0, lastSlash + 1) + newDir; // Append to the current directory
                        } else {
                            // If at root, just append the newDir
                            newUrl = "sftp://" + REMOTE + newDir; // Set the new directory
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
                    // Check if the first argument is a quote
                    bool isQuoted = args[1][0] == '"' && args[args.size() - 1].back() == '"';

                    std::string newDir;

                    if (isQuoted) {
                        // Combine all parts into a single path string, removing the quotes
                        for (size_t i = 1; i < args.size(); ++i) {
                            newDir += args[i];
                            if (i + 1 < args.size()) {
                                newDir += " ";  // Add space between words
                            }
                        }
                        
                        // Remove the surrounding quotes
                        newDir = newDir.substr(1, newDir.size() - 2);
                    } else {
                        // If no quotes, just take the second argument
                        newDir = args[1];
                    }

                    // Encode the newDir for URL
                    char *encodedDir = curl_easy_escape(curl, newDir.c_str(), 0);

                    // Construct the new URL based on the newDir
                    std::string newUrl;

                    // Check if newDir is an absolute path (starts with '/')
                    if (newDir[0] == '/') {
                        // Absolute path: directly append to sftp://pi
                        newUrl = "sftp://" + REMOTE + encodedDir; 
                    } else {
                        currentUrl = (currentUrl[currentUrl.size() - 1] == '/') ? currentUrl : currentUrl + '/';

                        // Relative path: append to the current URL
                        size_t lastSlash = currentUrl.find_last_of('/');
                        if (lastSlash != std::string::npos) {
                            // Construct the new URL using the current URL and the newDir
                            newUrl = currentUrl.substr(0, lastSlash + 1) + encodedDir; // Append to the current directory
                        } else {
                            // If at root, just append the newDir
                            newUrl = "sftp://" + REMOTE + encodedDir; // Set the new directory
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
                    continue;
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

        
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
