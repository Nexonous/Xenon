// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.hpp"
#include "LODTree.hpp"

#include "../XenonCore/Logging.hpp"
#include "../XenonCore/CountingFence.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <optick.h>

#include <latch>
#include <fstream>

constexpr std::array<const char*, 21> g_Attributes = {
	"POSITION",
	"NORMAL",
	"TANGENT",
	"COLOR_0",
	"COLOR_1",
	"COLOR_2",
	"COLOR_3",
	"COLOR_4",
	"COLOR_5",
	"COLOR_6",
	"COLOR_7",
	"TEXCOORD_0",
	"TEXCOORD_1",
	"TEXCOORD_2",
	"TEXCOORD_3",
	"TEXCOORD_4",
	"TEXCOORD_5",
	"TEXCOORD_6",
	"TEXCOORD_7",
	"JOINTS_0",
	"WEIGHTS_0",
};

/**
 * Texture info type concept.
 */
template<class Type>
concept TextureInfo = (std::same_as<Type, tinygltf::TextureInfo> || std::same_as<Type, tinygltf::NormalTextureInfo> || std::same_as<Type, tinygltf::OcclusionTextureInfo>);

namespace /* anonymous */
{
	/**
	 * Check if the attribute exists in the primitive and if so, setup the vertex specification for that element.
	 *
	 * @param model The model to get the element information.
	 * @param primitive The primitive containing the attribute information.
	 * @param attribute The attribute name to check.
	 * @param element The vertex element.
	 * @param specification The specification to configure.
	 * @return The size of the vertex buffer to store all the vertices.
	 */
	uint64_t ResolvePrimitive(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::string& attribute,
		Xenon::Backend::InputElement element,
		Xenon::Backend::VertexSpecification& specification)
	{
		OPTICK_EVENT();

		if (!primitive.attributes.contains(attribute))
			return 0;

		const auto index = primitive.attributes.at(attribute);
		const auto& accessor = model.accessors[index];
		const auto& bufferView = model.bufferViews[accessor.bufferView];

		// Setup the data type.
		Xenon::Backend::AttributeDataType dataType = Xenon::Backend::AttributeDataType::Vec3;
		switch (accessor.type)
		{
		case TINYGLTF_TYPE_VEC2:
			dataType = Xenon::Backend::AttributeDataType::Vec2;
			break;

		case TINYGLTF_TYPE_VEC3:
			dataType = Xenon::Backend::AttributeDataType::Vec3;
			break;

		case TINYGLTF_TYPE_VEC4:
			dataType = Xenon::Backend::AttributeDataType::Vec4;
			break;

		case TINYGLTF_TYPE_MAT2:
			dataType = Xenon::Backend::AttributeDataType::Mat2;
			break;

		case TINYGLTF_TYPE_MAT3:
			dataType = Xenon::Backend::AttributeDataType::Mat3;
			break;

		case TINYGLTF_TYPE_MAT4:
			dataType = Xenon::Backend::AttributeDataType::Mat4;
			break;

		case TINYGLTF_TYPE_SCALAR:
			dataType = Xenon::Backend::AttributeDataType::Scalar;
			break;

		case TINYGLTF_TYPE_VECTOR:
			dataType = Xenon::Backend::AttributeDataType::Vec3;
			break;

		case TINYGLTF_TYPE_MATRIX:
			dataType = Xenon::Backend::AttributeDataType::Mat4;
			break;

		default:
			XENON_LOG_ERROR("Invalid or unsupported vertex data type in the provided model file. Defaulting to vector 3.");
			dataType = Xenon::Backend::AttributeDataType::Vec3;
			break;
		}

		// Setup the component type.
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Uint8);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Int8);
			break;

		case TINYGLTF_COMPONENT_TYPE_SHORT:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Int16);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Uint16);
			break;

		case TINYGLTF_COMPONENT_TYPE_INT:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Int32);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Uint32);
			break;

		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Float);
			break;

		case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			specification.addElement(element, dataType, Xenon::Backend::ComponentDataType::Double);
			break;

		default:
			XENON_LOG_ERROR("Invalid or unsupported vertex element type in the provided model file.");
			break;
		}

		return accessor.ByteStride(bufferView) * accessor.count;
	}

	/**
	 * Attribute view structure.
	 */
	struct AttributeView final
	{
		using Iterator = std::vector<unsigned char>::const_iterator;

		Iterator m_Begin;
		Iterator m_End;

		uint64_t m_Size = 0;
		uint32_t m_Stride = 0;

		Xenon::Backend::InputElement m_Element = Xenon::Backend::InputElement::VertexPosition;
		Xenon::PrimitiveMode m_PrimitiveMode = Xenon::PrimitiveMode::Triangles;
	};

	/**
	 * Get an attribute's buffer view.
	 *
	 * @param model The model in which the data are stored.
	 * @param primitive The primitive to access.
	 * @param element The vertex element to get the view of.
	 * @return The tuple of iterators and sizes containing the begin and end iterators and the byte stride.
	 */
	decltype(auto) GetAttributeView(const tinygltf::Model& model, const tinygltf::Primitive& primitive, Xenon::Backend::InputElement element)
	{
		OPTICK_EVENT();

		AttributeView view;
		view.m_Element = element;

		if (!primitive.attributes.contains(g_Attributes[Xenon::EnumToInt(element)]))
			return view;

		const auto& accessor = model.accessors[primitive.attributes.at(g_Attributes[Xenon::EnumToInt(element)])];
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		const auto& buffer = model.buffers[bufferView.buffer];

		view.m_Stride = accessor.ByteStride(bufferView);
		view.m_Begin = buffer.data.begin() + accessor.byteOffset + bufferView.byteOffset;
		view.m_End = view.m_Begin + (accessor.count * view.m_Stride);

		view.m_Size = std::distance(view.m_Begin, view.m_End);

		// Setup the vertex mode.
		switch (primitive.mode)
		{
		case TINYGLTF_MODE_POINTS:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::Points;
			break;

		case TINYGLTF_MODE_LINE:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::Line;
			break;

		case TINYGLTF_MODE_LINE_LOOP:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::LineLoop;
			break;

		case TINYGLTF_MODE_LINE_STRIP:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::LineStrip;
			break;

		case TINYGLTF_MODE_TRIANGLES:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::Triangles;
			break;

		case TINYGLTF_MODE_TRIANGLE_STRIP:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::TriangleStrip;
			break;

		case TINYGLTF_MODE_TRIANGLE_FAN:
			view.m_PrimitiveMode = Xenon::PrimitiveMode::TriangleFan;
			break;

		default:
			XENON_LOG_ERROR("Invalid or unsupported vertex mode type in the provided model file. Defaulting to triangle.");
			view.m_PrimitiveMode = Xenon::PrimitiveMode::Triangles;
			break;
		}

		return view;
	}

	/**
	 * Load a sub-mesh from a primitive.
	 *
	 * @param subMesh The sub-mesh to load the data to.
	 * @param specification The vertex specification.
	 * @param model The glTF model.
	 * @param primitive The glTF primitive.
	 * @param vertexBegin The vertex begin iterator to load the data to.
	 * @param indexBegin The index begin iterator to load the data to.
	 */
	void LoadSubMesh(
		Xenon::SubMesh& subMesh,
		const Xenon::Backend::VertexSpecification& specification,
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		std::vector<unsigned char>::iterator vertexBegin,
		std::vector<unsigned char>::iterator indexBegin)
	{
		OPTICK_EVENT();

		// Setup the primitive mode.
		switch (primitive.mode)
		{
		case TINYGLTF_MODE_POINTS:
			subMesh.m_Mode = Xenon::PrimitiveMode::Points;
			break;

		case TINYGLTF_MODE_LINE:
			subMesh.m_Mode = Xenon::PrimitiveMode::Line;
			break;

		case TINYGLTF_MODE_LINE_LOOP:
			subMesh.m_Mode = Xenon::PrimitiveMode::LineLoop;
			break;

		case TINYGLTF_MODE_LINE_STRIP:
			subMesh.m_Mode = Xenon::PrimitiveMode::LineStrip;
			break;

		case TINYGLTF_MODE_TRIANGLES:
			subMesh.m_Mode = Xenon::PrimitiveMode::Triangles;
			break;

		case TINYGLTF_MODE_TRIANGLE_STRIP:
			subMesh.m_Mode = Xenon::PrimitiveMode::TriangleStrip;
			break;

		case TINYGLTF_MODE_TRIANGLE_FAN:
			subMesh.m_Mode = Xenon::PrimitiveMode::TriangleFan;
			break;

		default:
			XENON_LOG_ERROR("Invalid or unsupported vertex mode type in the provided model file. Defaulting to triangle.");
			subMesh.m_Mode = Xenon::PrimitiveMode::Triangles;
			break;
		}

		uint64_t vertexStride = 0;
		uint64_t vertexBufferSize = 0;

		// Get the vertex information.
		std::vector<AttributeView> attributes;
		for (auto i = Xenon::EnumToInt(Xenon::Backend::InputElement::VertexPosition); i < Xenon::EnumToInt(Xenon::Backend::InputElement::VertexElementCount); i++)
		{
			if (specification.isAvailable(static_cast<Xenon::Backend::InputElement>(i)))
			{
				const auto& view = attributes.emplace_back(GetAttributeView(model, primitive, static_cast<Xenon::Backend::InputElement>(i)));
				vertexStride += view.m_Stride;
				vertexBufferSize += view.m_Size;
			}
		}

		// Load the vertex data to the buffer.
		subMesh.m_VertexCount = vertexBufferSize / vertexStride;
		for (uint64_t i = 0; i < subMesh.m_VertexCount; i++)
		{
			for (auto& attribute : attributes)
			{
				if (attribute.m_Begin != attribute.m_End)
				{
					std::copy_n(attribute.m_Begin, attribute.m_Stride, vertexBegin);
					attribute.m_Begin += attribute.m_Stride;
				}

				vertexBegin += attribute.m_Stride;
			}
		}

		// Load the index buffer data.
		if (primitive.indices >= 0)
		{
			const auto& accessor = model.accessors.at(primitive.indices);
			const auto& bufferView = model.bufferViews.at(accessor.bufferView);
			const auto& buffer = model.buffers.at(bufferView.buffer);

			const auto stride = accessor.ByteStride(bufferView);
			const auto start = accessor.byteOffset + bufferView.byteOffset;
			const auto end = start + accessor.count * stride;

			subMesh.m_IndexCount = accessor.count;
			subMesh.m_IndexSize = static_cast<uint8_t>(stride);

			if (subMesh.m_IndexOffset > 0)
				subMesh.m_IndexOffset /= subMesh.m_IndexSize;

			std::copy(buffer.data.begin() + start, buffer.data.begin() + end, indexBegin);
		}
	}

	/**
	 * Get the image specification.
	 *
	 * @param sampler The sampler.
	 * @return The image sampler specification.
	 */
	XENON_NODISCARD Xenon::Backend::ImageSamplerSpecification GetImageSamplerSpecification(const tinygltf::Sampler& sampler) noexcept
	{
		OPTICK_EVENT();

		Xenon::Backend::ImageSamplerSpecification specification;

		switch (sampler.minFilter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			specification.m_ImageMinificationFilter = Xenon::Backend::ImageFilter::Nearest;
			break;

		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			specification.m_ImageMinificationFilter = Xenon::Backend::ImageFilter::Linear;
			break;

		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			specification.m_MipMapMode = Xenon::Backend::ImageMipMapMode::Nearest;
			break;

		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			specification.m_MipMapMode = Xenon::Backend::ImageMipMapMode::Linear;
			break;

		default:
			break;
		}

		switch (sampler.magFilter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			specification.m_ImageMagificationFilter = Xenon::Backend::ImageFilter::Nearest;
			break;

		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			specification.m_ImageMagificationFilter = Xenon::Backend::ImageFilter::Linear;
			break;

		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			specification.m_MipMapMode = Xenon::Backend::ImageMipMapMode::Nearest;
			break;

		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			specification.m_MipMapMode = Xenon::Backend::ImageMipMapMode::Linear;
			break;

		default:
			break;
		}

		// TINYGLTF_TEXTURE_WRAP_REPEAT 
		// TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE 
		// TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT 

		return specification;
	}

	/**
	 * Create a new texture.
	 *
	 * @tparam Type The texture info type.
	 * @param geometry The geometry to get the images from.
	 * @param model The model to get the images from.
	 * @param info The texture info structure.
	 * @return The created texture structure.
	 */
	template<TextureInfo Type>
	XENON_NODISCARD Xenon::Texture CreateTexture(Xenon::Instance& instance, const Xenon::Geometry& geometry, const tinygltf::Model& model, const Type& info)
	{
		OPTICK_EVENT();

		Xenon::Texture xTexture = {};

		if (info.index < 0)
		{
			xTexture.m_pImage = instance.getDefaultImage();
			xTexture.m_pImageView = instance.getDefaultImageView();
			xTexture.m_pImageSampler = instance.getDefaultImageSampler();
		}
		else
		{
			const auto& texture = model.textures[info.index];

			const auto& [pImage, pView] = geometry.getImageAndImageViews()[texture.source];
			const auto& pSampler = geometry.getImageSamplers()[texture.sampler];

			xTexture.m_pImage = pImage.get();
			xTexture.m_pImageView = pView.get();
			xTexture.m_pImageSampler = pSampler.get();
		}

		return xTexture;
	}

	/**
	 * Load a node from the model.
	 *
	 * @param instance The instance reference.
	 * @param model The model to load from.
	 * @param node The node to load.
	 * @param geometry The mesh geometry.
	 * @param vertices The vertex storage.
	 * @param vertexItr The vertex storage iterator.
	 * @param indices The index storage.
	 * @param indexItr The index storage iterator.
	 * @param synchronization The synchronization latch.
	 */
	void LoadNode(
		Xenon::Instance& instance,
		const tinygltf::Model& model,
		const tinygltf::Node& node,
		Xenon::Geometry& geometry,
		std::vector<unsigned char>& vertices,
		std::vector<unsigned char>::iterator& vertexItr,
		std::vector<unsigned char>& indices,
		std::vector<unsigned char>::iterator& indexItr,
		std::latch& synchronization)
	{
		OPTICK_EVENT();

		// If it's an invalid index, return.
		if (node.mesh == -1)
			return;

		// Get the mesh and initialize everything.
		const auto& gltfMesh = model.meshes[node.mesh];
		auto& mesh = geometry.getMeshes().emplace_back();
		mesh.m_Name = gltfMesh.name;
		mesh.m_SubMeshes.reserve(gltfMesh.primitives.size());

		// Load the sub-mesh information.
		for (const auto& gltfPrimitive : gltfMesh.primitives)
		{
			// Create the primitive.
			auto& subMesh = mesh.m_SubMeshes.emplace_back();
			subMesh.m_VertexOffset = std::distance(vertices.begin(), vertexItr);
			subMesh.m_IndexOffset = std::distance(indices.begin(), indexItr);

			if (subMesh.m_VertexOffset > 0)
				subMesh.m_VertexOffset /= geometry.getVertexSpecification().getSize();

			// Setup the sub-mesh loader. This is done so VS won't fuck up the formatting smh...
			const auto subMeshLoader = [&subMesh, &model, &geometry, &gltfPrimitive, vertexItr, indexItr, &synchronization]
			{
				OPTICK_EVENT_DYNAMIC("Loading Sub-Mesh Data");
				LoadSubMesh(subMesh, geometry.getVertexSpecification(), model, gltfPrimitive, vertexItr, indexItr);
				synchronization.count_down();
			};

			// Insert the job.
			Xenon::XObject::GetJobSystem().insert(subMeshLoader);

			// Get the next available vertex begin position.
			for (auto i = Xenon::EnumToInt(Xenon::Backend::InputElement::VertexPosition); i < Xenon::EnumToInt(Xenon::Backend::InputElement::VertexElementCount); i++)
			{
				if (geometry.getVertexSpecification().isAvailable(static_cast<Xenon::Backend::InputElement>(i)) && gltfPrimitive.attributes.contains(g_Attributes[i]))
				{
					const auto& accessor = model.accessors[gltfPrimitive.attributes.at(g_Attributes[i])];
					const auto& bufferView = model.bufferViews[accessor.bufferView];

					vertexItr += accessor.ByteStride(bufferView) * accessor.count;
				}
			}

			// Get the next available index begin position.
			if (gltfPrimitive.indices >= 0)
			{
				const auto& accessor = model.accessors.at(gltfPrimitive.indices);
				const auto& bufferView = model.bufferViews.at(accessor.bufferView);

				indexItr += accessor.ByteStride(bufferView) * accessor.count;
			}

			// Setup the textures.
			const auto& material = model.materials[gltfPrimitive.material];
			subMesh.m_BaseColorTexture = CreateTexture(instance, geometry, model, material.pbrMetallicRoughness.baseColorTexture);
			subMesh.m_RoughnessTexture = CreateTexture(instance, geometry, model, material.pbrMetallicRoughness.metallicRoughnessTexture);
			subMesh.m_NormalTexture = CreateTexture(instance, geometry, model, material.normalTexture);
			subMesh.m_OcclusionTexture = CreateTexture(instance, geometry, model, material.occlusionTexture);
			subMesh.m_EmissiveTexture = CreateTexture(instance, geometry, model, material.emissiveTexture);
		}

		// // Load the children.
		// for (const auto child : node.children)
		// 	LoadNode(instance, model, model.nodes[child], geometry, vertices, vertexItr, indices, indexItr, synchronization);
	}

	/**
	 * Get the data format from the bits and component count.
	 *
	 * @param bits The bits of the pixel.
	 * @param components The component count.
	 * @param pixelType The pixel type.
	 * @return The format.
	 */
	XENON_NODISCARD constexpr Xenon::Backend::DataFormat GetDataFormat(int bits, int components, int pixelType) noexcept
	{
		// For now just don't do anything. We got more things to worry about than this :P
		switch (pixelType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			break;

		case TINYGLTF_COMPONENT_TYPE_SHORT:
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			break;

		case TINYGLTF_COMPONENT_TYPE_INT:
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			break;

		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			break;

		case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			break;

		default:
			break;
		}

		return Xenon::Backend::DataFormat::R8G8B8A8_SRGB;
	}
}

