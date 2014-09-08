/*!
  @file fileUtils.cpp
  @author Charles Irick
*/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <iterator>
#include "boost/filesystem.hpp"
#include "FileUtils.h"

/*! BUFFER_SIZE
This param is used to tweak the size of the buffer
used when comparing files. At first the buffer size starts
small since two different files are likely to have differences
early in the file. Then as we continue to match we ramp up
the comparison size. Tweaked for small (<1KB) and large (>1GB) */
const unsigned int FileUtils::BUFFER_SIZE[MAX_PASS] = 
  {64,255,4096,65535,16777215,268435456};


/*! FindDups
This function is used to find duplicate files under the 
root directory provided 

@param const std::string & root
The root directory to recursively search for dups under
*/
void FileUtils::FindDups( const std::string& dir_path )
{
  /* Build a hashmap where we hash all files that are the 
  same file size. We also build a set that contains all 
  keys where there were more than one file. This set can be used
  to access the hashmap to get all files that had matching sizes. 
  
  Comparing only files with matching sizes reduces the number of
  comparisons done */
  BuildFileMap(dir_path);
  
  // Compare only files where keys (sizes) match 
  CompareMatchingKeys();
  
  // Print stats about number of files scanned and total size of data
  PrintMapStats();
}

/*! CompareFiles
This function is used to compare two files to each other. The method
is to open the file as a binary file and read in raw bytes for 
comparison. We start with small buffer sizes and rapidly increase the
buffer size used if the file continues to match.

@param const std::string & file1
@param const std::string & file2
@return boolean
If files are the same or not
*/
bool FileUtils::CompareFiles(const std::string& file1, const std::string& file2)
{
  /* raw C file pointers are much faster at reading in 
  files than ifstream objects. */
  FILE* if1 = fopen(file1.c_str(),"rb");
  FILE* if2 = fopen(file2.c_str(),"rb");
  char *block1;
  char *block2;
  unsigned int size = 0;
  unsigned int pass = 0;
  
  if(if1 == NULL)
  {
    std::cout << "Could not open: " << file1 << std::endl;
    return false;
  }
  if(if2 == NULL)
  {
    std::cout << "Could not open: " << file2 << std::endl;
    return false;
  }
  
  /* Continue to compare files with increasing buffer sizes
  until we reach the end of the file, or there is a difference
  in the comparisons */
  do {
    if(pass < MAX_PASS)
    {
      size = BUFFER_SIZE[pass++];
    }
    
    block1 = new char[size]();
    block2 = new char[size]();
    
    fread(block1, 1, size,if1);
    fread(block2, 1, size,if2);

    /* compare binary blocks of data */
    if (memcmp(block1, block2, size) != 0)
    {
      delete[] block1;
      delete[] block2;
      fclose(if1);
      fclose(if2);
      return false;
    }
    
    delete[] block1;
    delete[] block2;
  } while ((feof(if1) == 0) && (feof(if2) == 0));
  
  fclose(if1);
  fclose(if2);
  
  return true;
}

/*! CompareMatchingKeys
This function iterates through the Hash Map of files hashed
based on size. It will attempt to compare files only if they
have the same size. Each time two files are compared, if they
are found to be the same size they are pushed into a set so
that we do not compare two same files more than once as we
permute through all possible matches 
*/
void FileUtils::CompareMatchingKeys()
{
  std::vector<std::string>::iterator i,j;
  std::set<std::string> existing_matches;
  bool first_match, match_found;
  
  std::cout << "Matching Files: \n";
  
  /* Iterate of Hash Map */
  for(auto& x : matching_keys)
  {
    /* Get the current vector of files with the same size */
    std::vector<std::string> &curr = file_map[x];  
    
    /* Compare all files with the same size against the others */
    for(i=curr.begin();i!=curr.end();i++)
    {
      first_match = true;
      match_found = false;
      for(j=i+1;j!=curr.end();j++)
      {
        /* Check in the set if we are comparing against something
        we already have matched with */
        if(existing_matches.find(*i) != existing_matches.end())
        {
          continue;
        }
        
        /* Compare the two files here. Heart of work being done. */
        if(CompareFiles(*i,*j))
        {
          /* If this is the first match in this set, print header
          info for match as well as match */
          if(first_match)
          {
            std::cout << "[ " << *i << "," << std::endl
                 << "  " << *j;
            match_found = true;
            first_match = false;
          }
          /* Else just print current match */
          else
          {
            std::cout << ", " << std::endl
              << "  "  << *j;
          }
          /* Insert current match to set */
          existing_matches.insert(*j);
        }
      }
      /* If the current iteration matched with anything push
      it into the set */
      if(match_found)
      {
        std::cout << " ]" << std::endl << std::endl; 
        existing_matches.insert(*i); 
      } /* if(match_found) */
    } /* for(i=curr.begin();i!=curr.end();i++) */
  } /* for(auto& x : matching_keys) */
}

/*! BuildFileMap
This function iterates through the Hash Map of files hashed
based on size. It will attempt to compare files only if they
have the same size. Each time two files are compared, if they
are found to be the same size they are pushed into a set so
that we do not compare two same files more than once as we
permute through all possible matches

@param const boost::filesystem::path & dir_path
Root directory to build the Hash Map from.
@return boolean
If has map building succeeds or not. 
*/
bool FileUtils::BuildFileMap(const std::string& dir_path)
{
  const boost::filesystem::path root(dir_path.c_str());
  
  // In case user inputs bad path and didn't check first themselves
  if ( !exists( root ) ) 
  {
    std::cerr << "Root directory" << root << "does not exist\n";
    return false;
  }
  
  boost::filesystem::directory_iterator end_itr; 
  
  for ( boost::filesystem::directory_iterator itr( root );
        itr != end_itr;
        ++itr )
  {
    /* If current item is a directory iterate into directory to get files */
    if ( is_directory(itr->status()) )
    {
      BuildFileMap( itr->path().string() );
    }
    /* If this is a file, read size and push to map */
    else if ( is_regular_file(itr->status()) ) 
    {
      int size = file_size(itr->path());

      /* Push absolute path into the map based on filesize */
      file_map[size].push_back(itr->path().string());
      
      /* Check if we have already seen a file of this size
         Push all Keys with multiple entries into vector */
      if(file_map[size].size() > 1)
      {
        matching_keys.insert(size);
      }
    }
  }
  /* Flag that we have built the full map of all files */
  map_built = true;
  
  return true;
}

/*! PrintMap
This function is a debug function that can be used
to print the contents of the Hash Map for debugging.
*/
void FileUtils::PrintMap()
{
  for(auto& x : file_map)
  {
    if(x.second.size() > 1){
      std::cout << "Potential dup!" << std::endl;
    }
    std::cout << "Key " << x.first << ": " << std::endl;
    for(auto& y : x.second)
      {
        std::cout << y << std::endl;
      } 
  }
  
  for(auto& x : matching_keys)
  {
    std::cout << x << std::endl;
  }
}

/*! PrintMapStats
This function can be used to print useful stats
about the hashmap.
*/
void FileUtils::PrintMapStats()
{
  int num_files = 0;
  double total_size = 0;
  
  for(auto& x : file_map)
  {
    num_files += x.second.size();
    total_size += ((double)x.first * (double)x.second.size())/(double)(1<<20);
  }
  
  std::cout << std::fixed << std::showpoint << std::setprecision(2) 
       << "-- Stats -- \n"
       << "Number of files scanned: " << num_files << std::endl
       << "Total data compared:     " << total_size << "MB" << std::endl;
}
