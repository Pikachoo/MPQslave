#ifndef MAIN_HPP_
#define MAIN_HPP_

#include "src/blp.hpp"
#include "src/IO.hpp"
#include "src/regex.hpp"
#include "src/unpacker.hpp"

#include "src/w3i.hpp"

void parse(const std::string &path, const std::string &extension);
void parse_blp(const std::string &path, const std::string &extension);

#endif /* MAIN_HPP_ */
