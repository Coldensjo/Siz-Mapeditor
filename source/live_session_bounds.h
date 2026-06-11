//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_LIVE_SESSION_BOUNDS_H_
#define RME_LIVE_SESSION_BOUNDS_H_

#include <cstdint>

struct LiveSessionBounds {
	bool enabled = false;
	uint16_t centerX = 0;
	uint16_t centerY = 0;
	uint8_t centerZ = 7;
	uint32_t radius = 0;

	bool contains(int x, int y) const {
		if (!enabled) {
			return true;
		}
		const int64_t dx = static_cast<int64_t>(x) - centerX;
		const int64_t dy = static_cast<int64_t>(y) - centerY;
		const int64_t r = radius;
		return dx * dx + dy * dy <= r * r;
	}

	bool intersectsLeaf(int leafTileX, int leafTileY) const {
		if (!enabled) {
			return true;
		}
		for (int x = 0; x < 4; ++x) {
			for (int y = 0; y < 4; ++y) {
				if (contains(leafTileX + x, leafTileY + y)) {
					return true;
				}
			}
		}
		return false;
	}
};

#endif
