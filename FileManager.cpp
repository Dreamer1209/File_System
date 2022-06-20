#include "FileManager.h"

FileManager::FileManager()
{
	curr_inode = 0;
}

FileManager::~FileManager()
{
}

void FileManager::format() {
	disk.format();
	curr_inode = 0;
}

vector<string> FileManager::processPath(string path)
{
	vector<string> splited;
	if (path.size() == 0) return splited;
	if (path[0] == '/') splited.push_back("/");
	char* path_c = new char[path.size() + 1];
	strcpy_s(path_c, path.size() + 1, path.c_str());
	char* buf;
	char* p = strtok_s(path_c, "/", &buf);
	while (p) {
		if (strcmp(p, ".")) splited.push_back(p);
		p = strtok_s(NULL, "/", &buf);
	}
	delete[] path_c;
	return splited;
}

bool FileManager::getINodeMap(int dir_inode_id, unordered_map<string, int>& inode_map)
{
	INODE node = disk.inodes[dir_inode_id];
	if (node.getInodeType() != "DIR") return false;
	inode_map.clear();
	vector<int> blocks_id = getINodeBlocks(dir_inode_id);
	for (int block_id : blocks_id)
		getINodeMapInBlock(block_id, inode_map);
	return true;
}

void FileManager::getINodeMapInBlock(int block_id, unordered_map<string, int>& inode_map)
{
	if (block_id == -1) return;
	int i = 0;
	char* p = disk.getDBlockAddr(block_id);
	while (*p != EOF && i < NUM_ENTRY_PER_BLOCK) {
		char name[NAME_LENGTH + 1];
		memcpy(name, p, NAME_LENGTH);
		name[NAME_LENGTH] = '\0';
		char id_c[INODE_ID_LENGTH];
		memcpy(id_c, p + NAME_LENGTH, INODE_ID_LENGTH);
		int id = getIntFromChar(id_c);
		inode_map[name] = id;
		i++;
		p += DIR_ENTRY_LENGTH;
	}
}

bool FileManager::setINodeMap(int dir_inode_id, const unordered_map<string, int>& inode_map)
{
	INODE& dir_inode = disk.inodes[dir_inode_id];
	if (dir_inode.getInodeType() != "DIR") return false;
	releaseBlocks(dir_inode_id);
	vector<int> blocks_id;
	auto iter = inode_map.begin();
	while (iter != inode_map.end()) {
		int block_id = disk.getFreeDBlockID();
		disk.setDBlockBitmap(block_id, 1);
		blocks_id.push_back(block_id);
		char* p = disk.getDBlockAddr(block_id);
		int i;
		for (i = 0; i < NUM_ENTRY_PER_BLOCK && iter != inode_map.end(); i++) {
			memcpy(p, (iter->first).c_str(), NAME_LENGTH);
			char id_c[INODE_ID_LENGTH];
			getCharFromInt(id_c, iter->second);
			memcpy(p + NAME_LENGTH, id_c, INODE_ID_LENGTH);
			++iter;
			p += DIR_ENTRY_LENGTH;
		}
		if (i < NUM_ENTRY_PER_BLOCK) {
			*p = EOF;
			break;
		}
	}
	setINodeBlocks(dir_inode_id, blocks_id);
	return true;
}

void FileManager::addEntry(int dir_inode_id, const char* name, int inode_id)
{
	unordered_map<string, int> inode_map;
	getINodeMap(dir_inode_id, inode_map);
	inode_map[name] = inode_id;
	setINodeMap(dir_inode_id, inode_map);
}

void FileManager::removeEntry(int dir_inode_id, const char* name)
{
	unordered_map<string, int> inode_map;
	getINodeMap(dir_inode_id, inode_map);
	inode_map.erase(name);
	setINodeMap(dir_inode_id, inode_map);
}

void FileManager::randFillBlock(int block_id, int size)
{
	char* block = disk.getDBlockAddr(block_id);
	size = min(size, BLOCK_SIZE);
	char* rand_str = new char[size];
	for (int i = 0; i < size; i++)
		rand_str[i] = 32 + (rand() % (127 - 32));
	memcpy(block, rand_str, size);
}