namespace Xenon
{
	Xenon::Geometry Geometry::FromFile(Instance& instance, const std::filesystem::path& file)
	{
		OPTICK_EVENT();

		Geometry geometry;

		// Load the model data.
		tinygltf::Model model;
		std::string errorString;
		std::string warningString;

		// Try and load the model.
		if (tinygltf::TinyGLTF loader; !loader.LoadASCIIFromFile(&model, &errorString, &warningString, file.string()))
		{
			// Show the error if there are any.
			if (!errorString.empty())
				XENON_LOG_ERROR("glTF loading error: {}", errorString);

			// Show the warning if there are any.
			if (!warningString.empty())
				XENON_LOG_WARNING("glTF loading warning: {}", warningString);

			return geometry;
		}

		// Show the warning if there are any.
		if (!warningString.empty())
			XENON_LOG_WARNING("glTF loading warning: {}", warningString);

		// Resolve the vertex specification and other information.
		uint64_t vertexBufferSize = 0;
		uint64_t indexBufferSize = 0;

		uint32_t workerSubmissions = 0;
		for (const auto& mesh : model.meshes)
		{
			workerSubmissions += mesh.primitives.size();
			for (const auto& primitive : mesh.primitives)
			{
				// Get the vertex information.
				for (auto i = EnumToInt(Backend::InputElement::VertexPosition); i < EnumToInt(Backend::InputElement::VertexElementCount); i++)
					vertexBufferSize += ResolvePrimitive(model, primitive, g_Attributes[i], static_cast<Backend::InputElement>(i), geometry.m_VertexSpecification);

				// Get the index buffer size.
				if (primitive.indices >= 0)
				{
					const auto& accessor = model.accessors.at(primitive.indices);
					const auto& bufferView = model.bufferViews.at(accessor.bufferView);

					indexBufferSize += accessor.count * accessor.ByteStride(bufferView);
				}
			}
		}

		// Check if we have data to create the vertex buffers.
		if (vertexBufferSize == 0)
		{
			XENON_LOG_ERROR("The submitted model file '{}' does not have any vertex data to load!", file.string());
			return geometry;
		}

		// Check if we have data to create the index buffers.
		if (indexBufferSize == 0)
		{
			XENON_LOG_WARNING("The submitted model file '{}' does not have any index data to load!", file.string());
		}

		// Setup the image synchronization primitive.
		auto imageSynchronization = CountingFence(model.images.size());

		// Setup the images.
		geometry.m_pImageAndImageViews.reserve(model.images.size());
		for (const auto& image : model.images)
		{
			const auto imageLoader = [&instance, entry = &geometry.m_pImageAndImageViews.emplace_back(), &image, &imageSynchronization]
			{
				// Setup the image.
				Xenon::Backend::ImageSpecification imageSpecification = {};
				imageSpecification.m_Width = image.width;
				imageSpecification.m_Height = image.height;
				imageSpecification.m_Format = GetDataFormat(image.bits, image.component, image.pixel_type);
				entry->first = instance.getFactory()->createImage(instance.getBackendDevice(), imageSpecification);

				// Copy the image data to the image.
				{
					const auto copySize = entry->first->getWidth() * entry->first->getHeight() * image.component/* * (image.bits / 8)*/;
					auto pStagingBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), copySize, Xenon::Backend::BufferType::Staging);

					pStagingBuffer->write(Xenon::ToBytes(image.image.data()), image.image.size());
					entry->first->copyFrom(pStagingBuffer.get());
				}

				// Setup image view.
				entry->second = instance.getFactory()->createImageView(instance.getBackendDevice(), entry->first.get(), {});

				// Notify that we're done.
				imageSynchronization.arrive();
			};

