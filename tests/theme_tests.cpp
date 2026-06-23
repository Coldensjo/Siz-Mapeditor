#include "../source/theme.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

int main()
{
	assert(ThemeManager::PaletteFor(ThemeMode::Dark).window == wxColour(30, 30, 30));
	assert(ThemeManager::NormalizeMode(-1) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(0) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(1) == ThemeMode::Light);
	assert(ThemeManager::NormalizeMode(2) == ThemeMode::System);
	assert(ThemeManager::NormalizeMode(3) == ThemeMode::Dark);
}
