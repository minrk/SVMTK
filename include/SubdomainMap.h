// Copyright (C) 2018-2019 Lars Magnus Valnes and 
//
// This file is part of Surface Volume Meshing Toolkit (SVM-TK).
//
// SVM-Tk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SVM-Tk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SVM-Tk.  If not, see <http://www.gnu.org/licenses/>.
#ifndef SubdomainMap_H


#define SubdomainMap_H

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

class AbstractMap
{
   public:
        typedef int return_type;
        typedef boost::dynamic_bitset<> Bmask;
        
        virtual return_type index(const Bmask bits) = 0;
        //virtual return_type patch_index(const double s1,const double s2)=0;
        virtual const std::map<std::pair<int,int>,int> get_interfaces(const int number_of_surfaces)=0;
        virtual ~AbstractMap() {}

};


class DefaultMap : virtual public AbstractMap
{
   public:
        typedef int return_type;
        typedef boost::dynamic_bitset<> Bmask;
    
        DefaultMap() {}
        ~DefaultMap() {} 
         
        return_type index(const Bmask bits) 
        {
           return static_cast<return_type>(bits.to_ulong());
        }
              
      
        const std::map<std::pair<int,int>,int> get_interfaces(const int number_of_surfaces)
        { 
             
           int ulim =  pow (2,number_of_surfaces )+1; 
           std::map<std::pair<int,int> ,int> patches;
           int iter=1;
           for(int i =1; i< ulim; i++ )
           {
              for(int j=0; j< i ; j++)
              {
                 patches[std::pair<int,int>(i,j)]=iter++;
              }
           } 
           return patches;
        }


};

class SubdomainMap :virtual public AbstractMap
{

   public:
        typedef int return_type;
        typedef boost::dynamic_bitset<> Bmask;
       
        SubdomainMap() {}
        ~SubdomainMap() {} 

        void add(std::string string, int subdomain)
        {
           std::reverse( string.begin(), string.end());
           subdmap[Bmask(string)]=subdomain;
        } 
        return_type index(const Bmask bits) 
        {
           return static_cast<return_type>(subdmap[bits]);  
        }
        void print() 
        {
           for(std::map<boost::dynamic_bitset<>,int>::iterator it=subdmap.begin();it!=subdmap.end();++it )
           {
              std::cout<< "Subdomain: " << it->first << " " << it->second << " " << std::endl;
           }
           for(std::map<std::pair<int,int>,int>::iterator it=patches.begin();it!=patches.end();++it )
           {
              std::cout<< "Patches: " << it->first.first << " " << it->first.second << " " << it->second << " " << std::endl;
           }

        }
        std::vector<int> get_tags() 
        {
              std::vector<int> tags;
              tags.push_back(0);
              for (auto it : subdmap) 
              {
                 tags.push_back(it.second);
              } 
              return tags;
        }
        void add_interface(std::pair<int,int> interface, int tag)
        {
              if (interface.second > interface.first) 
              {std::swap(interface.first, interface.second);}
              patches[interface] = tag;
        } 
        const std::map<std::pair<int,int>, int> get_interfaces(const int number_of_surfaces)
        {
           if (!patches.empty()) 
           {
              return patches;
           }
           else
           {
              int iter=1;
              std::vector<int> tags = get_tags();
              for( auto i : tags )
              {
                 for(auto j : tags)
                 {
                    if( j>i )
                    {
                      patches[std::pair<int,int>(j,i)]=iter++;
                    }
                 } 
              }
              return patches;
           } 
        }

   private:
        std::map<boost::dynamic_bitset<>,int> subdmap;
   protected:
        std::map<std::pair<int,int> ,int> patches;
};



#endif