string FileManager::getINodeData(int inode_id)
{
	string str;
	const INODE& node = disk.inodes[inode_id];
	vector<int> blocks_id = getINodeBlocks(inode_id);
	for (int i = 0; i < blocks_id.size(); i++) {
		int block_id = blocks_id[i];
		char* p = disk.getDBlockAddr(block_id);
		char buf[BLOCK_SIZE + 1];
		memcpy(buf, p, BLOCK_SIZE);
		buf[BLOCK_SIZE] = '\0';
		str += buf;
	}
	return str;
}

void FileManager::releaseBlocks(int inode_id)
{
	INODE& inode = disk.inodes[inode_id];
	SuperBlock* superblock = disk.getSuperBlock();
	for (int i = 0; i < INODE_DIRECT_ADDR; i++) {
		int block_id = inode.getDirectAddr(i);
		if (block_id == -1) continue;
		disk.setDBlockBitmap(block_id, 0);
		inode.setDirectAddr(i, -1);
		superblock->upFreeBlocks(1);
	}
	for (int i = 0; i < INODE_INDIRE_ADDR; i++) {
		int ind_block_id = inode.getIndirectAddr(i);
		if (ind_block_id == -1) continue;
		char* p = disk.getDBlockAddr(ind_block_id);
		for (int i = 0; i < NUM_IND_TO_DIR && *p != EOF; i++) {
			char id_c[INODE_ID_LENGTH];
			memcpy(id_c, p, INODE_ID_LENGTH);
			int block_id = getIntFromChar(id_c);
			disk.setDBlockBitmap(block_id, 0);
			superblock->upFreeBlocks(1);
			p += INODE_ID_LENGTH;
		}
		disk.setDBlockBitmap(ind_block_id, 0);
		inode.setIndirectAddr(i, -1);
		superblock->upFreeBlocks(1);
	}
}

bool FileManager::setINodeBlocks(int inode_id, const vector<int>& blocks_id)
{
	int n = blocks_id.size();
	if (n > MAX_FILE_SIZE) return false;
	releaseBlocks(inode_id);
	INODE& inode = disk.inodes[inode_id];
	SuperBlock* superblock = disk.getSuperBlock();
	int idx = 0;
	for (int i = 0; i < min(n, INODE_DIRECT_ADDR); i++) {
		inode.setDirectAddr(i, blocks_id[idx]);
		disk.setDBlockBitmap(blocks_id[idx++], 1);
		superblock->downFreeBlocks(1);
	}
	for (int i = 0; i < INODE_INDIRE_ADDR; i++) {
		if (idx == n) return true;
		int indirect_block_id = disk.getFreeDBlockID();
		disk.setDBlockBitmap(indirect_block_id, 1);
		inode.setIndirectAddr(i, indirect_block_id);
		superblock->downFreeBlocks(1);
		char* p = disk.getDBlockAddr(indirect_block_id);
		for (int j = 0; j < NUM_IND_TO_DIR; j++) {
			if (idx == n) {
				*p = EOF;
				return true;
			}
			disk.setDBlockBitmap(blocks_id[idx], 1);
			superblock->downFreeBlocks(1);
			char id_c[INODE_ID_LENGTH];
			getCharFromInt(id_c, blocks_id[idx++]);
			memcpy(p, id_c, INODE_ID_LENGTH);
			p += INODE_ID_LENGTH;
		}
	}
	return true;
}

vector<int> FileManager::getINodeBlocks(int inode_id) //find blocks corresponding to the inode
{
	vector<int> blocks;
	const INODE& inode = disk.inodes[inode_id];
	for (int i = 0; i < INODE_DIRECT_ADDR; i++) {
		int direct_block_id = inode.getDirectAddr(i);
		if (direct_block_id == -1) continue;
		blocks.push_back(direct_block_id);
	}
	for (int i = 0; i < INODE_INDIRE_ADDR; i++) {
		int indirect_block_id = inode.getIndirectAddr(i);
		if (indirect_block_id == -1) continue;
		char* p = disk.getDBlockAddr(indirect_block_id);
		for (int i = 0; i < NUM_IND_TO_DIR && *p != EOF; i++) {
			char id_c[INODE_ID_LENGTH];
			memcpy(id_c, p, INODE_ID_LENGTH);
			int block_id = getIntFromChar(id_c);
			blocks.push_back(block_id);
			p += INODE_ID_LENGTH;
		}
	}
	return blocks;
}

