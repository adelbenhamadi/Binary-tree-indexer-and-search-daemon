
#include "utils.h"
#include "mynodes.h"
#include "cstring"

using namespace mynodes;


int main(int argc, char *argv[])
{

    bool testing = 0;

    size_t limit = 0;

    myNodes mynodes;

    mynodes.load_data("data");
    mynodes.print_documents(testing || (limit>0 && limit<=10));
    mynodes.print_nodes(testing);
    mynodes.print_leaves(testing);

    //searching
    myprintf("\n\nbegin searching\n");

    const string_t sStar = "*";
    char word[32];
    size_t climit = 0;
    string_t f,s;
    time_point_t t_s;
    while(1){
        myprintf("\nPlease enter a word to search for (press ! to quit): ");
        scan_stdin("%s %d",2,word,&climit);

        myprintf("\n-----------------\n");
        if(climit==0) { climit = 1000000;}
        strToLower(word);

        f =word;
        std::size_t pos = f.find(sStar);

        t_s = std::chrono::high_resolution_clock::now();
        if(pos == std::string::npos){
            mynodes.doSearch(f,s,3);
        }
        else if(pos == 0){
            f = f.substr(pos+1,std::string::npos);
            mynodes.doSearch(f,s,0);
        }else if(pos >0 && pos <f.size()-1){
            s = f.substr(pos+1,std::string::npos);
            f = f.substr(0,pos);
            mynodes.doSearch(f,s,2);
        }else if(pos ==f.size()-1){
            f = f.substr(0,f.length()-1);
            mynodes.doSearch(f,s,1);
        }

        size_t i=1;
        for(LeafItemsVector_t::iterator it=mynodes.m_tResultItems.begin();i<=climit && it!=mynodes.m_tResultItems.end();++it)
             {
                myprintf("\n%d %llu \t%s",i++,*it,mynodes.get_document(*it).c_str());
             }
        myprintf("\n-----------------\n");
        myprintf("\n %llu docs found for %s\n",mynodes.m_iResultCount,word);

        print_time("searching done in",t_s);

    }
}

