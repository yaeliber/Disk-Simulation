/*
 * code file for the disk system simulator
 */
#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256

void decToBinary(int n, char &c)
{
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0)
    {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--)
    {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}

// #define SYS_CALL
// ============================================================================
class fsInode
{
    int fileSize;
    int block_in_use;

    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;

public:
    fsInode(int _block_size, int _num_of_direct_blocks)
    {
        fileSize = 0;
        block_in_use = 0;
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
        assert(directBlocks);
        for (int i = 0; i < num_of_direct_blocks; i++)
        {
            directBlocks[i] = -1;
        }
        singleInDirect = -1;
    }

    int GetfileSize()
    {
        return fileSize;
    }
    int GetBlockInUse()
    {
        return block_in_use;
    }
    int plusBlockInUse()
    {
        block_in_use++;
    }
    int GetNumOfDirectBlocks()
    {
        return num_of_direct_blocks;
    }
    int GetBlockSize()
    {
        return block_size;
    }

    int *GetDirectBlocks()
    {
        return directBlocks;
    }

    void SetFileSize(int newSize)
    {
        fileSize = newSize;
    }

    int GetsingleInDirect()
    {
        return singleInDirect;
    }

    void SetsingleInDirect(int single)
    {
        singleInDirect = single;
    }

    ~fsInode()
    {
        delete directBlocks;
    }
};

// ============================================================================
class FileDescriptor
{
    pair<string, fsInode *> file;
    bool inUse;

public:
    FileDescriptor(string FileName, fsInode *fsi)
    {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }

    string getFileName()
    {
        return file.first;
    }

    fsInode *getInode()
    {
        return file.second;
    }

