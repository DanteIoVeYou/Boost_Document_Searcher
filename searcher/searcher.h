#include <iostream>                                           
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <vector>                                  
#include <unordered_map>      
#include <algorithm>
#include <boost/filesystem/path.hpp>                        
#include <boost/filesystem/operations.hpp>
#include <jsoncpp/json/json.h>
#include "../../cppjieba/include/cppjieba/Jieba.hpp"                                                                       
#include "../common/util.hpp"
                                                     
using std::cout;
using std::cin;
using std::endl;                                                   
using std::string;           
using std::vector;
using std::unordered_map; 


namespace searcher{

////////////////////////////////////////////////////////////////////////////////////////// 
// Index model
////////////////////////////////////////////////////////////////////////////////////////// 
// Index struct
  struct DocInfo {
    int64_t doc_id;
    string title;
    string url;
    string content;
  };

  struct Weight {
   int64_t doc_id;
   int weight;
   string word;
  };

  typedef vector<Weight> InvertedList;
  class Index {

    private:
      vector<DocInfo> forward_index;
      unordered_map<string, InvertedList> inverted_index;
    public:
      Index();
      // 1. Forward searcher func, return pointer to use NULL to indicate invalid situation
      const DocInfo* GetDocInfo(int64_t doc_id);
      // 2. Inverted searcher func
      const InvertedList* GetInvertedList(const string& key);
      // 3. Build the Index 
      bool Build(const string& input_path);

      //4.WordCut Function
      void CutWord(const string& input, vector<string>* output);
    private:
      DocInfo* BuildForward(const string& line);
      void BuildInverted(const DocInfo& doc_info);
      cppjieba::Jieba jieba;
  };







////////////////////////////////////////////////////////////////////////////////////////// 
// Searcher model
////////////////////////////////////////////////////////////////////////////////////////// 
  class Searcher {
    private:
      Index* index;

    public:
      Searcher() : index(new Index()){}
      bool Init(const string& input_path);
      bool Search(const string& query, string* output);
    private:
      string GenerateDescription(const string& content, const string& word);
  };
} // namesapce end
