// The model is to pretreat
// - read and analyse htmls from boost
// - get title. url and contents wuthout html labels
// - output is texts


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "../common/util.hpp"


using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::unordered_map;

// input file path of boost html
string g_input_path = "../data/input";

// output file path 
string g_output_path = "../data/tmp/raw_input";

// structure of a html file
struct DocInfo {
  string title;
  string url;
  string content;
};

//   core steps
// 1.get all the urls of html files
// 2.read contents of all html files following urls and analyse them
// 3.write contents into output files
 

bool EnumFile(const string& g_input_path, vector<string>* file_list) {
  namespace fs = boost::filesystem;
  fs::path root_path(g_input_path);
  if(!fs::exists(root_path)) {
    std::cout << "Input path is invalid" << std::endl;
    return false;
  }
  // recursive traversal  
  fs::recursive_directory_iterator end_iter;
  for(fs::recursive_directory_iterator iter(root_path); iter != end_iter; iter++) {
    // judge if path is a directory, if so, skip 
    if(!fs::is_regular_file(*iter)) {
      continue;
    }
    // judge if path is  a html file , if not, skip
    if(iter->path().extension() != ".html") {
      continue;
    }
    // add html file name to file_list 
    file_list->push_back(iter->path().string());
  }

  return true;
}


// find title label 
bool ParseTitle(const string& html, string* title) {

  size_t begin = html.find("<title>");
  if(begin == string::npos) {
    std::cout <<"title is not found" << std::endl;
    return false;
  }
  
  
  size_t end = html.find("</title>");
  if(end == string::npos) {
    std::cout <<"/title is not found" << std::endl;
    return false;
  }
 
  begin += string("<title>").size();
  if(begin >= end) {
    std::cout << "Title is invalid" << std::endl;
    return false;
  }
  
  *title = html.substr(begin, end - begin);

  return true;
}


// local path: ../data/input/html/xxx.html 
// online path: https://www.boost.org/doc/libs/1_53_0/doc/html/xxx.html
bool ParseUrl(const string& file_path, string* url) {
  string url_head = "https://www.boost.org/doc/libs/1_53_0/doc";
  string url_tail = file_path.substr(g_input_path.size());
  *url = url_head + url_tail;
  return true;
}


// remove html label in html string 
bool ParseContent(const string& html, string* content) {
  bool is_content = true;
  for(auto e: html) {
    if(is_content == true) {
      if(e == '<') {
        is_content = false;
      }
      else {
        //deal with '\n'
        if(e == '\n') {
          e = ' ';
        }
        // common char
        content->push_back(e);
      }
    }
    else {
      if(e == '>') {
        is_content = true;
      }
    }
  }
  return true;
}

bool ParseFile(const string& file_path, DocInfo* doc_info) {

  // 1.read file content 
  string html;
  bool ret = common::Util::Read(file_path, &html);
  
  if(!ret) {
    std::cout << file_path << " :read file content failing..." << std::endl;
    return false;
  }
  // 2.extract title according to <title></title>
  ret = ParseTitle(html, &doc_info->title);
  if(!ret) {
    std::cout << file_path << " :title extracted failing..," << std::endl;
    return false;
  }
  // 3.construct online document url on the basis of file path 
  ret = ParseUrl(file_path, &doc_info->url);
  if(!ret) {
    std::cout << file_path << " :extract url failing..." << std::endl;
    return false;
  }
  // 4.remove html label
  ret = ParseContent(html, &doc_info->content);
  if(!ret) {
    std::cout << file_path << " :remove html label failing..." << std::endl;
    return false;
  }
  return true;
}


void WriteOutput(const DocInfo& doc_info, std::ofstream& ofstream) {
 ofstream << doc_info.title << '\3' << doc_info.url << '\3' << doc_info.content << std::endl;  
}

int main() {
  
  // enumerate urls
  vector<string> file_list;
  bool ret = EnumFile(g_input_path, &file_list);
  
  if(!ret) {
    std::cout << "Enumerate failing..." << std::endl;
    return 1;
  }
  

  // traverse the html directory and deal with every html file
  int ans = 0;
  std::ofstream output_file(g_output_path.c_str());
  if(!output_file.is_open()) {
    std::cout << "open output_file failing..." << std::endl; 
  }
  for(auto file_path = file_list.begin();  file_path != file_list.end(); file_path++) {
    std::cout << *file_path << std::endl;
    ans++;

    // create DocInfo structure to store every html file infomation
    DocInfo doc_info;
    // 
    ret = ParseFile(*file_path, &doc_info);
    if(!ret) {
      std::cout << "Parse file failing:" << *file_path << std::endl;
      continue;
    }

    // wirte dec_info into file
    WriteOutput(doc_info, output_file);
  }
  std::cout << "There are " << ans << " html files" << std::endl;
}

