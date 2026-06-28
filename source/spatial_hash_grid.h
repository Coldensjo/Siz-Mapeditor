//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_SPATIAL_HASH_GRID_H
#define RME_SPATIAL_HASH_GRID_H

#include <cstdint>
#include <unordered_map>

class QTreeNode;

// Caches quadtree leaf lookups so the hot getTile path can skip re-walking the
// 7-level node tree on every access. Each leaf covers a 4x4 tile block, keyed by
// the upper 14 bits of the x/y coordinate (exactly the bits the tree navigation
// in QTreeNode::getLeaf consumes).
//
// Leaf pointers are stable for the entire lifetime of the owning BaseMap: leaves
// are only ever freed by the recursive ~QTreeNode at full teardown (there is no
// per-leaf pruning), so caching raw pointers cannot dangle while the map lives.
// The grid is purely an accelerator over the quadtree, which remains the source
// of truth; on map clear the cache is dropped and lazily rebuilt.
class SpatialHashGrid {
public:
	SpatialHashGrid() = default;

	// Identifies the leaf (4x4 block) that owns world coordinate (x, y).
	static uint32_t cellKey(int x, int y) {
		const uint32_t cx = static_cast<uint16_t>(x) >> 2;
		const uint32_t cy = static_cast<uint16_t>(y) >> 2;
		return (cx << 14) | cy; // 14 bits each, fits in 28 bits
	}

	// Returns the cached leaf for this key, or nullptr if not cached yet.
	QTreeNode* find(uint32_t key) const {
		if (key == last_key) {
			return last_leaf; // single-entry fast path for spatially-local access
		}
		const auto it = cells.find(key);
		if (it == cells.end()) {
			return nullptr;
		}
		last_key = key;
		last_leaf = it->second;
		return it->second;
	}

	void insert(uint32_t key, QTreeNode* leaf) {
		cells[key] = leaf;
		last_key = key;
		last_leaf = leaf;
	}

	void clear() {
		cells.clear();
		last_key = INVALID_KEY;
		last_leaf = nullptr;
	}

private:
	// Real keys only use 28 bits, so all-ones can never collide with one.
	static constexpr uint32_t INVALID_KEY = 0xFFFFFFFFu;

	std::unordered_map<uint32_t, QTreeNode*> cells;
	mutable uint32_t last_key = INVALID_KEY;
	mutable QTreeNode* last_leaf = nullptr;
};

#endif
