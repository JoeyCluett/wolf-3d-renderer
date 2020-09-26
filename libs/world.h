#pragma once

#include <SDL/SDL.h>

#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>
#include <array>

#include "block.h"

struct world {

    std::map<std::pair<int, int>, block_t> env_table;
    typedef std::map<std::pair<int, int>, block_t>::iterator iterator;

    world(void) 
    {
    }
    
    auto find(std::pair<int, int> req) -> world::iterator {
        return this->env_table.find(req);
    }
    
    auto end(void) -> world::iterator {
        return this->env_table.end();
    }

    auto operator[](std::pair<int, int> idx) -> block_t&
    {
        return this->env_table[idx];
    }

};
