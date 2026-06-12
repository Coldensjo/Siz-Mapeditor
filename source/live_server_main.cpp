//////////////////////////////////////////////////////////////////////
// Standalone console host for live collaborative map editing.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_server.h"
#include "live_peer.h"
#include "live_session_bounds.h"
#include "editor.h"
#include "gui.h"
#include "settings.h"
#include "client_version.h"
#include "net_connection.h"
#include "iomap_otbm.h"
#include "map.h"

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include <wx/app.h>
#include <wx/init.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::atomic<bool> g_running { true };
Editor* g_editor = nullptr;
LiveServer* g_server = nullptr;
unsigned g_autosaveIntervalSeconds = 5 * 60;
std::chrono::steady_clock::time_point g_lastAutosave = std::chrono::steady_clock::now();
std::mutex g_commandMutex;
std::vector<std::string> g_pendingCommands;

void printUsage() {
	std::cout << "MapServer - Live collaboration host for Remere's Map Editor\n"
	          << "Usage: MapServer_x64.exe --map <file.otbm> [options]\n"
	          << "Options:\n"
	          << "  --port <number>       TCP port (default 31313)\n"
	          << "  --password <pass>     Session password (default: empty)\n"
	          << "  --name <name>         Session display name\n"
	          << "  --autosave <minutes>  Auto-save interval (default 5, 0 disables)\n"
	          << "  --x <coord>           Session center X (requires --y, --z, --radius)\n"
	          << "  --y <coord>           Session center Y\n"
	          << "  --z <floor>           Session center floor\n"
	          << "  --radius <sqm>        Editable/viewable radius around center\n"
	          << "  --assets <directory>  Folder containing Tibia.dat/.spr and items.otb/.xml\n"
	          << "\nConsole commands: save, list, kick <name>, exit\n";
}

size_t countMapCreatures(Map& map) {
	size_t count = 0;
	for (MapIterator it = map.begin(); it != map.end(); ++it) {
		Tile* tile = (*it)->get();
		if (tile && tile->creature) {
			++count;
		}
	}
	return count;
}

void logMapLoadSummary(Map& map) {
	std::cout << "[live] House file: " << map.getHouseFilename() << std::endl;
	std::cout << "[live] Houses on map: " << map.houses.count() << std::endl;
	std::cout << "[live] Spawn file: " << map.getSpawnFilename() << std::endl;
	std::cout << "[live] Creatures on map: " << countMapCreatures(map) << std::endl;

	if (!map.hasWarnings()) {
		return;
	}

	std::cout << "[live] Map loader warnings:" << std::endl;
	for (const wxString& warning : map.getWarnings()) {
		std::cout << "  " << warning.ToStdString() << std::endl;
	}
}

bool saveHostedMap(bool automatic) {
	if (!g_editor) {
		return false;
	}

	FileName filename = g_editor->getMap().getFilename();
	if (filename.GetFullPath().empty()) {
		std::cout << "[live] Map has no filename; cannot save." << std::endl;
		return false;
	}

	if (!g_editor->saveMap(filename, false)) {
		std::cout << "[live] Save failed." << std::endl;
		return false;
	}

	g_lastAutosave = std::chrono::steady_clock::now();

	std::cout << "[live] " << (automatic ? "Auto-saved" : "Saved")
	          << " map to " << filename.GetFullPath().ToStdString() << std::endl;
	return true;
}

void tickAutosave() {
	if (!g_editor || g_autosaveIntervalSeconds == 0) {
		return;
	}

	if (!g_editor->getMap().hasChanged()) {
		return;
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::steady_clock::now() - g_lastAutosave
	).count();

	if (elapsed >= static_cast<long long>(g_autosaveIntervalSeconds)) {
		saveHostedMap(true);
	}
}