int FileManager::getInode(const vector<string>& path)
{
	int n = path.size();
	if (n == 0) return curr_inode;
	int i = 0;
	int node_id = curr_inode;
	if (path[0] == "/") {
		node_id = 0;
		i++;
	}
	while (i < n) {
		unordered_map<string, int> inode_map;
		if (!getINodeMap(node_id, inode_map) || !inode_map.count(path[i])) return -1;
		node_id = inode_map[path[i]];
		i++;
	}
	return node_id;
}

void FileManager::createFile(const char* file_name, const int file_size)
{
	vector<string> path = processPath(file_name);
	if (getInode(path) != -1) {
		cout << "cannot createFile '" << file_name << "': File already exists";
		return;
	}
	if (path.size() == 0) return;
	string name = path.back();
	path.pop_back();
	int dir_inode_id = getInode(path);
	if (dir_inode_id == -1) {
		cout << "cannot createFile '" << file_name << "': No such file or directory";
		return;
	}

	if (file_size > MAX_FILE_SIZE) {
		cout << "cannot createFile '" << file_name << "': The file is too large: at most " << MAX_FILE_SIZE << "KB";
		return;
	}
	if (file_size > disk.getSuperBlock()->getFreeBlocks()) {
		cout << "cannot createFile '" << file_name << "': Insufficient space";
		return;
	}
	int inode_id = disk.getFreeInodeID();
	if (inode_id == -1) {
		cout << "cannot createFile '" << file_name << "': The number of file/directory reaches the limit";
		return;
	}
	INODE& inode = disk.inodes[inode_id];
	inode = INODE("FILE", file_size, getCurrTime().c_str());
	addEntry(dir_inode_id, name.c_str(), inode_id);
	disk.setInodeBitmap(inode_id, 1);
	vector<int> blocks_id(file_size);
	srand((unsigned)time(NULL));
	for (int i = 0; i < file_size; i++) {
		int block_id = disk.getFreeDBlockID();
		disk.setDBlockBitmap(block_id, 1);
		blocks_id[i] = block_id;
		randFillBlock(block_id, BLOCK_SIZE);
	}
	setINodeBlocks(inode_id, blocks_id);
}

void FileManager::deleteFile(const char* file_name)
{
	vector<string> path = processPath(file_name);
	int file_inode_id = getInode(path);
	if (file_inode_id == -1) {
		cout << "cannot deleteFile '" << file_name << "': No such file or directory";
		return;
	}
	if (disk.inodes[file_inode_id].getInodeType() == "DIR") {
		cout << "cannot deleteFile '" << file_name << "': Is a directory";
		return;
	}
	releaseBlocks(file_inode_id);
	disk.setInodeBitmap(file_inode_id, 0);
	string name = path.back();
	path.pop_back();
	int dir_inode_id = getInode(path);
	removeEntry(dir_inode_id, name.c_str());
}

void FileManager::createDir(const char* dir_name)
{
	vector<string> path = processPath(dir_name);
	if (getInode(path) != -1) {
		cout << "cannot createDir '" << dir_name << "': file exists";
		return;
	}
	if (path.size() == 0) return;
	string name = path.back();
	path.pop_back();
	int dir_prt_node_id = getInode(path);
	if (dir_prt_node_id == -1) {
		cout << "cannot createDir '" << dir_name << "': No such file or directory";
		return;
	}
	int inode_id = disk.getFreeInodeID();
	int block_id = disk.getFreeDBlockID();
	if (inode_id == -1 || block_id == -1) {
		cout << "cannot createDir '" << dir_name << "': The number of file/directory reaches the limit";
		return;
	}
	INODE& inode = disk.inodes[inode_id];
	inode = INODE("DIR", -1, getCurrTime().c_str());
	addEntry(dir_prt_node_id, name.c_str(), inode_id);
	disk.setInodeBitmap(inode_id, 1);
	addEntry(inode_id, "..", dir_prt_node_id);
	disk.setDBlockBitmap(block_id, 1);
}

