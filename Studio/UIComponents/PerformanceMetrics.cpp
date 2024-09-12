// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "PerformanceMetrics.hpp"

#include "XenonCore/Common.hpp"

#include <imgui.h>

#include <algorithm>
#include <numeric>

void PerformanceMetrics::begin(std::chrono::nanoseconds delta)
{
	XENON_NAMESPACE_RANGES::rotate(m_FrameRates.begin(), m_FrameRates.end() - 1, m_FrameRates.end());
	m_FrameRates[0] = std::nano::den / static_cast<float>(delta.count());

	if (m_bIsOpen)
	{
		if (ImGui::Begin("Performance Metrics", &m_bIsOpen))
		{
			// Plot the frame rate graph.
			ImGui::Text("Frame Timing");
			ImGui::Separator();
			ImGui::PlotLines("Frame Rate", m_FrameRates.data(), static_cast<int32_t>(m_FrameRates.size()));
			ImGui::Text("Average frame rate: %f FPS", std::accumulate(m_FrameRates.begin(), m_FrameRates.end(), 0.0f) / static_cast<float>(m_FrameRates.size()));
			ImGui::Spacing();

			// Show the draw call count.
			ImGui::Text("Draw Calls");
			ImGui::Separator();
			ImGui::Text("Total draw call count: %u", m_TotalDrawCount);
			ImGui::Text("Actual draw call count: %u", m_ActualDrawCount);
			ImGui::Text("Occluded draw count: %u", m_TotalDrawCount - m_ActualDrawCount);
		}

		ImGui::End();
	}
}

void PerformanceMetrics::end()
{
}

void PerformanceMetrics::setDrawCallCount(uint64_t totalCount, uint64_t actualCount)
{
	m_TotalDrawCount = totalCount;
	m_ActualDrawCount = actualCount;
}
