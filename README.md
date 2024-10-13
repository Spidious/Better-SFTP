# Better SFTP

This was a quick project that I built to simplify the process of transfering files between my PC and my Raspberry Pi.

## Installation

Installing the application requires setup of an SSH key.
If you choose password authentication you can change the connection method to use your password.
***Installation of a C/C++ compiler like MinGW is Required***
***Installation of Libcurl is required***

### SSH Keygen

I have found this [github page](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent) to being particularly helpful in generating an SSH Key
*Currently this only works WITHOUT a password*

For this application to work your remote must accept SSH key verification with your public key:

1. run  `nano ~/.ssh/authorized_keys` on the remote.
2. Paste the contents of your public key `.pub` that you just generated directly into the file.
3. Save and exit the file and then exit the server. When you ssh back into the remote it should just connect right away.

### Adding the application to the context menu

This application was designed with the idea of right clicking the desired application to select and automatically connect to an sftp accessible server.

1. Create application folder

   - Create a folder wherever you would like to place the app.
   - For simplicity I will refer to this as *AppFolder*
   - Move `main.cpp` into this folder.
2. Create the batch file
   Inside of the folder you just created (*AppFolder*), create a text file and paste this.

   ```text
   @echo off
   set FILE_PATH=%~1

   "<AppFolder>/sftp.exe" "%FILE_PATH%"
   ```

   *Replacing `<AppFolder>`*

   Save the file as `launchSFTP.bat`

3) Modify the Windows Registry

   Open Registry Editor: Press Win + R, type regedit, and press Enter.

   Navigate to File Types: Go to the following path, depending on the type of file you want to add the option to:
   For all files, navigate to:

   `HKEY_CLASSES_ROOT\*\shell`

   - Create a New Key:

     Right-click on shell, select New > Key, and name it something like LaunchSFTP.
   - Set the Display Name:

     In the right pane, double-click on (Default) and set its value to something like Upload via SFTP.
   - Add a Command:

     Right-click on the new LaunchSFTP key you created, select New > Key, and name it command.
     In the right pane, double-click on (Default) and set its value to:

     `"<AppFolder>/launchSFTP.bat" "%1"`
4) Testing

   - Now, right-click any file in Windows Explorer.
   - You should see the option "Upload via SFTP" in the context menu. (You may have to select "show more options")
   - When you click it, your script should launch, receiving the selected file's path as an argument.

### Updating main.cpp

In the main function, the connection parameters are set using the `curl_easy_setopt` methods.
*You must make sure you have installed a method to compile C++ code (msys2 with mingw) and installed libcurl*

1. Change the configuration at the top of the `main.cpp` file
2. Compile the code `g++ -o sftp.exe main.cpp -lcurl`

##### Install libcurl

This assumes you are  using MSYS2 with MinGW

```plaintext
pacman -Syu
pacman -S mingw-w64-x86_64-curl
```

### Notes

Additional notes about installing and modifying the application

#### Context Menu

Inside the registry editor you can add extra features such as an icon.
See other installed applications for reference
