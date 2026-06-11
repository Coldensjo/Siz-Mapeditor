// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "main.h"

#include "tomlnode.h"
#include "tomlutil.h"

#include <sstream>

namespace {
	std::string nodeToString(const toml::node* node)
	{
		if (!node) {
			return {};
		}
		if (auto* value = node->as_string()) {
			return std::string(value->get());
		}
		if (auto* value = node->as_integer()) {
			return std::to_string(value->get());
		}
		if (auto* value = node->as_floating_point()) {
			std::ostringstream ss;
			ss << value->get();
			return ss.str();
		}
		if (auto* value = node->as_boolean()) {
			return value->get() ? "1" : "0";
		}
		return {};
	}
}

TomlAttribute::TomlAttribute(std::string key, const toml::node* node) :
	key(std::move(key)), node(node)
{
}

const char* TomlAttribute::name() const
{
	return key.c_str();
}

const char* TomlAttribute::value() const
{
	static thread_local std::string buffer;
	buffer = nodeToString(node);
	return buffer.c_str();
}

std::string TomlAttribute::as_string() const
{
	return nodeToString(node);
}

bool TomlAttribute::as_bool(bool defaultValue) const
{
	if (!node) {
		return defaultValue;
	}
	if (auto* value = node->as_boolean()) {
		return value->get();
	}
	if (auto* value = node->as_integer()) {
		return value->get() != 0;
	}
	if (auto* value = node->as_string()) {
		const std::string str = value->get();
		return str == "1" || str == "true" || str == "yes";
	}
	return false;
}

int TomlAttribute::as_int() const
{
	if (!node) {
		return 0;
	}
	if (auto* value = node->as_integer()) {
		return static_cast<int>(value->get());
	}
	if (auto* value = node->as_floating_point()) {
		return static_cast<int>(value->get());
	}
	if (auto* value = node->as_string()) {
		const std::string str = value->get();
		if (str.empty()) {
			return 0;
		}
		try {
			return std::stoi(str);
		} catch (const std::exception&) {
			return 0;
		}
	}
	return 0;
}

int64_t TomlAttribute::as_int64(int64_t defaultValue) const
{
	if (!node) {
		return defaultValue;
	}
	if (auto* value = node->as_integer()) {
		return value->get();
	}
	if (auto* value = node->as_floating_point()) {
		return static_cast<int64_t>(value->get());
	}
	if (auto* value = node->as_boolean()) {
		return value->get() ? 1 : 0;
	}
	if (auto* value = node->as_string()) {
		const std::string str = value->get();
		if (str.empty()) {
			return defaultValue;
		}
		try {
			return std::stoll(str);
		} catch (const std::exception&) {
			return defaultValue;
		}
	}
	return defaultValue;
}

uint64_t TomlAttribute::as_uint64(uint64_t defaultValue) const
{
	if (!node) {
		return defaultValue;
	}
	if (auto* value = node->as_integer()) {
		return static_cast<uint64_t>(std::max<int64_t>(0, value->get()));
	}
	if (auto* value = node->as_floating_point()) {
		return static_cast<uint64_t>(std::max<double>(0, value->get()));
	}
	if (auto* value = node->as_boolean()) {
		return value->get() ? 1 : 0;
	}
	if (auto* value = node->as_string()) {
		const std::string str = value->get();
		if (str.empty()) {
			return defaultValue;
		}
		try {
			return std::stoull(str);
		} catch (const std::exception&) {
			return defaultValue;
		}
	}
	return defaultValue;
}

bool TomlNode::loadFile(const std::string& filename, toml::table& rootTable)
{
	try {
		rootTable = toml::parse_file(filename);
		return true;
	} catch (const toml::parse_error& error) {
		printTomlError("Error - TomlNode::loadFile", filename, error);
		return false;
	}
}

TomlNode TomlNode::fromFile(const std::string& filename, toml::table& storage)
{
	if (!loadFile(filename, storage)) {
		return {};
	}
	return TomlNode(&storage);
}

TomlNode TomlNode::fromBuffer(const std::string_view& buffer, toml::table& storage)
{
	try {
		storage = toml::parse(buffer);
		return TomlNode(&storage);
	} catch (const toml::parse_error&) {
		return {};
	}
}

TomlNode::TomlNode(const toml::table* table, std::string elementName) :
	table(table), elementName(std::move(elementName))
{
}