void FileManager::deleteDir(const char* dir_name)
{
	vector<string> path = processPath(dir_name);
	stack<string> currentstack = getCurrentPath();
	vector<string> currentpath;
	while (!currentstack.empty()) {
		currentpath.push_back(currentstack.top());
		currentstack.pop();
	}
	for (int i = 1; i < path.size()-1; i++){
		if (path[i] == currentpath[i-1]) {
			cout << "cannot delete parent directory!\n";
			return;
		}
	}
	if (path.size() == 0 || (path.size() == 1 && path[0] == "..")) {
		cout << "cannot operate '.' or '..'";
		return;
	}
	int dir_inode_id = getInode(path);
	if (dir_inode_id == 0) {
		cout << "cannot delete /";
		return;
	}
	if (dir_inode_id == -1) {
		cout << "cannot deleteDir '" << dir_name << "': No such file or directory";
		return;
	}
	INODE& dir_inode = disk.inodes[dir_inode_id];
	if (dir_inode.getInodeType() != "DIR") {
		cout << "cannot deleteDir '" << dir_name << "': Is not a directory";
		return;
	}
	unordered_map<string, int> inode_map;
	getINodeMap(dir_inode_id, inode_map);
	int prt_inode_id = inode_map[".."];
	inode_map.erase("..");
	for (auto iter = inode_map.begin(); iter != inode_map.end(); iter++) {
		INODE& inode = disk.inodes[iter->second];
		string name = dir_name;
		name += "/" + iter->first;
		if (inode.getInodeType() == "FILE") deleteFile(name.c_str());
		else deleteDir(name.c_str());
	}
	releaseBlocks(dir_inode_id);
	disk.setInodeBitmap(dir_inode_id, 0);
	removeEntry(prt_inode_id, path.back().c_str());
}

void FileManager::changeDir(const char* dir_name)
{
	vector<string> path = processPath(dir_name);
	if (dir_name == "..") {
		curr_inode = 0;
		return;
	}
	int inode_id = getInode(path);
	if (inode_id == -1) {
		cout << "cannot changeDir '" << dir_name << "': No such file or directory";
		return;
	}
	if (disk.inodes[inode_id].getInodeType() != "DIR") {
		cout << "cannot changeDir '" << dir_name << "': Not a directory";
		return;
	}
	curr_inode = inode_id;
}

void FileManager::copyFile(const char* file_1, const char* file_2)
{
	vector<string> path1 = processPath(file_1);
	int file_inode_id1 = getInode(path1);
	if (file_inode_id1 == -1) {
		cout << "cannot cp '" << file_1 << "': No such file or directory";
		return;
	}
	const INODE& inode1 = disk.inodes[file_inode_id1];
	int file_size = inode1.getInodeSize();
	if (inode1.getInodeType() == "DIR") {
		cout << "cannot cp '" << file_1 << "': Is a directory";
		return;
	}
	vector<string> path2 = processPath(file_2);
	if (getInode(path2) != -1) {
		cout << "cannot cp '" << file_1 << "' to '" << file_2 << "': File exists";
		return;
	}
	if (path2.size() == 0) return;
	string name2 = path2.back();
	path2.pop_back();
	int dir_node_id = getInode(path2);
	if (dir_node_id == -1) {
		cout << "cannot cp '" << file_1 << "' to '" << file_2 << "': No such file or directory";
		return;
	}
	if (file_size > disk.getSuperBlock()->getFreeBlocks()) {
		cout << "cannot cp '" << file_1 << "': Insufficient space";
		return;
	}

	int inode_id2 = disk.getFreeInodeID();
	if (inode_id2 == -1) {
		cout << "cannot cp '" << file_1 << "': The number of file/directory reaches the limit";
		return;
	}
	INODE& inode2 = disk.inodes[inode_id2];
	inode2 = INODE("FILE", file_size, getCurrTime().c_str());
	addEntry(dir_node_id, name2.c_str(), inode_id2);
	disk.setInodeBitmap(inode_id2, 1);
	vector<int> blocks1_id = getINodeBlocks(file_inode_id1);
	vector<int> blocks2_id(file_size);
	for (int i = 0; i < file_size; i++) {
		int block1_id = blocks1_id[i];
		int block2_id = disk.getFreeDBlockID();
		disk.setDBlockBitmap(block2_id, 1);
		blocks2_id[i] = block2_id;
		char* p1 = disk.getDBlockAddr(block1_id);
		char* p2 = disk.getDBlockAddr(block2_id);
		memcpy(p2, p1, BLOCK_SIZE);
	}
	setINodeBlocks(inode_id2, blocks2_id);
	return;
}

