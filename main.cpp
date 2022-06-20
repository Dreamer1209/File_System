#include "FileManager.h"
#include <iostream>
#include <cstdio>
#include <sstream>
#include <cstdlib>

bool checkCommandLength(vector<string>& input, const int expected_length)
{
	const int input_length = input.size();
	if (input_length != expected_length)
	{
		cout << input[0] << ": Expected " << expected_length - 1 << " parameters, but given " << input_length - 1 << endl;
		return false;
	}
	return true;
}
bool checkshort(string opname, string target)
{
	if (opname.size() < 4)
		return 0;
	if (opname.substr(opname.size() - 3, 3) == "Dir" || opname.substr(opname.size() - 3, 3) == "dir") {
		if (opname[0] == 'c' && opname[1] == 'r')
			return "createDir" == target;
		else if (opname[0] == 'c' && opname[1] == 'h')
			return "changeDir" == target;
		else if (opname[0] == 'd')
			return "deleteDir" == target;
	}
	if (opname.substr(opname.size() - 4, 4) == "File" || opname.substr(opname.size() - 4, 4) == "file") {
		if (opname[0] == 'c')
			return "createFile" == target;
		else if (opname[0] == 'd')
			return "deleteFile" == target;
	}
	return 0;
}

void printInterface()
{
	cout << "+-----------------------------------------------------------------------+" << endl;
	cout << "|                      Unix-Style File System                           |" << endl;
	cout << "|                                                                       |" << endl;
	cout << "|-----------------------------------------------------------------------|" << endl;
	cout << "|            1. Create a file: createFile <fileName> <fileSize>         |" << endl;
	cout << "|            2. Delete a file: deleteFile <fileName>                    |" << endl;
	cout << "|            3. Create a directory: createDir <dirName>                 |" << endl;
	cout << "|            4. Delete a directory: deleteDir <dirName>                 |" << endl;
	cout << "|            5. Change directory: changeDir <dirName>                   |" << endl;
	cout << "|            6. Back to parent directory: ..                            |" << endl;
	cout << "|            7. List all the files: dir                                 |" << endl;
	cout << "|            8. Copy a file: cp <exsitedFile> <newFile>                 |" << endl;
	cout << "|            9. Display space usage: sum                                |" << endl;
	cout << "|            10. Print file content: cat <filename>                     |" << endl;
	cout << "|            11. Format the file system: format                         |" << endl;
	cout << "|            12. Show command menu: help                                |" << endl;
	cout << "|            13. Exit the system: exit                                  |" << endl;
	cout << "+-----------------------------------------------------------------------+" << endl;
}

int main()
{
	srand(time(0));
	FileManager fm;
	printInterface();
	vector<string> str;
	string input;
	string currentDir = "/";
	cout << endl;
	while (true) {
		stack<string> path = fm.getCurrentPath();
		currentDir = path.empty() ? "/" : path.top();
		vector<string> currentpath;
		while (!path.empty()) {
			currentpath.push_back(path.top());
			path.pop();
		}
		cout << "[root@MyFilesystem] " << "/";
		for (int i = 0; i < currentpath.size(); i++) cout << currentpath[i] << "/";
		cout << " $ ";
		getline(cin, input);
		istringstream input_stream(input);
		string item;
		while (input_stream >> item) str.push_back(item);
		if (str.empty()) continue;
		else if (str[0] == "help") {
			printInterface();
		}
		else if (str[0] == "exit") {
			cout << "GoodBye!" << endl;
			break;
		}
		else if (str[0] == "dir") {
			fm.dir();
		}
		else if (str[0] == "..")
		{
			stack<string> currentPath = fm.getCurrentPath();
			if (currentPath.empty()) {
				cout << " parent directory is not exit " << endl;
			}
			else if (currentPath.size() == 1) {
				currentDir = "/";
				fm.changeDir(currentDir.data());
			}
			else {
				string parentpath = "/";
				string curDir;
				while (currentPath.size() > 2) {
					parentpath += currentPath.top() + "/";
					currentPath.pop();
				}
				parentpath += currentPath.top();
				fm.changeDir(parentpath.data());
				currentDir = currentPath.top();
			}
		}
		else if (checkshort(str[0], "changeDir")) {
			if (checkCommandLength(str, 2))
			{
				fm.changeDir(str[1].data());
				stack<string> currentPath = fm.getCurrentPath();
				string curDir;
				if (currentPath.empty())
				{
					curDir = "/";
				}
				else
				{
					while (!currentPath.empty())
					{
						curDir = currentPath.top();
						currentPath.pop();
					}
				}
				currentDir = curDir;
			}
		}
		else if (checkshort(str[0], "createFile")) {
			if (checkCommandLength(str, 3))
			{
				if (str[1] == ".." || str[1] == "/..")
					cout << " .. cannot be file name!";
				else
					fm.createFile(str[1].data(), atoi(str[2].data()));
			}
		}
		else if (checkshort(str[0], "deleteFile")) {
			if (checkCommandLength(str, 2)) {
				fm.deleteFile(str[1].data());
			}
		}
		else if (checkshort(str[0], "createDir")) {
			if (checkCommandLength(str, 2)) {
				if (str[1] == ".." || str[1] == "/..")
					cout << " .. cannot be directory name!";
				else
					fm.createDir(str[1].data());
			}
		}
		else if (checkshort(str[0], "deleteDir")) {
			if (checkCommandLength(str, 2))
				fm.deleteDir(str[1].data());
		}
		else if (str[0] == "cp") {
			if (checkCommandLength(str, 3))
				fm.copyFile(str[1].data(), str[2].data());
		}
		else if (str[0] == "sum") {
			fm.sum();
		}
		else if (str[0] == "cat") {
			if (checkCommandLength(str, 2))
				fm.printFile(str[1].data());
		}
		else if (str[0] == "format") {
			cout << "All data will be wiped after formatting, continue? [y/n] ";
			char yn[2];
			cin.getline(yn, 2);
			if (yn[0] == 'y') {
				fm.format();
				cout << "Formatting is done.";
			}
		}
		else {
			cout << "-bash: " << str[0] << ": Command not found" << endl;
		}
		str.clear();
		cout << endl;
	}
	return 0;
}

