// Copyright 2022 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "VulkanFactory.hpp"

#include "VulkanInstance.hpp"
#include "VulkanDevice.hpp"

namespace Xenon
{
	namespace Backend
	{
		std::unique_ptr<Xenon::Backend::Instance> VulkanFactory::createInstance(const std::string& applicationName, uint32_t applicationVersion)
		{
			return std::make_unique<VulkanInstance>(applicationName, applicationVersion);
		}

		std::unique_ptr<Xenon::Backend::Device> VulkanFactory::createDevice(Instance* pInstance, RenderTargetType requiredRenderTargets)
		{
			return std::make_unique<VulkanDevice>(pInstance->as<VulkanInstance>(), requiredRenderTargets);
		}
	}
}