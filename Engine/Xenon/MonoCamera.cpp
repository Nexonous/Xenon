// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "MonoCamera.hpp"

#include <optick.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Xenon
{
	MonoCamera::MonoCamera(Instance& instance, uint32_t width, uint32_t height)
		: Camera(width, height)
		, m_BackendType(instance.getBackendType())
	{
		// Create the uniform buffer.
		m_pUniformBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), sizeof(CameraBuffer), Backend::BufferType::Uniform);

		// Setup the viewport.
		m_Viewport.m_pUniformBuffer = m_pUniformBuffer.get();
		m_Viewport.m_Width = static_cast<float>(width);
		m_Viewport.m_Height = static_cast<float>(height);
	}

	void MonoCamera::update()
	{
		OPTICK_EVENT();

		glm::vec3 front = {};
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		m_Front = glm::normalize(front);
		m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		// Calculate the matrices.
		m_CameraBuffer.m_View = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_CameraBuffer.m_Projection = glm::perspective(glm::radians(m_FieldOfView), m_AspectRatio, m_NearPlane, m_FarPlane);

#ifdef XENON_PLATFORM_WINDOWS
		// Flip the projection if we're on Vulkan.
		if (m_BackendType == BackendType::Vulkan)
			m_CameraBuffer.m_Projection[1][1] *= -1.0f;
#else
		m_CameraBuffer.m_Projection[1][1] *= -1.0f;

#endif // XENON_PLATFORM_WINDOWS


		// Copy the data to the uniform buffer.
		m_pUniformBuffer->write(ToBytes(&m_CameraBuffer), sizeof(CameraBuffer));
	}
}