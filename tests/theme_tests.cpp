#include "../source/theme.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

int main()
{
	const auto dark = ThemeManager::PaletteFor(ThemeMode::Dark);
	assert(dark.window == wxColour(30, 30, 30));
	assert(dark.text != dark.window);
	assert(dark.border != dark.surface);
	assert(dark.hover != dark.selection);
	assert(ThemeManager::NormalizeMode(-1) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(0) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(1) == ThemeMode::Light);
	assert(ThemeManager::NormalizeMode(2) == ThemeMode::System);
	assert(ThemeManager::NormalizeMode(3) == ThemeMode::Dark);
}
