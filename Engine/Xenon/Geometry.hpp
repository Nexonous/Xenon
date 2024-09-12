// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Instance.hpp"
#include "Material.hpp"

#include <filesystem>

namespace Xenon
{
	/**
	 * Primitive mode.
	 * This defines what the primitive mode is for a single
	 */
	enum class PrimitiveMode : uint8_t
	{
		Points,
		Line,
		LineLoop,
		LineStrip,
		Triangles,
		TriangleStrip,
		TriangleFan
	};

	/**
	 * Sub-mesh structure.
	 * Sub-meshes are the building blocks of a mesh.
	 */
	struct SubMesh final
	{
		Texture m_BaseColorTexture = {};
		Texture m_RoughnessTexture = {};
		Texture m_NormalTexture = {};
		Texture m_OcclusionTexture = {};
		Texture m_EmissiveTexture = {};

		uint64_t m_VertexOffset = 0;
		uint64_t m_VertexCount = 0;

		uint64_t m_IndexOffset = 0;
		uint64_t m_IndexCount = 0;		// If this is set to 0, it will draw using the vertices.

		PrimitiveMode m_Mode = PrimitiveMode::Triangles;
		uint8_t m_IndexSize = 0;

		/**
		 * Is equals operator overload.
		 *
		 * @param other The other sub-mesh to compare with.
		 * @return True if the two sub-meshes are equal.
		 * @return False if the they're not equal.
		 */
		XENON_NODISCARD bool operator==(const SubMesh& other) const = default;
	};

	/**
	 * Mesh structure.
	 * This contains information about a single mesh instance.
	 */
	struct Mesh final
	{
		std::string m_Name;
		std::vector<SubMesh> m_SubMeshes;
	};

	/**
	 * Geometry class.
	 * This class contains all the meshes of a single model, with or without animation.
	 *
	 * All meshes are stored in a tree like hierarchy. But for performance, all the mesh data (vertex data and index data) are stored in individual vertex and index buffers, and
	 * mesh specific information (offsets, names, materials, etc...) are stored in a vector where each mesh contains information about child nodes and so on.
	 */
	class Geometry final
	{
		using ImageAndImageViewContainer = std::vector<std::pair<std::unique_ptr<Backend::Image>, std::unique_ptr<Backend::ImageView>>>;
		using ImageSamplerContainer = std::vector<std::unique_ptr<Backend::ImageSampler>>;

	public:
		/**
		 * Default constructor.
		 */
		Geometry() = default;

		/**
		 * Load the meshes from a file and create the geometry class.
		 *
		 * @param instance The instance reference.
		 * @param file The file path to load the data from.
		 * @return The created geometry.
		 */
		XENON_NODISCARD static Geometry FromFile(Instance& instance, const std::filesystem::path& file);

		/**
		 * Create a quad geometry.
		 *
		 * @param instance The instance reference.
		 * @return The created geometry.
		 */
		XENON_NODISCARD static Geometry CreateQuad(Instance& instance);

		/**
		 * Create an image from loading the data from a file.
		 *
		 * @param instance The instance reference.
		 * @param file The image file to load from.
		 * @return The created image pointer.
		 */
		XENON_NODISCARD static std::unique_ptr<Backend::Image> CreateImageFromFile(Instance& instance, const std::filesystem::path& file);

		/**
		 * Get the vertex specification.
		 *
		 * @return The vertex specification.
		 */
		XENON_NODISCARD const Backend::VertexSpecification& getVertexSpecification() const { return m_VertexSpecification; }

		/**
		 * Get the index buffer.
		 *
		 * @return The index buffer pointer.
		 */
		XENON_NODISCARD Backend::Buffer* getIndexBuffer() { return m_pIndexBuffer.get(); }

		/**
		 * Get the index buffer.
		 *
		 * @return The index buffer pointer.
		 */
		XENON_NODISCARD const Backend::Buffer* getIndexBuffer() const { return m_pIndexBuffer.get(); }

		/**
		 * Get the vertex buffer.
		 *
		 * @return The vertex buffer pointer.
		 */
		XENON_NODISCARD Backend::Buffer* getVertexBuffer() { return m_pVertexBuffer.get(); }

		/**
		 * Get the vertex buffer.
		 *
		 * @return The vertex buffer pointer.
		 */
		XENON_NODISCARD const Backend::Buffer* getVertexBuffer() const { return m_pVertexBuffer.get(); }

		/**
		 * Get the meshes.
		 *
		 * @return The meshes.
		 */
		XENON_NODISCARD std::vector<Mesh>& getMeshes() { return m_Meshes; }

		/**
		 * Get the meshes.
		 *
		 * @return The meshes.
		 */
		XENON_NODISCARD const std::vector<Mesh>& getMeshes() const { return m_Meshes; }

		/**
		 * Get the image and it's image view objects.
		 *
		 * @return The image and image view container reference.
		 */
		XENON_NODISCARD const ImageAndImageViewContainer& getImageAndImageViews() const noexcept { return m_pImageAndImageViews; }

		/**
		 * Get the stored image samplers.
		 *
		 * @return The image samplers.
		 */
		XENON_NODISCARD const ImageSamplerContainer& getImageSamplers() const noexcept { return m_pImageSamplers; }

	private:
		std::unique_ptr<Backend::Buffer> m_pIndexBuffer = nullptr;
		std::unique_ptr<Backend::Buffer> m_pVertexBuffer = nullptr;

		std::vector<std::unique_ptr<Backend::Buffer>> m_LODs;

		ImageAndImageViewContainer m_pImageAndImageViews;
		ImageSamplerContainer m_pImageSamplers;

		std::vector<Mesh> m_Meshes;

		Backend::VertexSpecification m_VertexSpecification;
	};
}

namespace std
{
	/**
	 * std::hash specialization for the Xenon SubMesh structure.
	 */
	template<>
	struct hash<Xenon::SubMesh>
	{
		std::size_t operator()(const Xenon::SubMesh& subMesh) const
		{
			return Xenon::GenerateHash(Xenon::ToBytes(&subMesh), sizeof(Xenon::SubMesh));
		}
	};
}