stack<string> FileManager::getCurrentPath()
{
	stack<string> path_stack;
	int inode_id = curr_inode;
	int prt_inode_id;
	while (inode_id != 0) {
		const INODE& inode = disk.inodes[curr_inode];
		unordered_map<string, int> inode_map;
		getINodeMap(inode_id, inode_map);
		prt_inode_id = inode_map[".."];
		getINodeMap(prt_inode_id, inode_map);
		for (auto iter = inode_map.begin(); iter != inode_map.end(); ++iter) {
			if (iter->second == inode_id) {
				path_stack.push(iter->first);
				break;
			}
		}
		inode_id = prt_inode_id;
	}
	return path_stack;
}

void FileManager::dir()
{
	vector<string> name_list;
	vector<string> type_list;
	vector<string> size_list;
	vector<string> time_created_list;
	unordered_map<string, int> inode_map;
	getINodeMap(curr_inode, inode_map);
	for (auto iter = inode_map.begin(); iter != inode_map.end(); ++iter) {
		name_list.push_back(iter->first);
		const INODE& inode = disk.inodes[iter->second];
		type_list.push_back(inode.getInodeType());
		time_created_list.push_back(inode.getTimeCreated());
		int size = inode.getInodeSize();
		if (size == -1)  size_list.push_back("----");
		else size_list.push_back(to_string(inode.getInodeSize()) + "KB");
	}
	cout << left << setw(NAME_LENGTH) << "Name" << setw(12) << "Type" << setw(12) << "Size" << "Time Created" << endl;
	for (int i = 1; i <= 72; ++i) cout << "-";
	cout << endl;
	for (int i = 0; i < name_list.size(); ++i)
		cout << left << setw(NAME_LENGTH) << name_list[i] << setw(12) << type_list[i] << setw(12) << size_list[i] << time_created_list[i] << endl;
}

void FileManager::printFile(const char* file_name)
{
	vector<string> path = processPath(file_name);
	int inode_id = getInode(path);
	if (inode_id == -1) {
		cout << "cannot cat '" << file_name << "': No such file or directory";
		return;
	}
	if (disk.inodes[inode_id].getInodeType() == "DIR") {
		cout << "cannot cat '" << file_name << "': Is a directory";
		return;
	}
	cout << getINodeData(inode_id);
}

void FileManager::sum()
{
	int unused_cnt = disk.getSuperBlock()->getFreeBlocks();
	int used_cnt = DBLOCK_NUM - unused_cnt;
	double usage = (double)(used_cnt + OFFSET_DBLOCK) / (double)BLOCK_NUM;
	int total = 25 * usage;
	cout << endl << "Space usage: [";
	for (int i = 0; i < 25; ++i) {
		if (i < total) cout << "=";
		else cout << " ";
	}
	cout << setprecision(3) << "] " << usage * 100 << " %  " << usage * 16 << " MB / " << "16 MB" << endl;
	cout << left << "Used blocks: " << setw(6) << used_cnt << "Unused blocks: " << setw(6) << unused_cnt << endl;
}

