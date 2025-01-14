// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "Scene.hpp"

#include "../XenonCore/Logging.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Xenon
{
	Scene::Scene(Instance& instance, std::unique_ptr<Backend::Camera>&& pCamera)
		: m_UniqueLock(m_Mutex)
		, m_Instance(instance)
		, m_pCamera(std::move(pCamera))
	{
		// Setup connections.
		m_Registry.on_construct<Geometry>().connect<&Scene::onGeometryConstruction>(this);
		m_Registry.on_construct<Material>().connect<&Scene::onMaterialConstruction>(this);

		m_Registry.on_construct<Components::Transform>().connect<&Scene::onTransformComponentConstruction>(this);
		m_Registry.on_update<Components::Transform>().connect<&Scene::onTransformComponentUpdate>(this);
		m_Registry.on_destroy<Components::Transform>().connect<&Scene::onTransformComponentDestruction>(this);

		// Setup the buffers.
		m_pSceneInformationUniform = m_Instance.getFactory()->createBuffer(m_Instance.getBackendDevice(), sizeof(SceneInformation), Backend::BufferType::Uniform);
		m_pLightSourceUniform = m_Instance.getFactory()->createBuffer(m_Instance.getBackendDevice(), sizeof(Components::LightSource) * XENON_MAX_LIGHT_SOURCE_COUNT, Backend::BufferType::Uniform);

		// Unlock the lock so the user can do whatever they want.
		m_UniqueLock.unlock();
	}

	void Scene::beginUpdate()
	{
		if (m_UniqueLock) m_UniqueLock.unlock();
	}

	void Scene::endUpdate()
	{
		if (!m_UniqueLock) m_UniqueLock.lock();

		setupLights();

		m_pSceneInformationUniform->write(ToBytes(&m_SceneInformation), sizeof(SceneInformation));
		m_pCamera->update();

		m_IsUpdatable = false;
	}

	void Scene::cleanup()
	{
		m_Instance.getBackendDevice()->waitIdle();
		if (m_UniqueLock) m_UniqueLock.unlock();

		m_Registry.clear();
		m_pCamera.reset();
		m_pSceneInformationUniform.reset();
		m_pLightSourceUniform.reset();
	}

	XENON_NODISCARD Material& Scene::createMaterial(Group group, MaterialBuilder& builder)
	{
		XENON_TODO_NOW("Find a better system to specialize using the create function.");

		const auto lock = std::scoped_lock(m_Mutex);
		return m_Registry.emplace<Material>(group, m_Instance.getMaterialDatabase().storeSpecification(static_cast<const MaterialSpecification&>(builder)));
	}

	void Scene::setupDescriptor(Backend::Descriptor* pSceneDescriptor, const Backend::RasterizingPipeline* pPipeline)
	{
		// Get all the unique resources.
		std::vector<Backend::ShaderResource> resources = pPipeline->getSpecification().m_VertexShader.getResources();
		for (const auto& resource : pPipeline->getSpecification().m_FragmentShader.getResources())
		{
			if (std::find(resources.begin(), resources.end(), resource) == resources.end())
				resources.emplace_back(resource);
		}

		// Setup the bindings.
		for (const auto& resource : resources)
		{
			// Continue if we have any other resource other than scene.
			if (resource.m_Set != Backend::DescriptorType::Scene)
				continue;

			// Bind everything that we need!
			switch (static_cast<Backend::SceneBindings>(resource.m_Binding))
			{
			case Xenon::Backend::SceneBindings::SceneInformation:
				pSceneDescriptor->attach(resource.m_Binding, m_pSceneInformationUniform.get());
				break;

			case Xenon::Backend::SceneBindings::Camera:
				pSceneDescriptor->attach(resource.m_Binding, m_pCamera->getViewports().front().m_pUniformBuffer);
				break;

			case Xenon::Backend::SceneBindings::LightSources:
				pSceneDescriptor->attach(resource.m_Binding, m_pLightSourceUniform.get());
				break;

			case Xenon::Backend::SceneBindings::AccelerationStructure:
				break;

			case Xenon::Backend::SceneBindings::RenderTarget:
				break;

			default:
				break;
			}
		}
	}

	void Scene::onGeometryConstruction(entt::registry& registry, Group group)
	{
		if (registry.any_of<Material>(group))
		{
			for (const auto& geometry = registry.get<Geometry>(group); const auto & mesh : geometry.getMeshes())
				m_DrawableCount += mesh.m_SubMeshes.size();

			m_DrawableGeometryCount++;
		}
	}

	void Scene::onMaterialConstruction(entt::registry& registry, Group group)
	{
		if (registry.any_of<Geometry>(group))
		{
			for (const auto& geometry = registry.get<Geometry>(group); const auto & mesh : geometry.getMeshes())
				m_DrawableCount += mesh.m_SubMeshes.size();

			m_DrawableGeometryCount++;
		}
	}

	void Scene::onTransformComponentConstruction(entt::registry& registry, Group group)
	{
		const auto& transform = registry.get<Components::Transform>(group);
		const auto modelMatrix = transform.computeModelMatrix();

		auto& uniformBuffer = registry.emplace<Internal::TransformUniformBuffer>(group, m_Instance.getFactory()->createBuffer(m_Instance.getBackendDevice(), sizeof(modelMatrix), Backend::BufferType::Uniform));
		uniformBuffer.m_pUniformBuffer->write(ToBytes(glm::value_ptr(modelMatrix)), sizeof(modelMatrix));
	}

	void Scene::onTransformComponentUpdate(entt::registry& registry, Group group) const
	{
		const auto& transform = registry.get<Components::Transform>(group);
		const auto modelMatrix = transform.computeModelMatrix();

		registry.get<Internal::TransformUniformBuffer>(group).m_pUniformBuffer->write(ToBytes(glm::value_ptr(modelMatrix)), sizeof(modelMatrix));

	}

	void Scene::onTransformComponentDestruction(entt::registry& registry, Group group) const
	{
		registry.remove<Internal::TransformUniformBuffer>(group);
	}

	void Scene::setupLights()
	{
		std::vector<Components::LightSource> lightSources;
		for (const auto group : m_Registry.view<Components::LightSource>())
			lightSources.emplace_back(m_Registry.get<Components::LightSource>(group));

		const auto copySize = lightSources.size() * sizeof(Components::LightSource);
		m_pLightSourceUniform->write(ToBytes(lightSources.data()), copySize);
		m_SceneInformation.m_LightSourceCount = static_cast<uint32_t>(lightSources.size());
	}
}