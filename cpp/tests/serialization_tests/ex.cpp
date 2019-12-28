//Listing 1

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <cstdlib>



const char* fname = "records.txt";
int main()
{
    std::ofstream ofile(fname);
    std::ostream_iterator<long> iter(ofile, " ");
    // ------ vector of pseudorandom numbers
    std::vector<long> rno(100);
    for (int i = 0; i < 100; i++)
        rno[i] = rand();
    // ------- write the data
    std::copy(rno.begin(), rno.end(), iter);
    ofile.close();
    return 0;
}


//Listing 2
 // ------ tstios01.cpp#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>



const char* fname = "records.txt";
main()
{
    std::vector<long> rno(100);
    std::ifstream infile(fname);
    // ----- read vector of pseudorandom numbers
    std::istream_iterator<long> in(infile);
    std::istream_iterator<long> end;
    std::copy(in, end, rno.begin());
    // ------- display the input numbers
    for (int i = 0; i < 100; i++)
        std::cout << rno[i] << ' ';
    return 0;
}


//Listing 3
#include <vector>
#include <typeinfo>
// --- base persistent allocator class
template <class T>
class PAllocator : public std::allocator<T> {
protected:
    PAllocator(const char* filename);
    // ...
};
// --- specialized persistent allocator class
class IntAllocator : public PAllocator<int>  {
public:
    IntAllocator() : PAllocator<int>(typeid(*this).name())
        {  }
};
int main()
{
    std::vector<int, IntAllocator> pints;
    // ...
    return 0;
}