bool parseArgs(int argc, wxChar** argv, FileName& mapPath, uint16_t& port, wxString& password, wxString& sessionName, LiveSessionBounds& sessionBounds, FileName& assetsPath) {
	port = 31313;
	password = wxEmptyString;
	sessionName = wxEmptyString;
	sessionBounds = LiveSessionBounds {};
	assetsPath.Clear();

	int boundsX = -1;
	int boundsY = -1;
	int boundsZ = -1;
	int boundsRadius = -1;

	for (int i = 1; i < argc; ++i) {
		wxString arg = argv[i];
		if (arg == wxT("--help") || arg == wxT("-h")) {
			printUsage();
			return false;
		} else if (arg == wxT("--map") && i + 1 < argc) {
			mapPath = FileName(argv[++i]);
		} else if (arg == wxT("--port") && i + 1 < argc) {
			long parsedPort = wxAtoi(argv[++i]);
			if (parsedPort < 1 || parsedPort > 65535) {
				std::cerr << "Invalid port number." << std::endl;
				return false;
			}
			port = static_cast<uint16_t>(parsedPort);
		} else if (arg == wxT("--password") && i + 1 < argc) {
			password = argv[++i];
		} else if (arg == wxT("--name") && i + 1 < argc) {
			sessionName = argv[++i];
		} else if (arg == wxT("--autosave") && i + 1 < argc) {
			long autosaveMinutes = wxAtoi(argv[++i]);
			if (autosaveMinutes < 0) {
				std::cerr << "Invalid autosave interval." << std::endl;
				return false;
			}
			g_autosaveIntervalSeconds = static_cast<unsigned>(autosaveMinutes * 60);
		} else if (arg == wxT("--x") && i + 1 < argc) {
			boundsX = wxAtoi(argv[++i]);
		} else if (arg == wxT("--y") && i + 1 < argc) {
			boundsY = wxAtoi(argv[++i]);
		} else if (arg == wxT("--z") && i + 1 < argc) {
			boundsZ = wxAtoi(argv[++i]);
		} else if (arg == wxT("--radius") && i + 1 < argc) {
			boundsRadius = wxAtoi(argv[++i]);
		} else if (arg == wxT("--assets") && i + 1 < argc) {
			assetsPath = FileName(argv[++i]);
		} else {
			std::cerr << "Unknown argument: " << arg.ToStdString() << std::endl;
			return false;
		}
	}

	if (!mapPath.FileExists()) {
		std::cerr << "Map file not found. Use --map <file.otbm>." << std::endl;
		printUsage();
		return false;
	}

	const bool anyBoundsArg = boundsX >= 0 || boundsY >= 0 || boundsZ >= 0 || boundsRadius >= 0;
	if (anyBoundsArg) {
		if (boundsX < 0 || boundsY < 0 || boundsZ < 0 || boundsRadius <= 0) {
			std::cerr << "Session bounds require all of --x, --y, --z, and --radius." << std::endl;
			return false;
		}
		if (boundsZ < 0 || boundsZ > 15) {
			std::cerr << "Invalid session floor (0-15)." << std::endl;
			return false;
		}
		sessionBounds.enabled = true;
		sessionBounds.centerX = static_cast<uint16_t>(boundsX);
		sessionBounds.centerY = static_cast<uint16_t>(boundsY);
		sessionBounds.centerZ = static_cast<uint8_t>(boundsZ);
		sessionBounds.radius = static_cast<uint32_t>(boundsRadius);
	}
	return true;
}

bool configureAssetsForMap(const FileName& mapPath, const FileName& assetsPath) {
	MapVersion version;
	if (!IOMapOTBM::getVersionInfo(mapPath, version)) {
		std::cerr << "Could not read client version from map file." << std::endl;
		return false;
	}

	ClientVersion* clientVersion = ClientVersion::get(version.client);
	if (clientVersion == nullptr) {
		std::cerr << "Unsupported map client version." << std::endl;
		return false;
	}

	wxArrayString searchDirs;
	if (assetsPath.DirExists()) {
		searchDirs.Add(assetsPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
	}
	searchDirs.Add(g_gui.GetExecDirectory());

	wxString workingDirectory;
	workingDirectory = wxGetCwd();
	if (!workingDirectory.empty()) {
		workingDirectory = wxGetCwd();
		if (!workingDirectory.EndsWith(FileName::GetPathSeparator())) {
			workingDirectory << FileName::GetPathSeparator();
		}
		searchDirs.Add(workingDirectory);
	}

	for (size_t i = 0; i < searchDirs.size(); ++i) {
		const wxString candidate = searchDirs[i];
		bool duplicate = false;
		for (size_t j = 0; j < i; ++j) {
			if (searchDirs[j] == candidate) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) {
			continue;
		}

		if (clientVersion->tryConfigureFromDirectory(FileName(candidate))) {
			std::cout << "[live] Using assets from " << candidate.ToStdString() << std::endl;
			ClientVersion::saveVersions();
			return true;
		}
	}

	std::cerr << "Could not locate Tibia.dat/Tibia.spr for map client version "
	          << clientVersion->getName() << "." << std::endl;
	std::cerr << "Place the asset files next to MapServer_x64.exe or pass --assets <directory>."
	          << std::endl;
	return false;
}

void processCommand(const std::string& line) {
	std::istringstream iss(line);
	std::string cmd;
	iss >> cmd;
	if (cmd.empty()) {
		return;
	}

	if (cmd == "exit" || cmd == "quit") {
		g_running = false;
		if (g_server) {
			g_server->close();
		}
	} else if (cmd == "save") {
		saveHostedMap(false);
	} else if (cmd == "list") {
		if (!g_server) {
			return;
		}
		const auto& clients = g_server->getClients();
		if (clients.empty()) {
			std::cout << "[live] No clients connected." << std::endl;
			return;
		}
		std::cout << "[live] Connected clients:" << std::endl;
		for (const auto& entry : clients) {
			LivePeer* peer = entry.second;
			std::cout << "  - " << peer->getName().ToStdString()
			          << " (" << peer->getHostName() << ")" << std::endl;
		}
	} else if (cmd == "kick") {
		std::string name;
		iss >> name;
		if (name.empty()) {
			std::cout << "[live] Usage: kick <name>" << std::endl;
			return;
		}
		if (g_server) {
			g_server->kickClient(wxstr(name));
		}
	} else {
		std::cout << "[live] Unknown command. Try: save, list, kick <name>, exit" << std::endl;
	}
}

void drainPendingCommands() {
	std::vector<std::string> commands;
	{
		std::lock_guard<std::mutex> lock(g_commandMutex);
		commands.swap(g_pendingCommands);
	}
	for (const std::string& command : commands) {
		processCommand(command);
	}
}

} // namespace

