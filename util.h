#pragma once
#include<iostream>
#include<random>
#include<ctime>
#include<string>
#include<stack>
#include<vector>
#include<iomanip>
#include<cstring>
using namespace std;

/*
			  Block Structure of the Filesystem:
 ------------------------------------------------------------
 |SuperBlock|INODE Bitmap|DBlock Bitmap| INODE | Data Block |
 |    1     |      1     |      16     |  16   |    ...     |
 ------------------------------------------------------------
 */

const string SYSTEM_FILE = "my_system.dat";
const int SYSTEM_SIZE = 16 * 1024 * 1024;									   // 16MB total space
const int BLOCK_SIZE = 1 * 1024;											   // 1KB block size
const int BLOCK_NUM = 16 * 1024;											   // 16384 total blocks
const int INODE_NUM = 256;													   // 256 inodes
const int INODE_DIRECT_ADDR = 10;											   // 10 direct block address
const int INODE_INDIRE_ADDR = 1;									   		   // 1 idnirect block address 
const int ADDR_LENGTH = 3;													   // 24-bits address length
const int DEFAULT_MAGIC = 0x53;                                                // default magic num
const int NUM_BLOCK_SUPER_BLOCK = 1;                                           // 1 blcok for super block
const int NUM_BLOCK_INODE_BITMAP = 1;									       // 1 block for inode bitmap
const int NUM_BLOCK_DATA_BITMAP = 16;										   // 16 blocks for data bitmap
const int NUM_BLOCK_INODE = 16;												   // 16 blocks for inode

const int OFFSET_SUPER_BLOCK = 0;                                              // start of super block
const int OFFSET_INODE_BITMAP = OFFSET_SUPER_BLOCK + NUM_BLOCK_SUPER_BLOCK;	   // start of inode bitmap
const int OFFSET_DBLOCK_BITMAP = OFFSET_INODE_BITMAP + NUM_BLOCK_INODE_BITMAP; // start of data block bitmap
const int OFFSET_INODE = OFFSET_DBLOCK_BITMAP + NUM_BLOCK_DATA_BITMAP;	       // start of inodes
const int OFFSET_DBLOCK = OFFSET_INODE + NUM_BLOCK_INODE;				       // start of data block
const int DBLOCK_NUM = BLOCK_NUM - OFFSET_DBLOCK;						       // rest blocks for data blocks

const int DIR_ENTRY_LENGTH = 32;										       // dir entry length
const int NAME_LENGTH = 29;												       // file/Dir name length
const int INODE_ID_LENGTH = DIR_ENTRY_LENGTH - NAME_LENGTH;					   // length of inode id
const int NUM_ENTRY_PER_BLOCK = BLOCK_SIZE / DIR_ENTRY_LENGTH;				   // num of directory entry
const int NUM_IND_TO_DIR = BLOCK_SIZE / INODE_ID_LENGTH;					   // num of indirect to direct (341)
const int INODE_MAX_NUM_BLOCKS = INODE_DIRECT_ADDR + INODE_INDIRE_ADDR * NUM_IND_TO_DIR + INODE_INDIRE_ADDR;
const int MAX_FILE_SIZE = INODE_MAX_NUM_BLOCKS - INODE_INDIRE_ADDR;            // max file size

class INT24                                                                    // INT24_MAX = 8388607
{
protected:
    unsigned char bytes[ADDR_LENGTH];
public:
    INT24() { }
    INT24(const int val) {    *this = val;  }
    INT24(const INT24& val){  *this = val; }

    operator int() const {
        if (bytes[2] & 0x80) // If its this a negative, siingn extend.
            return (0xff << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0);
        else
			return (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0);
    }

    operator float() const {
        return (float)this->operator int();
    }

    INT24& operator =(const INT24& input) {
        bytes[0] = input.bytes[0];
        bytes[1] = input.bytes[1];
        bytes[2] = input.bytes[2];
        return *this;
    }

    INT24& operator =(const int input) {
        bytes[0] = ((unsigned char*)&input)[0];
        bytes[1] = ((unsigned char*)&input)[1];
        bytes[2] = ((unsigned char*)&input)[2];
        return *this;
    }