			XObject::GetJobSystem().insert(imageLoader);
		}

		// Setup the samplers.
		geometry.m_pImageSamplers.reserve(model.samplers.size());
		for (const auto& sampler : model.samplers)
			geometry.m_pImageSamplers.emplace_back(instance.getFactory()->createImageSampler(instance.getBackendDevice(), GetImageSamplerSpecification(sampler)));

		// Setup animations.
		// for (const auto& animation : model.animations)
		// {
		// }

		// Wait till all the images are loaded before we proceed.
		imageSynchronization.wait();

		// Load the mesh information.
		auto vertices = std::vector<unsigned char>(vertexBufferSize);
		auto vertexItr = vertices.begin();

		auto indices = std::vector<unsigned char>(indexBufferSize);
		auto indexItr = indices.begin();

		// Load the default scene.
		geometry.m_Meshes.reserve(model.meshes.size());
		auto synchronization = std::latch(workerSubmissions);

		// Load the nodes.
		for (const auto& node : model.nodes)
			LoadNode(instance, model, node, geometry, vertices, vertexItr, indices, indexItr, synchronization);

		// Wait till all the sub-meshes are loaded.
		synchronization.wait();

		// Load the vertex data and clear the buffer.
		geometry.m_pVertexBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), vertexBufferSize, Backend::BufferType::Vertex);
		geometry.m_pVertexBuffer->write(ToBytes(vertices.data()), vertexBufferSize);

		// Load the index data and clear the buffer.
		if (indexBufferSize > 0)
		{
			geometry.m_pIndexBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), indexBufferSize, Backend::BufferType::Index);
			geometry.m_pIndexBuffer->write(ToBytes(indices.data()), indexBufferSize);
		}

		// Generate LOD tree.
		for (const auto& mesh : geometry.m_Meshes)
		{
			for (const auto& subMesh : mesh.m_SubMeshes)
			{
				const auto begin = indices.data() + subMesh.m_IndexOffset;
				const auto end = begin + (subMesh.m_IndexCount * subMesh.m_IndexSize);

				if (subMesh.m_IndexSize == sizeof(uint16_t))
				{
					const auto tree = LODTree(XENON_BIT_CAST(uint16_t*, begin), XENON_BIT_CAST(uint16_t*, end));
				}
				else if (subMesh.m_IndexSize == sizeof(uint32_t))
				{
					const auto tree = LODTree(XENON_BIT_CAST(uint32_t*, begin), XENON_BIT_CAST(uint32_t*, end));
				}
			}
		}

		vertices.clear();
		indices.clear();

		return geometry;
	}

	Xenon::Geometry Geometry::CreateQuad(Instance& instance)
	{
		/**
		 * Vertex structure.
		 */
		struct Vertex final
		{
			glm::vec2 m_Position;
			glm::vec2 m_UV;
		};

		// Define the geometry for a triangle.
		const std::array<Vertex, 4> triangleVertices =
		{
			Vertex{.m_Position = glm::vec2(1.0f, 1.0f), .m_UV = glm::vec2(0.0f, 0.0f)},
			Vertex{.m_Position = glm::vec2(1.0f, -1.0f), .m_UV = glm::vec2(0.0f, 1.0f)},
			Vertex{.m_Position = glm::vec2(-1.0f, 1.0f), .m_UV = glm::vec2(1.0f, 0.0f)},
			Vertex{.m_Position = glm::vec2(-1.0f, -1.0f), .m_UV = glm::vec2(1.0f, 1.0f)}
		};

		constexpr std::array<uint16_t, 6> triangleIndices =
		{
			0, 1, 2,
			2, 3, 1
		};

		// Setup the geometry.
		Geometry geometry;
		geometry.m_VertexSpecification.addElement(Backend::InputElement::VertexPosition, Backend::AttributeDataType::Vec2, Backend::ComponentDataType::Float);
		geometry.m_VertexSpecification.addElement(Backend::InputElement::VertexTextureCoordinate_0, Backend::AttributeDataType::Vec2, Backend::ComponentDataType::Float);

		// Setup the sub-mesh.
		auto& subMesh = geometry.m_Meshes.emplace_back().m_SubMeshes.emplace_back();
		subMesh.m_VertexCount = 3;
		subMesh.m_IndexCount = 6;
		subMesh.m_IndexSize = sizeof(uint16_t);

		// Load the vertex data.
		constexpr auto vertexBufferSize = triangleVertices.size() * sizeof(Vertex);
		geometry.m_pVertexBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), vertexBufferSize, Backend::BufferType::Vertex);
		geometry.m_pVertexBuffer->write(ToBytes(triangleVertices.data()), vertexBufferSize);

		// Load the index data.
		constexpr auto indexBufferSize = triangleIndices.size() * sizeof(uint16_t);
		geometry.m_pIndexBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), indexBufferSize, Backend::BufferType::Index);
		geometry.m_pIndexBuffer->write(ToBytes(triangleIndices.data()), indexBufferSize);

		return geometry;
	}

	std::unique_ptr<Xenon::Backend::Image> Geometry::CreateImageFromFile(Instance& instance, const std::filesystem::path& file)
	{
		constexpr uint8_t bits = 8;

		int width = 0;
		int height = 0;
		int components = 0;
		const auto pPixels = stbi_load(file.string().c_str(), &width, &height, &components, STBI_rgb_alpha);

		if (width == 0 || height == 0 || pPixels == nullptr)
			return nullptr;

		// Setup the image.
		Xenon::Backend::ImageSpecification imageSpecification = {};
		imageSpecification.m_Width = width;
		imageSpecification.m_Height = height;
		imageSpecification.m_Format = GetDataFormat(bits, components, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
		auto pImage = instance.getFactory()->createImage(instance.getBackendDevice(), imageSpecification);

		const auto copySize = pImage->getWidth() * pImage->getHeight() * components/* * (bits / 8)*/;
		auto pStagingBuffer = instance.getFactory()->createBuffer(instance.getBackendDevice(), copySize, Xenon::Backend::BufferType::Staging);

		pStagingBuffer->write(Xenon::ToBytes(pPixels), copySize);
		pImage->copyFrom(pStagingBuffer.get());

		STBI_FREE(pPixels);
		return pImage;
	}
}
