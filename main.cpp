#include <iostream>
#include <string>
#include "fileUtils.h"

int main(int argc, char *argv[])
{
  FileUtils tools;
  std::string root(argv[1]);
  
  if(argc != 2)
  {
    std::cerr << "Usage: file_utils <root_directory>\n";
    return 1;
  }
  
  // Find all duplicate files start at root directory
  tools.FindDups(root);
  
}