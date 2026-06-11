// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "main.h"

#include "tomlutil.h"

#include <fstream>
#include <iostream>

void printTomlError(const std::string& where, const std::string& fileName, const toml::parse_error& error)
{
	std::cout << '[' << where << "] Failed to load " << fileName << ": " << error.description() << std::endl;

	std::ifstream file(fileName);
	if (!file.is_open()) {
		return;
	}

	std::string line;
	uint32_t currentLine = 1;
	while (std::getline(file, line)) {
		if (currentLine >= error.source().begin.line && currentLine <= error.source().end.line) {
			std::cout << "Line " << currentLine << ": " << line << std::endl;
			if (currentLine == error.source().begin.line) {
				std::cout << std::string(error.source().begin.column - 1, ' ') << '^' << std::endl;
			}
		}
		++currentLine;
	}
}
