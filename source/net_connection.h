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

#ifndef RME_NET_CONNECTION_H_
#define RME_NET_CONNECTION_H_

#include "position.h"

#include <boost/asio.hpp>

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <thread>
#include <functional>
#include <memory>

// This fork links against Boost.Asio (see main.h). Alias the standalone-asio
// namespace used by the upstream live editing code to Boost so the rest of the
// implementation can stay close to upstream.
namespace asio = boost::asio;
typedef boost::system::error_code net_error_code;

struct NetworkMessage {
	NetworkMessage();

	void clear();
	void expand(const size_t length);

	template <typename T>
	T read() {
		if (position + sizeof(T) > buffer.size()) {
			// Truncated/corrupt packet: flag it so the caller can abort instead of
			// reading out of bounds or silently consuming garbage.
			overflow = true;
			position = buffer.size();
			return T {};
		}
		T value;
		memcpy(&value, &buffer[position], sizeof(T));
		position += sizeof(T);
		return value;
	}

	template <typename T>
	void write(const T& value) {
		expand(sizeof(T));
		memcpy(&buffer[position], &value, sizeof(T));
		position += sizeof(T);
	}

	std::vector<uint8_t> buffer;
	size_t position;
	size_t size;
	// Set when a read runs past the end of the buffer (truncated/corrupt packet).
	bool overflow = false;
};

template <>
std::string NetworkMessage::read<std::string>();
template <>
Position NetworkMessage::read<Position>();
template <>
void NetworkMessage::write<std::string>(const std::string& value);
template <>
void NetworkMessage::write<Position>(const Position& value);

// Owns the single asio io_context and the background thread that pumps it.
// Shared between the live client and live server.
class NetworkConnection {
private:
	NetworkConnection();
	NetworkConnection(const NetworkConnection& copy) = delete;

public:
	~NetworkConnection();

	static NetworkConnection& getInstance();

	bool start();
	void stop();

	asio::io_context& get_service();

	// Queue work onto the network thread (required for socket I/O).
	template<typename Handler>
	void post(Handler&& handler) {
		asio::post(get_service(), std::forward<Handler>(handler));
	}

	// Run immediately when already on the network thread, otherwise queue.
	template<typename Handler>
	void dispatch(Handler&& handler) {
		asio::dispatch(get_service(), std::forward<Handler>(handler));
	}

private:
	asio::io_context* service;
	std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> workGuard;
	std::thread thread;
	bool stopped;
};

#endif