    bool isInUse()
    {
        return (inUse);
    }
    void setInUse(bool _inUse)
    {
        inUse = _inUse;
    }
    void deleteFile()
    {
        inUse = false;
        delete file.second;
        file.second = NULL;
        file.first = "";
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk
{
    FILE *sim_disk_fd;

    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures,
    // each of which contains one filename and one inode number.
    map<string, fsInode *> MainDir;

    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector<FileDescriptor> OpenFileDescriptors;

    int direct_enteris;
    int block_size;

private:
    int numOfDescriptor = 0;
    int numOfBlockInUse = 0;

    int NumOfEmptyBlocks()
    {
        int count = 0;
       for(int i = 0; i < BitVectorSize ; i++)
       {
         if (BitVector[i] == 0)
         count++;
       }
       return count;
    }

public:
    // ------------------------------------------------------------------------
    fsDisk()
    {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++)
        {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }

    // ------------------------------------------------------------------------
    void listAll()
    {
        int i = 0;
        for (auto it = begin(OpenFileDescriptors); it != end(OpenFileDescriptors); ++it)
        {
            cout << "index: " << i << ": FileName: " << it->getFileName() << " , isInUse: " << it->isInUse() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++)
        {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4, int direct_Enteris_ = 3)
    {
        if(is_formated == true)
        {
             cout << "The file is already formatted" << endl;
            return;
        }
        direct_enteris = direct_Enteris_;
        block_size = blockSize;//Assuming the block size is correct

        BitVectorSize = DISK_SIZE / block_size;
        BitVector = (int *)malloc(BitVectorSize * sizeof(int));
        assert(BitVector != NULL);

        for (int i = 0; i < BitVectorSize; i++)
        {
            BitVector[i] = 0;
        }
        is_formated = true;
        cout << "FORMAT DISK: number of blocks: " << BitVectorSize << endl;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName)
    {
        if (is_formated == false) 
        {
             cout << "The disk was not formatted " << endl;
             return -1;
        }
            
        if (MainDir.find(fileName) != MainDir.end())
        {
            cout << "The file is already in the system " << endl;
            return -1;
        }

        if (numOfBlockInUse == BitVectorSize)
        {
           cout << "There are no free blocks on disk " << endl;
           return -1;
        }
        
        else{
            fsInode *temp = new fsInode(block_size, direct_enteris);
            OpenFileDescriptors.push_back(FileDescriptor(fileName, temp));
            MainDir.insert(pair<string, fsInode *>(fileName, temp)); 

            numOfDescriptor++;
            return (numOfDescriptor - 1); 
        }
    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName)
    {
        if (is_formated == false) 
        {
             cout << "The disk was not formatted " << endl;
             return -1;
        }
        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            if (fileName.compare(OpenFileDescriptors[i].getFileName()) == 0)
            {
                if (OpenFileDescriptors[i].isInUse())
                {
                    cout << "The file is already open" << endl;//return -1?
                    return i;
                }
                else
                {
                    OpenFileDescriptors[i].setInUse(true);
                    return i;
                }
            }
        }
        cout << "The file does not exist" << endl;
        return -1;
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd)
    {
        if (is_formated == false) 
        {
             cout << "The disk was not formatted " << endl;
             return "-1";
        }
        if ((fd < 0) || (fd > OpenFileDescriptors.size()))
        { 
            cout << "Invalid index " << endl;
            return "-1";
        }
        if (OpenFileDescriptors[fd].isInUse() == false)
        { 
            cout << "The file is already close" << endl;
            return "-1";
        }
        OpenFileDescriptors[fd].setInUse(false);
        return OpenFileDescriptors[fd].getFileName();
    }
    // ------------------------------------------------------------------------
   int WriteToFile(int fd, char *buf, int len) //void?
    {
        if (is_formated == false) 
        {
             cout << "The disk was not formatted " << endl;
             return -1;
        }
        if ((fd < 0) || (fd > OpenFileDescriptors.size() - 1))
        {
            cout << "There is no such file" << endl;
            return -1;
        }
        FileDescriptor file1 = OpenFileDescriptors[fd];
        int len2 = 0; //Length of what we wrote
        if (file1.isInUse() == false)
        {
            cout << "The file is closed" << endl;
            return -1;
        }
        if (len > (((file1.getInode()->GetNumOfDirectBlocks() + block_size) * block_size) - file1.getInode()->GetfileSize()))
        { //There is no space in the file
            cout << "The file is full" << endl;
            return -1;
        }
        int offSet = file1.getInode()->GetfileSize() % block_size;

        if(len >(NumOfEmptyBlocks()*block_size + (block_size - offSet)))
        {//There is no space in the disk
            cout << "The disk is full(no space for this size of chars..)" << endl;
            return -1;
        }
        int ret_val;
        int *t = file1.getInode()->GetDirectBlocks();
        int numOfBlockInUse1 = file1.getInode()->GetBlockInUse();
        
        if (offSet > 0)
        { //Stop internal fragmentation
            if ((numOfBlockInUse1 < direct_enteris) || (numOfBlockInUse1 == direct_enteris))
            { //in direct blocks
                int index = t[numOfBlockInUse1 - 1];
                ret_val = fseek(sim_disk_fd, (index * block_size) + offSet, SEEK_SET);
            }
            else //in single block
            {
                int index = (file1.getInode()->GetsingleInDirect() * block_size) + numOfBlockInUse1 - 1 - direct_enteris;
                char bufy;
                ret_val = fseek(sim_disk_fd, index, SEEK_SET);
                ret_val = fread(&bufy, 1, 1, sim_disk_fd);
                assert(ret_val == 1);

                ret_val = fseek(sim_disk_fd, (int)bufy*block_size + offSet, SEEK_SET);
            }

            for (int i = block_size - offSet; i > 0 && len2 < len; i--) //Stop internal fragmentation
            {
                ret_val = fwrite(buf + len2, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                len2++;
            }
            fflush(sim_disk_fd);
        }
        for (int i = 0; (i < BitVectorSize) && (len2 < len); i++)
        {//new block

            if (BitVector[i] == 0)
            {
                if (numOfBlockInUse1 < direct_enteris)
                {
                    t[numOfBlockInUse1] = i;

                    ret_val = fseek(sim_disk_fd, i * block_size, SEEK_SET);
                    for (int j = 0; j < block_size && len2 < len; j++)
                    {
                        ret_val = fwrite(buf + len2, 1, 1, sim_disk_fd);
                        assert(ret_val == 1);
                        len2++;
                    }
                    //fflush(sim_disk_fd);
                }
                else
                {
                    if (numOfBlockInUse1 == direct_enteris)
                    {
                        file1.getInode()->SetsingleInDirect(i);
                    }
                    else
                    {
                        char a = '\0';
                        decToBinary(i, a);
                        ret_val = fseek(sim_disk_fd, file1.getInode()->GetsingleInDirect() * block_size + numOfBlockInUse1 - direct_enteris, SEEK_SET);
                        ret_val = fwrite(&a, 1, 1, sim_disk_fd);
                        assert(ret_val == 1);
                        ret_val = fseek(sim_disk_fd, i*block_size, SEEK_SET);

                        for (int j = 0; j < block_size && len2 < len; j++)
                        {
                            ret_val = fwrite(buf + len2, 1, 1, sim_disk_fd);
                            assert(ret_val == 1);
                            len2++;
                        }
                    }
                }

                fflush(sim_disk_fd);
                BitVector[i] = 1;
                file1.getInode()->plusBlockInUse();
                numOfBlockInUse1++;//In a specific file
                numOfBlockInUse++;
            }
        }
        file1.getInode()->SetFileSize(file1.getInode()->GetfileSize() + len2);
    }
    // ------------------------------------------------------------------------
    int DelFile(string FileName)
    {
         if (is_formated == false) 
        {
             cout << "The disk was not formatted " << endl;
             return -1;
        }
        MainDir.erase(FileName);
        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            if (FileName.compare(OpenFileDescriptors[i].getFileName()) == 0)
            {
                if(OpenFileDescriptors[i].isInUse() == true)
                {
                    cout << "Unable to delete open file " << endl;
                    return -1;
                }
                OpenFileDescriptors[i].deleteFile();
                return i;
            }
        }
        cout << "There is no such file" << endl;
        return -1;
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len)
    {
        if (is_formated == false)
        { 
            cout << "The disk was not formatted " << endl;
            buf[0]='\0';
            return -1;
        }
        
        if ((fd < 0) || (fd > OpenFileDescriptors.size() - 1))
        {
            cout << "There is no such file" << endl;
            buf[0]='\0';
            return -1;
        }
        FileDescriptor file1 = OpenFileDescriptors[fd];
        if (file1.isInUse() == false)
        {
            cout << "The file is closed" << endl;
            buf[0]='\0';
            return -1;
        }
        int index,ret_val;
        int fileSize = file1.getInode()->GetfileSize();
        int len2 = 0;
        char t;
         int *direct = file1.getInode()->GetDirectBlocks();
        for(int i = 0 ; i < ((len/block_size) +1) && len2 < len && len2 < fileSize ;i++)
        {
            if(i < direct_enteris)
            {//Direct reading
               index=direct[i];
               ret_val = fseek(sim_disk_fd,index*block_size, SEEK_SET);  
            }
            else
            {
                index=file1.getInode()->GetsingleInDirect() * block_size + i + 1 -direct_enteris;
                ret_val = fseek(sim_disk_fd, index, SEEK_SET);
                ret_val = fread(&t, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                ret_val = fseek(sim_disk_fd, (int)t*block_size , SEEK_SET);
            }
            
            for( int j=0;(j < block_size) && (len2 < len) && (len2 < fileSize); j++)
                 { //Reading at maximum block size each time
                        ret_val = fread(&t, 1, 1, sim_disk_fd);
                        assert(ret_val == 1);
                        buf[len2]=t;
                        len2++;
                 }
        }
        buf[len2]='\0';
        return len2;
    }

    ~fsDisk()
    {
        fclose(sim_disk_fd);
        free(BitVector);
        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            delete OpenFileDescriptors[i].getInode();
        }
    }

};

int main()
{
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while (1)
    {
        cin >> cmd_;
        switch (cmd_)
        {
        case 0: // exit
            delete fs;
            exit(0);
            break;

        case 1: // list-file
            fs->listAll();
            break;

        case 2: // format
            cin >> blockSize;
            cin >> direct_entries;
            fs->fsFormat(blockSize, direct_entries);
            break;

        case 3: // creat-file
            cin >> fileName;
            _fd = fs->CreateFile(fileName);
            cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 4: // open-file
            cin >> fileName;
            _fd = fs->OpenFile(fileName);
            cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 5: // close-file
            cin >> _fd;
            fileName = fs->CloseFile(_fd);
            cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 6: // write-file
            cin >> _fd;
            cin >> str_to_write;
            fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
            break;

        case 7: // read-file
            cin >> _fd;
            cin >> size_to_read;
            fs->ReadFromFile(_fd, str_to_read, size_to_read);
            cout << "ReadFromFile: " << str_to_read << endl;
            break;

        case 8: // delete file
            cin >> fileName;
            _fd = fs->DelFile(fileName);
            cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;
        default:
            break;
        }
    }
}
