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

#ifndef RME_LIGHDRAWER_H
#define RME_LIGHDRAWER_H

#include "graphics.h"
#include "position.h"

#include <unordered_map>

class LightDrawer {
	struct Light {
		uint16_t map_x = 0;
		uint16_t map_y = 0;
		uint8_t map_z = 0;
		uint8_t color = 0;
		uint8_t intensity = 0;

		Light(uint16_t map_x, uint16_t map_y, uint8_t map_z, uint8_t color, uint8_t intensity) : map_x(map_x), map_y(map_y), map_z(map_z), color(color), intensity(intensity) { }
	};

public:
	LightDrawer();
	virtual ~LightDrawer();

	void draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y);

	void setGlobalLightColor(uint8_t color);
	void addLight(int map_x, int map_y, int map_z, const SpriteLight& light);
	// Record a tile column that blocks the view of floors below it (solid ground).
	// Used so a light on a lower floor is only visible through openings above it.
	void addOpaqueGround(int map_x, int map_y, int map_z);
	void clear() noexcept;

private:
	void createGLTexture();
	void unloadGLTexture();

	// Applies the same NW parallax shift as the drawn tiles, so coverage and
	// lights share the same buffer columns. Returns false if out of bounds.
	static inline bool applyFloorOffset(int& map_x, int& map_y, int map_z) {
		if (map_z <= GROUND_LAYER) {
			map_x -= (GROUND_LAYER - map_z);
			map_y -= (GROUND_LAYER - map_z);
		}
		return !(map_x <= 0 || map_x >= MAP_MAX_WIDTH || map_y <= 0 || map_y >= MAP_MAX_HEIGHT);
	}

	static inline uint32_t columnKey(int map_x, int map_y) {
		return (static_cast<uint32_t>(map_x) << 16) | static_cast<uint32_t>(map_y);
	}

	inline float calculateIntensity(int map_x, int map_y, const Light& light) {
		int dx = map_x - light.map_x;
		int dy = map_y - light.map_y;
		float distance = std::sqrt(dx * dx + dy * dy);
		if (distance > MaxLightIntensity) {
			return 0.f;
		}
		float intensity = (-distance + light.intensity) * 0.2f;
		if (intensity < 0.01f) {
			return 0.f;
		}
		return std::min(intensity, 1.f);
	}

	GLuint texture;
	std::vector<Light> lights;
	// Per buffer-column, the topmost (smallest z) floor with solid ground.
	// A light is occluded at a column when its floor is below this value.
	std::unordered_map<uint32_t, uint8_t> opaque_ground;
	std::vector<uint8_t> buffer;
	wxColor global_color;
};

#endif
