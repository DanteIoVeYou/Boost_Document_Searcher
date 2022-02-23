#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

using std::cout;
using std::endl;
using std::string;
using std::vector;   
using std::unordered_map;

namespace common {
class Util {
  public:
    // Read file content from input_path to output
    static bool Read(const string& input_path, string* output) {
        std::ifstream file(input_path.c_str());
        if(!file.is_open()) {
          std::cout << "Open file " << input_path << " failing" <<std::endl;
          return false;
        }
        // Read the whole file by lines and add every line to output
        string line;
        while(std::getline(file, line)) {
          *output += (line + '\n');
        }
        file.close();
        return true;
    }



    // Split line based on boost lib
    static void Split(const string& input, const string& delimiter, vector<string>* output) {

      boost::split(*output, input, boost::is_any_of(delimiter), boost::token_compress_off);

    }
};
} //namespace end
