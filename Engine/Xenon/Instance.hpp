// Copyright 2022 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonCore/Common.hpp"

#include "../XenonBackend/IFactory.hpp"

#include <string>

namespace Xenon
{
	namespace Globals
	{
		/**
		 * This variable contains the global backend object factory which is backend specific.
		 * It is used by the frontend to create the required backend objects.
		 */
		inline std::unique_ptr<Backend::IFactory> BackendFactory = nullptr;
	}

	/**
	 * Instance class.
	 * This is the main class which the user needs to instantiate to use the engine.
	 *
	 * If the requested render target types are not available by the device, it will only enable the render targets which are supported.
	 * A warning will be issued for this issue.
	 */
	class Instance final
	{
	public:
		/**
		 * Explicit constructor.
		 *
		 * @param applicationName The name of the application.
		 * @param applicationVersion The version of the application.
		 * @param renderTargets The render targets which the application will use.
		 */
		explicit Instance(const std::string& applicationName, uint32_t applicationVersion, RenderTargetType renderTargets);

		/**
		 * Destructor.
		 */
		~Instance();

	public:
		/**
		 * Get the application name.
		 *
		 * @return The application name string view.
		 */
		[[nodiscard]] const std::string_view getApplicationName() const { return m_ApplicationName; }

		/**
		 * Get the application version.
		 *
		 * @return The application version.
		 */
		[[nodiscard]] uint32_t getApplicationVersion() const { return m_ApplicationVersion; }

		/**
		 * Get the supported render target types.
		 *
		 * @return The render target types.
		 */
		[[nodsicard]] RenderTargetType getSupportedRenderTargetTypes() const { return m_RenderTargets; }

	private:
		std::string m_ApplicationName;
		uint32_t m_ApplicationVersion;
		RenderTargetType m_RenderTargets;

		std::unique_ptr<Backend::Instance> m_pInstance = nullptr;
		std::unique_ptr<Backend::Device> m_pDevice = nullptr;
	};
}