TomlNode::TomlNode(const toml::array* array) :
	array(array)
{
}

TomlNode TomlNode::child(const char* name) const
{
	if (array) {
		return {};
	}
	if (!table) {
		return {};
	}

	if (auto node = table->get(name)) {
		if (auto* childTable = node->as_table()) {
			return TomlNode(childTable, name);
		}
		if (auto* childArray = node->as_array()) {
			return TomlNode(childArray);
		}
	}
	return {};
}

TomlAttribute TomlNode::attribute(const char* name) const
{
	if (array || !table) {
		return {};
	}
	if (auto node = table->get(name)) {
		if (node->is_table() || node->is_array()) {
			return {};
		}
		return TomlAttribute(name, node);
	}
	return {};
}

TomlAttribute TomlNode::firstAttribute() const
{
	if (array || !table) {
		return {};
	}
	for (auto&& [key, value] : *table) {
		if (key == "_tag") {
			continue;
		}
		if (value.is_table() || value.is_array()) {
			continue;
		}
		return TomlAttribute(std::string(key), &value);
	}
	return {};
}

const char* TomlNode::name() const
{
	if (!elementName.empty()) {
		return elementName.c_str();
	}
	if (table) {
		if (auto tag = table->get("_tag")) {
			if (auto* value = tag->as_string()) {
				static thread_local std::string tagName;
				tagName = value->get();
				return tagName.c_str();
			}
		}
	}
	return "";
}

TomlNode::ChildrenIterator::ChildrenIterator(const TomlNode* parent, size_t index) :
	parent(parent), index(index)
{
}

TomlNode::ChildrenIterator& TomlNode::ChildrenIterator::operator++()
{
	++index;
	return *this;
}

bool TomlNode::ChildrenIterator::operator!=(const ChildrenIterator& other) const
{
	return index != other.index;
}

void TomlNode::forEachAttribute(const std::function<void(const std::string&, const TomlAttribute&)>& fn) const
{
	if (!table) {
		return;
	}
	for (auto&& [key, value] : *table) {
		if (key == "_tag" || key == "_text") {
			continue;
		}
		if (value.is_table() || value.is_array()) {
			continue;
		}
		const std::string keyName(key.str());
		fn(keyName, TomlAttribute(keyName, &value));
	}
}

TomlNode::ChildrenIterator TomlNode::beginChildren() const
{
	return ChildrenIterator(this, 0);
}

TomlNode::ChildrenIterator TomlNode::endChildren() const
{
	if (array) {
		return ChildrenIterator(this, array->size());
	}
	if (table && !elementName.empty()) {
		return ChildrenIterator(this, 1);
	}
	if (table) {
		for (auto&& [key, value] : *table) {
			if (key == "_tag" || key == "_text") {
				continue;
			}
			if (value.is_array()) {
				return ChildrenIterator(this, value.as_array()->size());
			}
			if (value.is_table()) {
				return ChildrenIterator(this, 1);
			}
		}
	}
	return ChildrenIterator(this, 0);
}

TomlNode TomlNode::ChildrenIterator::operator*() const
{
	if (!parent->array) {
		if (!parent->table) {
			return {};
		}
		if (!parent->elementName.empty()) {
			if (index == 0) {
				return TomlNode(parent->table, parent->elementName);
			}
			return {};
		}
		for (auto&& [key, value] : *parent->table) {
			if (key == "_tag" || key == "_text") {
				continue;
			}
			if (value.is_array()) {
				const toml::array& arr = *value.as_array();
				if (index >= arr.size()) {
					return {};
				}
				if (auto* childTable = arr.get(index)->as_table()) {
					return TomlNode(childTable, std::string(key.str()));
				}
				return {};
			}
			if (value.is_table() && index == 0) {
				return TomlNode(value.as_table(), std::string(key.str()));
			}
		}
		return {};
	}
	const toml::array& arr = *parent->array;
	if (index >= arr.size()) {
		return {};
	}
	if (auto* childTable = arr.get(index)->as_table()) {
		std::string tag;
		if (auto tagNode = childTable->get("_tag")) {
			if (auto* tagValue = tagNode->as_string()) {
				tag = tagValue->get();
			}
		}
		return TomlNode(childTable, tag);
	}
	return {};
}
