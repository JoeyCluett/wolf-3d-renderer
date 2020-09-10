#pragma once

#include <iostream>
#include <map>
#include <utility>

#include "block.h"

struct world {

    std::map<std::pair<int, int>, block_t> env_table;

    typedef std::map<std::pair<int, int>, block_t>::iterator iterator;

    auto find(std::pair<int, int> req) -> std::map<std::pair<int, int>, block_t>::iterator;
    
    auto end(void) -> std::map<std::pair<int, int>, block_t>::iterator;

    auto operator[](std::pair<int, int> idx) -> block_t&;

};

auto world::operator[](std::pair<int, int> idx) -> block_t& {
    return this->env_table[idx];
}

auto world::find(std::pair<int, int> req) -> std::map<std::pair<int, int>, block_t>::iterator {
    return this->env_table.find(req);
}

auto world::end(void) -> std::map<std::pair<int, int>, block_t>::iterator {
    return this->env_table.end();
}

