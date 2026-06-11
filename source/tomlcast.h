// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <type_traits>

namespace toml {

	inline bool isIntegerString(std::string_view str)
	{
		if (str.empty()) {
			return false;
		}

		size_t index = 0;
		if (str[index] == '-' || str[index] == '+') {
			++index;
			if (index >= str.size()) {
				return false;
			}
		}

		for (; index < str.size(); ++index) {
			if (!std::isdigit(static_cast<unsigned char>(str[index]))) {
				return false;
			}
		}

		return true;
	}

	template<typename T>
	T cast(std::string_view str)
	{
		if constexpr (std::is_same_v<T, std::string>) {
			return std::string(str);
		} else if constexpr (std::is_integral_v<T>) {
			if (!isIntegerString(str)) {
				return T{0};
			}
			return static_cast<T>(std::stoll(std::string(str)));
		} else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
			if (str.empty()) {
				return T{0};
			}
			try {
				return static_cast<T>(std::stod(std::string(str)));
			} catch (const std::exception&) {
				return T{0};
			}
		} else {
			static_assert(std::is_integral_v<T> || std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, std::string>,
				"Unsupported type");
		}
	}

}