class MapServerApp : public wxAppConsole {
public:
	bool OnInit() override {
#if defined(__LINUX__) || defined(__WINDOWS__)
		int glutArgc = 1;
		char* glutArgv[1] = { const_cast<char*>("MapServer") };
		glutInit(&glutArgc, glutArgv);
#endif

		mt_seed(time(nullptr));
		srand(time(nullptr));

		g_gui.SetHeadless(true);
		g_gui.discoverDataDirectory("clients.xml");
		g_settings.load();
		ClientVersion::loadVersions();
		g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, 1);

		FileName mapPath;
		uint16_t port = 31313;
		wxString password;
		wxString sessionName;
		LiveSessionBounds sessionBounds;
		FileName assetsPath;
		if (!parseArgs(argc, argv, mapPath, port, password, sessionName, sessionBounds, assetsPath)) {
			return false;
		}

		if (!configureAssetsForMap(mapPath, assetsPath)) {
			return false;
		}

		try {
			g_editor = newd Editor(g_gui.copybuffer, mapPath);
		} catch (const std::runtime_error& e) {
			std::cerr << "Failed to load map: " << e.what() << std::endl;
			return false;
		} catch (const std::string& e) {
			std::cerr << "Failed to load map: " << e << std::endl;
			return false;
		}

		if (!sessionName.empty()) {
			g_editor->getMap().setName(nstr(sessionName));
		}

		Map& map = g_editor->getMap();
		if (!IOMapOTBM::loadAuxiliaryHouses(map, mapPath)) {
			std::cerr << "[live] Warning: no house data loaded." << std::endl;
		}
		if (countMapCreatures(map) == 0) {
			if (!IOMapOTBM::loadAuxiliarySpawns(map, mapPath)) {
				std::cerr << "[live] Warning: no creatures loaded from spawn file." << std::endl;
			}
		}
		if (!IOMapOTBM::loadAuxiliaryComments(map, mapPath)) {
			std::cerr << "[live] Warning: no map comments loaded." << std::endl;
		}
		logMapLoadSummary(map);

		g_server = g_editor->StartLiveServer();
		g_server->setPort(port);
		g_server->setPassword(password);
		if (sessionBounds.enabled) {
			g_server->setSessionBounds(sessionBounds);
		}

		if (!g_server->bind()) {
			std::cerr << g_server->getLastError().ToStdString() << std::endl;
			return false;
		}

		std::cout << "[live] Hosting '" << g_editor->getMap().getName()
		          << "' on port " << port << std::endl;
		if (sessionBounds.enabled) {
			std::cout << "[live] Session area: center ("
			          << sessionBounds.centerX << ","
			          << sessionBounds.centerY << ","
			          << static_cast<unsigned>(sessionBounds.centerZ) << ") radius "
			          << sessionBounds.radius << std::endl;
		}
		if (g_autosaveIntervalSeconds > 0) {
			std::cout << "[live] Auto-save every " << (g_autosaveIntervalSeconds / 60)
			          << " minute(s) with timestamped backups." << std::endl;
		} else {
			std::cout << "[live] Auto-save disabled." << std::endl;
		}
		std::cout << "[live] Type 'save', 'list', 'kick <name>', or 'exit'." << std::endl;
		g_lastAutosave = std::chrono::steady_clock::now();
		return true;
	}

	int OnRun() override {
		std::thread inputThread([]() {
			std::string line;
			while (g_running && std::getline(std::cin, line)) {
				{
					std::lock_guard<std::mutex> lock(g_commandMutex);
					g_pendingCommands.push_back(line);
				}
			}
			g_running = false;
		});

		while (g_running) {
			while (Pending()) {
				Dispatch();
			}
			drainPendingCommands();
			tickAutosave();
			wxMilliSleep(10);
		}

		if (inputThread.joinable()) {
			inputThread.join();
		}

		if (g_editor && g_editor->getMap().hasChanged()) {
			std::cout << "[live] Saving pending changes before shutdown..." << std::endl;
			saveHostedMap(true);
		}

		NetworkConnection::getInstance().stop();

		if (g_editor) {
			g_editor->CloseLiveServer();
			delete g_editor;
			g_editor = nullptr;
			g_server = nullptr;
		}

		return 0;
	}
};

wxIMPLEMENT_APP_CONSOLE(MapServerApp);
