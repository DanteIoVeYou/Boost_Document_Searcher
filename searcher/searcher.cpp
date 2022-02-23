#include "searcher.h"
#include <algorithm>
#define PROCESS_BAR_LENGTH 102
namespace searcher{

  const char* const DICT_PATH = "../../cppjieba/dict/jieba.dict.utf8";
  const char* const HMM_PATH = "../../cppjieba//dict/hmm_model.utf8";
  const char* const USER_DICT_PATH = "../../cppjieba/dict/user.dict.utf8";
  const char* const IDF_PATH = "../../cppjieba/dict/idf.utf8";
  const char* const STOP_WORD_PATH = "../../cppjieba/dict/stop_words.utf8";
  Index::Index()
    : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH){}

  void ProcessBar(int64_t line_amount, int64_t doc_id) {
    char pb[PROCESS_BAR_LENGTH] = {0};
    char state[4] = {'-', '\\', '|', '/'};
    pb[0] = '[';
    pb[PROCESS_BAR_LENGTH - 1] = ']';
    int64_t mark_amount = (doc_id + 2) * 100 / line_amount;
    for(int i = 1; i <= mark_amount; i++) {
      pb[i] = '#';
    }
    for(int i = 0; i < PROCESS_BAR_LENGTH; i++) {
      if(pb[i] != 0) {
        cout << pb[i];
      }
      else {
        cout << " ";
      }
    }
    cout << " [%" << mark_amount << "] [" << state[mark_amount % 4] << "]";
    if(mark_amount == 100) {
      sleep(1);
    }
    std::fflush(stdout);
    cout << '\r';
  }


  
  const DocInfo* Index::GetDocInfo(int64_t doc_id) {

    if(doc_id < 0 || doc_id > forward_index.size()) {
      return nullptr;
    }
    return &forward_index[doc_id];
  }

  const InvertedList* Index::GetInvertedList(const string& key) {
    unordered_map<string, InvertedList>::iterator it = inverted_index.find(key);
    if(it == inverted_index.end()) {
      return nullptr;
    }
    return &it->second;
  }

  bool Index::Build(const string& input_path) {

    // 1. read raw_input file, which is a line txt that means each line stands for a html file 
    std::cout << "Start Build Index..." << std::endl; 
    std::ifstream file(input_path.c_str());
    if(!file.is_open()) {
      std::cout << "Read raw_input Failing..." << std::endl;
      return false;
    }

    // get number of lines in line 
    std::ifstream tmpfile(input_path.c_str());
    string tmpline;
    int64_t line_amount = 0;
    while(std::getline(tmpfile, tmpline)) {
      line_amount++;
    }
    string line;
    while(std::getline(file, line)) {

      // 2. convert each line to  DocInfo , construct ForwardIndex
      DocInfo* doc_info = BuildForward(line);
      if(!doc_info) {
        std::cout << "Construct Failing..." << std::endl;
        continue;
      }
      // 3. using ForwardIndex to Build Inverted Index
      BuildInverted(*doc_info);
      // print process bar
      ProcessBar(line_amount, (*doc_info).doc_id);
      
    }
    

    std::cout << "Finish Build Index..." << std::endl;
    file.close();
    return true;
  }


  // use '\3' to cut title, url, content and convert to DocInfo
  DocInfo* Index::BuildForward(const string& line) {
    vector<string> tokens;
    // 1.Split raw_input with "\3"
    common::Util::Split(line, "\3", &tokens);
    if(tokens.size() != 3) {
      // Split failed if string is not consisted of 3 strings
      return nullptr;
    }
    // 2.put token into DocInfo 
    DocInfo doc_info;
    doc_info.doc_id = forward_index.size();
    doc_info.title = tokens[0];
    doc_info.url = tokens[1];
    doc_info.content = tokens[2];
    forward_index.push_back(std::move(doc_info));
    return &forward_index.back();
  }

  void Index::BuildInverted(const DocInfo& doc_info) {
    struct WordCut{
      int title_cnt;
      int content_cnt;

      WordCut()
        :title_cnt(0)
        ,content_cnt(0){}
    };
    unordered_map<string, WordCut> word_cut_map;
    //1.title segment
    vector<string> title_token;
    CutWord(doc_info.title, &title_token);
    //2.traversal, count frequency of title words
    for(string word: title_token) {
      boost::to_lower(word); // convert all character to lower
      word_cut_map[word].title_cnt++;
    }
    //3.content segment
    vector<string> content_token;
    CutWord(doc_info.content, &content_token);
    //4.traversal, count frequency of content words
    for(string word: content_token) {
      boost::to_lower(word);
      word_cut_map[word].content_cnt++;
    }

    //5.deal with weight
    for(const auto& word_pair: word_cut_map) {
      Weight weight;
      weight.doc_id = doc_info.doc_id;
      // weight = title_cnt * 10 + content_cnt * 1
      weight.weight = 10 * word_pair.second.title_cnt + word_pair.second.content_cnt;
      weight.word = word_pair.first;
      //insert word into inverted_index
      InvertedList& invert_list = inverted_index[word_pair.first];
      invert_list.push_back(weight);
    }

  }
  
  void Index::CutWord(const string& input, vector<string>* output) {
    jieba.CutForSearch(input, *output);
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////
  bool Searcher::Init(const string& input_path) {
    return index->Build(input_path);
  }
  bool Searcher::Search(const string& query, string* output) {
    // 1.word segment
    vector<string> tokens;
    index->CutWord(query, &tokens);
    // 2.trigger
    vector<Weight> all_token_result;
    for(string word : tokens) {
      boost::to_lower(word);
      auto* inverted_list = index->GetInvertedList(word);
      if(!inverted_list) {
        // the word is not found
        continue;
      }
      all_token_result.insert(all_token_result.end(), inverted_list->begin(), inverted_list->end());
    }
    // 3.descending sort 
    std::sort(all_token_result.begin(), all_token_result.end(), [](const Weight& w1, const Weight& w2){return w1.weight > w2.weight;});
    // 4.pack
    Json::Value results;
    for(const auto& weight: all_token_result) {
      const DocInfo* doc_info = index->GetDocInfo(weight.doc_id);
      Json::Value result;
      result["title"] = doc_info->title;
      result["url"] = doc_info->url;
      result["description"] = GenerateDescription(doc_info->content, weight.word); 
      results.append(result);
    }


    Json::FastWriter writer;
    *output = writer.write(results);
    return true;
  }
  string Searcher::GenerateDescription(const string& content, const string& word) {
    // word as center ,find 60(or other) bytes before and 160(or other) bytes after
    // if less than 60 bytes before, start from 0
    // less than 160 bytes after, to the end
    
    size_t first_pos = content.find(word);
    size_t begin = 0;
    if(begin == string::npos) {
      // word not found
      if(content.size() < 160) {
        return content;
      }
      string description =  content.substr(0, 160);
      description[description.size() - 1] = '.';
      description[description.size() - 2] = '.';
      description[description.size() - 3] = '.';
      return content.substr(0, 160);
    }

    begin = first_pos < 60 ? 0 : first_pos - 60;
    if(begin + 160 > content.size()) {
      return content.substr(begin);
    }
    else {
      string description =  content.substr(begin, 160);
      description[description.size() - 1] = '.';
      description[description.size() - 2] = '.';
      description[description.size() - 3] = '.';
      return description;
    }


  }
} // namespace end
