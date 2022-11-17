// Copyright 2022 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Device.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Pipeline cache handler class.
		 * This class specifies how to handle the pipeline cache of a pipeline.
		 */
		class PipelineCacheHandler
		{
		public:
			/**
			 * Default constructor.
			 */
			PipelineCacheHandler() = default;

			/**
			 * Default virtual destructor.
			 */
			virtual ~PipelineCacheHandler() = default;

			/**
			 * Load the cache data from the store.
			 *
			 * @return The pipeline cache.
			 */
			[[nodiscard]] virtual std::vector<std::byte> load() = 0;

			/**
			 * Store the cache data generated from the backend.
			 *
			 * @param bytes The bytes to store.
			 */
			virtual void store(const std::vector<std::byte>& bytes) = 0;
		};

		/**
		 * Pipeline class.
		 * This is the base class for all the pipelines in the engine.
		 */
		class Pipeline : public BackendObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param pCacheHandler The cache handler pointer. This can be null in which case the pipeline creation might get slow.
			 */
			explicit Pipeline([[maybe_unused]] Device* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler) : m_pCacheHandler(std::move(pCacheHandler)) {}

		private:
			std::unique_ptr<PipelineCacheHandler> m_pCacheHandler = nullptr;
		};
	}
}