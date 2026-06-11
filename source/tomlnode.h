// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <functional>
#include <toml++/toml.hpp>

class TomlAttribute
{
	public:
		TomlAttribute() = default;
		TomlAttribute(std::string key, const toml::node* node);

		explicit operator bool() const
		{
			return node != nullptr;
		}

		const char* name() const;
		const char* value() const;
		std::string as_string() const;
		bool as_bool(bool defaultValue = false) const;
		int as_int() const;
		int64_t as_int64(int64_t defaultValue = 0) const;
		uint64_t as_uint64(uint64_t defaultValue = 0) const;

	private:
		std::string key;
		const toml::node* node = nullptr;
};

class TomlNode
{
	public:
		TomlNode() = default;

		static bool loadFile(const std::string& filename, toml::table& rootTable);
		static TomlNode fromFile(const std::string& filename, toml::table& storage);
		static TomlNode fromBuffer(const std::string_view& buffer, toml::table& storage);

		TomlNode child(const char* name) const;
		TomlAttribute attribute(const char* name) const;
		TomlAttribute firstAttribute() const;
		const char* name() const;

		class ChildrenIterator
		{
			public:
				ChildrenIterator(const TomlNode* parent, size_t index);
				TomlNode operator*() const;
				ChildrenIterator& operator++();
				bool operator!=(const ChildrenIterator& other) const;

			private:
				const TomlNode* parent;
				size_t index;
		};

		ChildrenIterator beginChildren() const;
		ChildrenIterator endChildren() const;

		void forEachAttribute(const std::function<void(const std::string&, const TomlAttribute&)>& fn) const;

		bool empty() const
		{
			return !table && !array;
		}

		explicit operator bool() const
		{
			return !empty();
		}

	private:
		TomlNode(const toml::table* table, std::string elementName = {});
		TomlNode(const toml::array* array);

		const toml::table* table = nullptr;
		const toml::array* array = nullptr;
		std::string elementName;
};

inline TomlNode::ChildrenIterator begin(const TomlNode& node)
{
	return node.beginChildren();
}

inline TomlNode::ChildrenIterator end(const TomlNode& node)
{
	return node.endChildren();
}