    INT24 operator + (const INT24& val) const {
        return INT24((int)*this + (int)val);
    }
    INT24 operator - (const INT24& val) const
    {
        return INT24((int)*this - (int)val);
    }
	INT24 operator *(const INT24& val) const
	{
		return INT24((int)*this * (int)val);
	}

    INT24 operator +(const int val) const
    {
        return INT24((int)*this + val);
    }

    INT24 operator -(const int val) const
    {
        return INT24((int)*this - val);
    }
	INT24 operator *(const int val) const
	{
		return INT24((int)*this * val);
	}
};
class SuperBlock {
private:
	int magic = DEFAULT_MAGIC;
	int num_of_inodes = INODE_NUM;
	int num_of_blocks = BLOCK_NUM;
	int num_of_free_blocks;

public:
	int getMagic() {
		return magic;
	}
	void setMagic(int m) {
		magic = m;
	}
	int getFreeBlocks() const {
		return num_of_free_blocks;
	}
	void setFreeBlocks(int n) {
		num_of_free_blocks = n;
	}
	void upFreeBlocks(int n) {
		num_of_free_blocks += n;
	}
	void downFreeBlocks(int n) {
		num_of_free_blocks -= n;
	}
};
class INODE {
public:
	INODE() { }
	INODE(string type, const int size, const char* time_created) {
		setInodeType(type);
		setInodeSize(size);
		setTimeCreated(time_created);
		for (int i = 0; i < INODE_DIRECT_ADDR; i++) setDirectAddr(i, -1);
		for (int i = 0; i < INODE_INDIRE_ADDR; i++) setIndirectAddr(i, -1);
	}
	void setInodeType(const string value) {
		if (value == "FILE")
			type = '0';
		else if (value == "DIR")
			type = '1';
		else
			cout << "Input type error!" << endl;
	}

	string getInodeType() const {
		if (type == '0')
			return "FILE";
		else
			return "DIR";
	}

	void setTimeCreated(const char* tm) {
		memcpy(time_created, tm, 20 * sizeof(char));
	}

	string getTimeCreated() const {
		return time_created;
	}

	int getInodeSize() const {
		return size;
	}

	void setInodeSize(int sz) {
		size = sz;
	}
    
    void setDirectAddr(int idx, int block_id) {
		addr[idx] = block_id;
	}

	int getDirectAddr(int idx) const {
		return addr[idx];
	}

	void setIndirectAddr(int idx, int block_id) {
		if (idx >= INODE_INDIRE_ADDR || idx < 0) return;
		addr[idx + INODE_DIRECT_ADDR] = block_id;
	}

	int getIndirectAddr(int idx) const {
		if (idx >= INODE_INDIRE_ADDR || idx < 0) return -1;
		return addr[idx + INODE_DIRECT_ADDR];
	}

private:
	char type;
	int size;
	char time_created[20];
    INT24 addr[INODE_DIRECT_ADDR + INODE_INDIRE_ADDR];
};

static string getCurrTime() {
	time_t seconds = time(0);
	struct tm now;
	localtime_s(&now, &seconds);
	string time = to_string(now.tm_year + 1900)+ "/" + to_string(now.tm_mon + 1) + "/" + to_string(now.tm_mday) + " "
		        + to_string(now.tm_hour) + ":" + to_string(now.tm_min) + ":" + to_string(now.tm_sec);
	return time;
}

static int getIntFromChar(char chs[3])
{
	char ch_int[3];
	int x = 0;
	memset(ch_int, 0, 3 * sizeof(char));
	memcpy(ch_int, chs, 3 * sizeof(char));
	memcpy(&x, ch_int, 3 * sizeof(char));
	return x;
}

static void getCharFromInt(char chs[3], int x)
{
	char ch[3];
	memset(ch, 0, 3 * sizeof(char));
	memcpy(ch, &x, 3 * sizeof(char));
	memcpy(chs, ch, 3 * sizeof(char));
}
