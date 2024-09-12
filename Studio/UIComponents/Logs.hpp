// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UIComponent.hpp"

#include <spdlog/sinks/base_sink.h>

#include <vector>
#include <mutex>

/**
 * Logs class.
 * This UI component displays logs sent using a custom spdlog sink.
 */
class Logs final : public UIComponent, public spdlog::sinks::base_sink<std::mutex>
{
public:
	/**
	 * Default constructor.
	 */
	Logs() = default;

	/**
	 * Begin the component draw.
	 *
	 * @delta The time difference between the previous frame and the current frame in nanoseconds.
	 */
	void begin(std::chrono::nanoseconds delta) override;

	/**
	 * End the component draw.
	 */
	void end() override;

private:
	/**
	 * Sink it function override.
	 *
	 * @param msg The message to log.
	 */
	void sink_it_(const spdlog::details::log_msg& msg) override;

	/**
	 * Flush function override.
	 */
	void flush_() override;

private:
	std::vector<std::pair<std::string, spdlog::level::level_enum>> m_Messages;
};