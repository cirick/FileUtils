#ifndef FILE_UTILS_H
#define FILE_UTILS_H
/*!
  @file fileUtils.h
  @author Charles Irick
*/

/* Includes */
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

/*!
  This class is used to provide utitilies for searching, manipulating, 
  and getting statistics about files. The initial revision only supports
  the ability to find duplicate files from a root directory. 

  @brief This class can be used to find duplicate files under some
  given directory. 
 */

class FileUtils
{
public:
  /*! Constructor */
  FileUtils()
    :map_built(false) {}
  void FindDups( const std::string& dir_path );
  
protected:
  bool BuildFileMap(const std::string& dir_path);
  void PrintMap();
  void PrintMapStats();
  void CompareMatchingKeys();
  bool CompareFiles(const std::string& file1, const std::string& file2);
  
private:
  static const unsigned char MAX_PASS = 6;
  static const unsigned int BUFFER_SIZE[MAX_PASS];
  
  std::unordered_map<int,std::vector<std::string> > file_map;
  /*! Hash Map used to has files disovered based on filesize */
  bool map_built;
  /*! Informs if the Hash Map has been build or not for this instance */
  std::set<int> matching_keys;
  /*! Set of Keys that have more than one entry in the Hash Map */
};

#endif /* FILE_UTILS_H */