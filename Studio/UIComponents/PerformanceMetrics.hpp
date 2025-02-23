// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UIComponent.hpp"

#include <vector>

/**
 * Performance metrics class.
 * This UI component contains some performance metrics information like the frame rate graph.
 */
class PerformanceMetrics final : public UIComponent
{
public:
	/**
	 * Default constructor.
	 */
	PerformanceMetrics() = default;

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

	/**
	 * Set the draw call count.
	 *
	 * @param totalCount The total draw call count.
	 * @param actualCount The actual draw call count.
	 */
	void setDrawCallCount(uint64_t totalCount, uint64_t actualCount);

private:
	std::vector<float> m_FrameRates = std::vector<float>(10);

	uint64_t m_TotalDrawCount = 0;
	uint64_t m_ActualDrawCount = 0;
};