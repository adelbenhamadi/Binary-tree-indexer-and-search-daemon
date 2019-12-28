//randacc.cpp
//Andrew Howard
//2015-04-02
//Random-access I/O - Makes a file that stores 100 records, each holding
//an integer. The user can read and write the integers stored in each of
//the records.

//Quick-n-dirty Memory mapping application
//=======================
//As the name suggests, this is a very simplistic application that deals with mapping memory to a file. You can //run the program multiple times and access the same array.

#include <iostream>
using std::cout;
using std::cin;
using std::endl;

#include <unistd.h> //for open, creat, write, close, _exit
#include <fcntl.h> //for O_RDWR
#include <sys/mman.h> //for mmap and munmap

const size_t MEMSIZE = 100 * sizeof(int); //==400 bytes
int * records;

void readRecord()
{
    int index;
    cout << "Please enter the index of a record to read (0 to 99): ";
    cin >> index;

    while (index < 0 || index > 99)
    {
        cout << "Invalid index. Please enter an index from 0 to 99: ";
        cin >> index;
    }

    cout << "Index " << index << " contains the number " << records[index] << endl;
}

void writeRecord()
{
    int index, toStore;
    cout << "Please enter the index of a record to write to (0 to 99): ";
    cin >> index;

    while (index < 0 || index > 99)
    {
        cout << "Invalid index. Please enter an index from 0 to 99: ";
        cin >> index;
    }

    cout << "Enter the integer you wish to store in index " << index << ": ";
    cin >> toStore;

    cout << "Storing the number " << toStore << " in index " << index <<"... ";
    records[index] = toStore;
    cout << "Done." << endl;
}

bool doMenu()
{
    int choice;

    cout << "----------------------------------------" << endl;
    cout << "Welcome! Please select an option:" << endl;
    cout << "1. Read a record" << endl;
    cout << "2. Write a record" << endl;
    cout << "3. Quit" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Your choice: ";

    while (true)
    {
        cin >> choice;
        switch (choice)
        {
            case 1:
                readRecord();
                return false;
                break;

            case 2:
                writeRecord();
                return false;
                break;

            case 3:
                cout << "Quitting." << endl;
                return true;
                break;

            default:
                cout << "Please select option 1, 2, or 3: ";
                break;

        }
    }
}

int main()
{
    //Open the file
    int fileDesc = open("records.txt", O_RDWR, 0644);

    if (fileDesc == -1)
    {
       //Create and initialize a new file
       cout << "Could not open the records file. Attempting to initialize a new file..." << endl;
       fileDesc = creat("records.txt", 0644);

       if (fileDesc == -1)
       {
           cout << "ERROR: could not open or create the records file." << endl;
           _exit(1);
       }

       size_t dummy;
       for (size_t i = 0; i <= MEMSIZE; ++i)
            dummy = write(fileDesc,"0",1); //Fill the file with just over 400 bytes (100 ints)
    }

    //Map memory to the file
    void * addr = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fileDesc, 0);

    if (addr == MAP_FAILED)
    {
        cout << "ERROR: could not map the file." << endl;
        close(fileDesc);
        _exit(1);
    }

    records = (int *)(addr);

    bool finished = false;

    while (!finished)
    {
        finished = doMenu();
    }

    munmap(addr, MEMSIZE);
    close(fileDesc);
    _exit(0);
}

