//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAP_COMMENT_H_
#define RME_MAP_COMMENT_H_

#include "position.h"

#include <cstdint>
#include <string>
#include <vector>

struct MapComment {
	uint32_t id = 0;
	Position pos;
	std::string author;
	std::string text;
	int64_t createdUnix = 0;
};

class MapComments {
public:
	MapComment createComment(const Position& pos, const std::string& author, const std::string& text);
	void upsert(const MapComment& comment);
	bool removeById(uint32_t id);
	void setAll(std::vector<MapComment> newComments, uint32_t nextCommentId);

	const std::vector<MapComment>& all() const {
		return comments;
	}
	std::vector<const MapComment*> atPosition(int x, int y, int z) const;
	const MapComment* findById(uint32_t id) const;

	uint32_t getNextId() const {
		return nextId;
	}
	void setNextId(uint32_t id) {
		nextId = id;
	}

	bool empty() const {
		return comments.empty();
	}
	void clear();

private:
	std::vector<MapComment> comments;
	uint32_t nextId = 1;
};

wxString formatMapCommentTime(int64_t createdUnix);

#endif
