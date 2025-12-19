#include <iostream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
// COMMENT BRYAN
#include <unistd.h> // for fork(), exec(), and dup2()
#include <wait.h>   // for waitpid()
#include <fcntl.h>  // for open()
namespace fs = std::filesystem;
int main() {
    std::string input;
    std::vector<std::string> history;
    std::cout << "Welcome to ElbeeShell" << std::endl;
    // Initialize working directory
    fs::path workingDir = fs::current_path();
    chdir(workingDir.c_str()); //Change process' working directory

    while(true){
        std::cout << workingDir << " $ ";
        std::getline(std::cin, input);
        //Exit Function
        if(input == "exit"){
            break;
        }
        //Print Working Directory
        else if(input == "workdir"){
            std::cout << "Working directory: " <<  workingDir << std::endl;
            history.push_back(std::move(input)); //Add to command history
        }
        //Change Directory function
        else if(input.substr(0, 2) == "cd"){
           std::string pathInput = input.substr(3);

           fs::path newPath;
           if(pathInput[0] == '/'){
            newPath = fs::path(pathInput);
           }
           else{
            newPath = workingDir / pathInput;
           }

           if(fs::exists(newPath) && fs::is_directory(newPath)){
            workingDir = fs::canonical(newPath);
            chdir(workingDir.c_str());  //Change process' working directory
            history.push_back(std::move(input)); //Add to command history
           }
           else{
            std::cout << "Path does not exist!" << std::endl;
           }
        }
        //List Function
        else if(input == "list"){
            std::vector<fs::path> directories;
            std::vector<fs::path> files;
            
            //checks if file is either a directory or a regular file and pushes them into the correct vector
            for(const auto& entry : fs::directory_iterator(workingDir)) {
                if(fs::is_directory(entry)){
                    directories.push_back(entry.path().filename()); 
                }
                else if(fs::is_regular_file(entry)){
                    files.push_back(entry.path().filename());
                }
            }
            //sorts the files
            std::sort(directories.begin(), directories.end());
            std::sort(files.begin(), files.end());

            //prints files in order of directories first then normal files
            for(const auto& entryd : directories){
                std::cout << entryd.string() << " (Directory) "  << std::endl;
            }
            for(const auto& entryf : files){
                std::cout << entryf.string() << " (Directory) " << std::endl;
            }
            history.push_back(std::move(input)); //Add to command history
        }
        //History Function
        else if(input == "history"){
            for(auto itr = history.rbegin(); itr != history.rend(); ++itr){ 
                std::cout << *itr << std::endl; //prints previous commands in reverse order
            }
        }
        else{
            //External Commands
            std::string commandPart = input;
            std::string outFile;
            //Output Redirection Check
            size_t redirect = input.find('>');
            if(redirect != std::string::npos){
                //seperates the command on the left of > and output file on the right of >
                commandPart = input.substr(0, redirect);
                outFile = input.substr(redirect + 1);
                
                //Gets rid of the whitespace on the left and right of both sides
                commandPart.erase(0, commandPart.find_first_not_of(" \t"));
                commandPart.erase(commandPart.find_last_not_of(" \t") + 1);
                outFile.erase(0, outFile.find_first_not_of(" \t"));
                outFile.erase(outFile.find_last_not_of(" \t") + 1);
            }
            //Specifying Arguments
            std::istringstream parse(commandPart);
            std::string programpath;
            std::vector<std::string> programArguments;
            std::string arg;   
            
            parse >> programpath;
            fs::path path = programpath;

            while(parse >> arg){
                if(!arg.empty()){
                    programArguments.push_back(arg);
                }
            }
            //Dynamic Array used for execv
            char** cargs {new char*[programArguments.size() + 2]};
            cargs[0] = programpath.data();
            
            for(size_t i = 0; i < programArguments.size(); ++i){
                cargs[i + 1] = programArguments[i].data();
            }
            cargs[programArguments.size() + 1] = nullptr;

            //Fork and Execute Child Process
            int cid {fork()};
            if(cid == 0){
                chdir(workingDir.c_str());  
                if(!outFile.empty()){
                    int outputFile = open(outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); 
                    if(outputFile == -1){
                        std::cout << "Cannot open output file" << std::endl;
                        return 1;
                    }
                    if(dup2(outputFile, STDOUT_FILENO) == -1 || dup2(outputFile, STDERR_FILENO) == -1){
                        std::cout << "Failed to redirect output to file" << std::endl;
                        close(outputFile);
                        return 1;
                    }
                    close(outputFile);
                }
                if (execv(path.c_str(), cargs) == -1){
                    std::cout << "Program cannot be found" << std::endl;
                    return 1;
                }
            }
            //Fork and Execute Parent Process
            else{
                int statusCode;
                waitpid(cid, &statusCode, 0);
                if(statusCode == 0){
                    history.push_back(std::move(input));
                }
                else{
                    std::cout << "Failed with code " << statusCode << std::endl;
                }
            }
            //Clear memory
            delete[] cargs;
        }
    }
    return 0;
}
