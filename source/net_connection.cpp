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

#include "main.h"
#include "net_connection.h"

NetworkMessage::NetworkMessage() {
	clear();
}

void NetworkMessage::clear() {
	buffer.resize(4);
	position = 4;
	size = 0;
}

void NetworkMessage::expand(const size_t length) {
	if (position + length >= buffer.size()) {
		buffer.resize(position + length + 1);
	}
	size += length;
}

template <>
std::string NetworkMessage::read<std::string>() {
	const uint32_t length = read<uint32_t>();
	if (length == 0) {
		return std::string();
	}
	if (position + length > buffer.size()) {
		position = buffer.size();
		return std::string();
	}

	char* strBuffer = reinterpret_cast<char*>(&buffer[position]);
	position += length;
	return std::string(strBuffer, length);
}

template <>
Position NetworkMessage::read<Position>() {
	Position position;
	position.x = read<uint16_t>();
	position.y = read<uint16_t>();
	position.z = read<uint8_t>();
	return position;
}

template <>
void NetworkMessage::write<std::string>(const std::string& value) {
	const uint32_t length = static_cast<uint32_t>(value.length());
	write<uint32_t>(length);

	if (length == 0) {
		return;
	}

	expand(length);
	memcpy(&buffer[position], value.data(), length);
	position += length;
}

template <>
void NetworkMessage::write<Position>(const Position& value) {
	write<uint16_t>(value.x);
	write<uint16_t>(value.y);
	write<uint8_t>(value.z);
}

// NetworkConnection
NetworkConnection::NetworkConnection() :
	service(nullptr), workGuard(nullptr), thread(), stopped(false) {
	//
}

NetworkConnection::~NetworkConnection() {
	stop();
}

NetworkConnection& NetworkConnection::getInstance() {
	static NetworkConnection connection;
	return connection;
}

bool NetworkConnection::start() {
	if (thread.joinable()) {
		if (stopped) {
			return false;
		}
		// run() returns when the io_context is stopped; a joinable thread with a
		// stopped service means the network pump died and must be restarted.
		if (service && !service->stopped()) {
			return true;
		}
		thread.join();
		workGuard.reset();
		if (service) {
			delete service;
			service = nullptr;
		}
	}

	stopped = false;
	if (!service) {
		service = new asio::io_context;
	}

	workGuard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
		asio::make_work_guard(*service)
	);

	thread = std::thread([this]() -> void {
		try {
			service->run();
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	});
	return true;
}

void NetworkConnection::stop() {
	if (!service) {
		return;
	}

	workGuard.reset();
	service->stop();
	stopped = true;
	if (thread.joinable()) {
		thread.join();
	}

	delete service;
	service = nullptr;
}

asio::io_context& NetworkConnection::get_service() {
	return *service;
}
