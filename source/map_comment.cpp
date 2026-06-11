//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "map_comment.h"

#include <ctime>

MapComment MapComments::createComment(const Position& pos, const std::string& author, const std::string& text) {
	MapComment comment;
	comment.id = nextId++;
	comment.pos = pos;
	comment.author = author;
	comment.text = text;
	comment.createdUnix = static_cast<int64_t>(time(nullptr));
	comments.push_back(comment);
	return comment;
}

void MapComments::upsert(const MapComment& comment) {
	for (MapComment& existing : comments) {
		if (existing.id == comment.id) {
			existing = comment;
			if (comment.id >= nextId) {
				nextId = comment.id + 1;
			}
			return;
		}
	}
	comments.push_back(comment);
	if (comment.id >= nextId) {
		nextId = comment.id + 1;
	}
}

bool MapComments::removeById(uint32_t id) {
	for (auto it = comments.begin(); it != comments.end(); ++it) {
		if (it->id == id) {
			comments.erase(it);
			return true;
		}
	}
	return false;
}

void MapComments::setAll(std::vector<MapComment> newComments, uint32_t nextCommentId) {
	comments = std::move(newComments);
	nextId = std::max<uint32_t>(nextCommentId, 1);
	for (const MapComment& comment : comments) {
		if (comment.id >= nextId) {
			nextId = comment.id + 1;
		}
	}
}

std::vector<const MapComment*> MapComments::atPosition(int x, int y, int z) const {
	std::vector<const MapComment*> result;
	for (const MapComment& comment : comments) {
		if (comment.pos.x == x && comment.pos.y == y && comment.pos.z == z) {
			result.push_back(&comment);
		}
	}
	return result;
}

const MapComment* MapComments::findById(uint32_t id) const {
	for (const MapComment& comment : comments) {
		if (comment.id == id) {
			return &comment;
		}
	}
	return nullptr;
}

void MapComments::clear() {
	comments.clear();
	nextId = 1;
}

wxString formatMapCommentTime(int64_t createdUnix) {
	if (createdUnix <= 0) {
		return "Unknown time";
	}

	const time_t stamp = static_cast<time_t>(createdUnix);
	tm localTime {};
#if defined(_WIN32)
	localtime_s(&localTime, &stamp);
#else
	localtime_r(&stamp, &localTime);
#endif

	char buffer[64];
	if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime) == 0) {
		return "Unknown time";
	}
	return wxString(buffer, wxConvUTF8);
}
