//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAP_VIEW_STATE_H_
#define RME_MAP_VIEW_STATE_H_

#include "position.h"

#include <string>

class Editor;

std::string makeLocalMapViewKey(const std::string& filename);
std::string makeLiveMapViewKey(const std::string& host, uint16_t port);

std::string getMapViewKey(const Editor& editor);

void saveMapViewPosition(const std::string& key, const Position& position);
bool loadMapViewPosition(const std::string& key, Position& position);

#endif
