// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <toml++/toml.hpp>

void printTomlError(const std::string& where, const std::string& fileName, const toml::parse_error& error);
