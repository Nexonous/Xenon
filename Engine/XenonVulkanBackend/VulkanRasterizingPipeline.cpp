// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "VulkanRasterizingPipeline.hpp"
#include "VulkanMacros.hpp"
#include "VulkanRasterizer.hpp"
#include "VulkanDescriptorSetManager.hpp"
#include "VulkanDescriptor.hpp"

#include <optick.h>

#include <algorithm>

#ifdef XENON_PLATFORM_WINDOWS
#include <execution>

#endif

// This magic number is used by the rasterizing pipeline to uniquely identify it's pipeline caches.
constexpr uint64_t g_MagicNumber = 0b0111110011100110101100111010010010001011111101111110001010110001;

namespace /* anonymous */
{
	/**
	 * Get the shader bindings.
	 *
	 * @param shader The shader to get the bindings from.
	 * @param bindingMap The binding map contains set-binding info.
	 * @param pushConstants The push constants vector to load the data.
	 * @param inputDescriptions The input descriptions vector. It will only add data to the vector if we're in the vertex shader.
	 * @param inputAttributeDescriptions The input attributes vector. It will only add data to the vector if we're in the vertex shader.
	 * @param type The shader type.
	 */
	void GetShaderBindings(
		const Xenon::Backend::Shader& shader,
		std::unordered_map<Xenon::Backend::DescriptorType, std::unordered_map<uint32_t, Xenon::Backend::DescriptorBindingInfo>>& bindingMap,
		std::vector<VkPushConstantRange>& pushConstants,
		std::vector<VkVertexInputBindingDescription>& inputBindingDescriptions,
		std::vector<VkVertexInputAttributeDescription>& inputAttributeDescriptions,
		Xenon::Backend::ShaderType type)
	{
		// Get the resources.
		for (const auto& resource : shader.getResources())
		{
			auto& bindings = bindingMap[static_cast<Xenon::Backend::DescriptorType>(Xenon::EnumToInt(resource.m_Set))];

			if (bindings.contains(resource.m_Binding))
			{
				bindings[resource.m_Binding].m_ApplicableShaders |= type;
			}
			else
			{
				auto& binding = bindings[resource.m_Binding];
				binding.m_Type = resource.m_Type;
				binding.m_ApplicableShaders = type;
			}
		}

		// Get the buffers.
		// for (const auto& buffer : shader.getConstantBuffers())
		// {
		// 	auto& range = pushConstants.emplace_back();
		// 	range.offset = buffer.m_Offset;
		// 	range.size = buffer.m_Size;
		// 	range.stageFlags = Xenon::Backend::VulkanDevice::GetShaderStageFlagBit(type);
		// }

		// Setup the input bindings if we're on the vertex shader.
		if (type & Xenon::Backend::ShaderType::Vertex)
		{
			bool hasInstanceInputs = false;
			for (const auto& input : shader.getInputAttributes())
			{
				const auto element = static_cast<Xenon::Backend::InputElement>(input.m_Location);
				auto& attribute = inputAttributeDescriptions.emplace_back();
				attribute.binding = IsVertexElement(element) ? 0 : 1;
				attribute.location = input.m_Location;

				hasInstanceInputs |= attribute.binding == 1;

				switch (static_cast<Xenon::Backend::InputElement>(input.m_Location))
				{
				case Xenon::Backend::InputElement::InstancePosition:
					attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
					attribute.offset = offsetof(Xenon::Backend::InstanceEntry, m_Position);
					break;

				case Xenon::Backend::InputElement::InstanceRotation:
					attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
					attribute.offset = offsetof(Xenon::Backend::InstanceEntry, m_Rotation);
					break;

				case Xenon::Backend::InputElement::InstanceScale:
					attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
					attribute.offset = offsetof(Xenon::Backend::InstanceEntry, m_Scale);
					break;

				case Xenon::Backend::InputElement::InstanceID:
					attribute.format = VK_FORMAT_R32_UINT;
					attribute.offset = offsetof(Xenon::Backend::InstanceEntry, m_InstanceID);
					break;

				default:
					break;
				}
			}

			// Setup the binding if we have instance inputs in the shader.
			if (hasInstanceInputs)
			{
				auto& binding = inputBindingDescriptions.emplace_back();
				binding.binding = 1;
				binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
				binding.stride = sizeof(Xenon::Backend::InstanceEntry);
			}
		}
	}

	/**
	 * Get the primitive topology.
	 *
	 * @param topology The flint primitive topology.
	 * @return The Vulkan primitive topology.
	 */
	XENON_NODISCARD constexpr VkPrimitiveTopology GetPrimitiveTopology(Xenon::Backend::PrimitiveTopology topology) noexcept
	{
		switch (topology)
		{
		case Xenon::Backend::PrimitiveTopology::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		case Xenon::Backend::PrimitiveTopology::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		case Xenon::Backend::PrimitiveTopology::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

		case Xenon::Backend::PrimitiveTopology::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		case Xenon::Backend::PrimitiveTopology::TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		case Xenon::Backend::PrimitiveTopology::TriangleFan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

		case Xenon::Backend::PrimitiveTopology::LineListWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;

		case Xenon::Backend::PrimitiveTopology::LineStripWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;

		case Xenon::Backend::PrimitiveTopology::TriangleListWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;

		case Xenon::Backend::PrimitiveTopology::TriangleStripWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;

		case Xenon::Backend::PrimitiveTopology::PatchList:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

		default:
			XENON_LOG_ERROR("Invalid or Undefined primitive topology! Defaulting to PointList.");
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		}
	}

	/**
	 * Get the cull mode.
	 *
	 * @param cull The flint cull mode.
	 * @return The Vulkan cull mode.
	 */
	XENON_NODISCARD constexpr VkCullModeFlags GetCullMode(Xenon::Backend::CullMode cull) noexcept
	{
		switch (cull)
		{
		case Xenon::Backend::CullMode::None:
			return VK_CULL_MODE_NONE;

		case Xenon::Backend::CullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;

		case Xenon::Backend::CullMode::Back:
			return VK_CULL_MODE_BACK_BIT;

		case Xenon::Backend::CullMode::FrontAndBack:
			return VK_CULL_MODE_FRONT_AND_BACK;

		default:
			XENON_LOG_ERROR("Invalid or Undefined cull mode! Defaulting to None.");
			return VK_CULL_MODE_NONE;
		}
	}

	/**
	 * Get the front face.
	 *
	 * @param face The flint front face.
	 * @return The Vulkan front face.
	 */
	XENON_NODISCARD constexpr VkFrontFace GetFrontFace(Xenon::Backend::FrontFace face) noexcept
	{
		switch (face)
		{
		case Xenon::Backend::FrontFace::CounterClockwise:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;

		case Xenon::Backend::FrontFace::Clockwise:
			return VK_FRONT_FACE_CLOCKWISE;

		default:
			XENON_LOG_ERROR("Invalid or Undefined front face! Defaulting to CounterClockwise.");
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

	/**
	 * Get the polygon mode.
	 *
	 * @param mode The flint polygon mode.
	 * @return The Vulkan polygon mode.
	 */
	XENON_NODISCARD constexpr VkPolygonMode GetPolygonMode(Xenon::Backend::PolygonMode mode) noexcept
	{
		switch (mode)
		{
		case Xenon::Backend::PolygonMode::Fill:
			return VK_POLYGON_MODE_FILL;

		case Xenon::Backend::PolygonMode::Line:
			return VK_POLYGON_MODE_LINE;

		case Xenon::Backend::PolygonMode::Point:
			return VK_POLYGON_MODE_POINT;

		default:
			XENON_LOG_ERROR("Invalid or Undefined polygon mode! Defaulting to Fill.");
			return VK_POLYGON_MODE_FILL;
		}
	}

	/**
	 * Get the logic operator.
	 *
	 * @param logic The flint logic.
	 * @return The Vulkan logic.
	 */
	XENON_NODISCARD constexpr VkLogicOp GetLogicOp(Xenon::Backend::ColorBlendLogic logic) noexcept
	{
		switch (logic)
		{
		case Xenon::Backend::ColorBlendLogic::Clear:
			return VK_LOGIC_OP_CLEAR;

		case Xenon::Backend::ColorBlendLogic::And:
			return VK_LOGIC_OP_AND;

		case Xenon::Backend::ColorBlendLogic::AndReverse:
			return VK_LOGIC_OP_AND_REVERSE;

		case Xenon::Backend::ColorBlendLogic::Copy:
			return VK_LOGIC_OP_COPY;

		case Xenon::Backend::ColorBlendLogic::AndInverted:
			return VK_LOGIC_OP_AND_INVERTED;

		case Xenon::Backend::ColorBlendLogic::NoOperator:
			return VK_LOGIC_OP_NO_OP;

		case Xenon::Backend::ColorBlendLogic::XOR:
			return VK_LOGIC_OP_XOR;

		case Xenon::Backend::ColorBlendLogic::OR:
			return VK_LOGIC_OP_OR;

		case Xenon::Backend::ColorBlendLogic::NOR:
			return VK_LOGIC_OP_NOR;

		case Xenon::Backend::ColorBlendLogic::Equivalent:
			return VK_LOGIC_OP_EQUIVALENT;

		case Xenon::Backend::ColorBlendLogic::Invert:
			return VK_LOGIC_OP_INVERT;

		case Xenon::Backend::ColorBlendLogic::ReverseOR:
			return VK_LOGIC_OP_OR_REVERSE;

		case Xenon::Backend::ColorBlendLogic::CopyInverted:
			return VK_LOGIC_OP_COPY_INVERTED;

		case Xenon::Backend::ColorBlendLogic::InvertedOR:
			return VK_LOGIC_OP_OR_INVERTED;

		case Xenon::Backend::ColorBlendLogic::NAND:
			return VK_LOGIC_OP_NAND;

		case Xenon::Backend::ColorBlendLogic::Set:
			return VK_LOGIC_OP_SET;

		default:
			XENON_LOG_ERROR("Invalid or Undefined color blend logic! Defaulting to Clear.");
			return VK_LOGIC_OP_CLEAR;
		}
	}

	/**
	 * Get the compare operator.
	 *
	 * @param logic the flint logic.
	 * @return The Vulkan logic operator.
	 */
	XENON_NODISCARD constexpr VkCompareOp GetCompareOp(Xenon::Backend::DepthCompareLogic logic) noexcept
	{
		switch (logic)
		{
		case Xenon::Backend::DepthCompareLogic::Never:
			return VK_COMPARE_OP_NEVER;

		case Xenon::Backend::DepthCompareLogic::Less:
			return VK_COMPARE_OP_LESS;

		case Xenon::Backend::DepthCompareLogic::Equal:
			return VK_COMPARE_OP_EQUAL;

		case Xenon::Backend::DepthCompareLogic::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;

		case Xenon::Backend::DepthCompareLogic::Greater:
			return VK_COMPARE_OP_GREATER;

		case Xenon::Backend::DepthCompareLogic::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;

		case Xenon::Backend::DepthCompareLogic::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;

		case Xenon::Backend::DepthCompareLogic::Always:
			return VK_COMPARE_OP_ALWAYS;

		default:
			XENON_LOG_ERROR("Invalid or Undefined depth compare logic! Defaulting to Never.");
			return VK_COMPARE_OP_NEVER;
		}
	}

#ifdef XENON_FEATURE_CONSTEXPR_VECTOR
	/**
	 * Get the dynamic states.
	 *
	 * @param flags The flint flags.
	 * @return The Vulkan flags.
	 */
	XENON_NODISCARD constexpr std::vector<VkDynamicState> GetDynamicStates(Xenon::Backend::DynamicStateFlags flags) noexcept
	{
		std::vector<VkDynamicState> states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		if (flags & Xenon::Backend::DynamicStateFlags::LineWidth) states.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		if (flags & Xenon::Backend::DynamicStateFlags::DepthBias) states.emplace_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		if (flags & Xenon::Backend::DynamicStateFlags::BlendConstants) states.emplace_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		if (flags & Xenon::Backend::DynamicStateFlags::DepthBounds) states.emplace_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);

		return states;
	}

#else
	/**
	 * Get the dynamic states.
	 *
	 * @param flags The flint flags.
	 * @return The Vulkan flags.
	 */
	XENON_NODISCARD std::vector<VkDynamicState> GetDynamicStates(Xenon::Backend::DynamicStateFlags flags) noexcept
	{
		std::vector<VkDynamicState> states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		if (flags & Xenon::Backend::DynamicStateFlags::LineWidth) states.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		if (flags & Xenon::Backend::DynamicStateFlags::DepthBias) states.emplace_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		if (flags & Xenon::Backend::DynamicStateFlags::BlendConstants) states.emplace_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		if (flags & Xenon::Backend::DynamicStateFlags::DepthBounds) states.emplace_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);

		return states;
	}

#endif // XENON_FEATURE_CONSTEXPR_VECTOR

	/**
	 * Get the blend factor.
	 *
	 * @param factor The flint factor.
	 * @return The Vulkan factor.
	 */
	XENON_NODISCARD constexpr VkBlendFactor GetBlendFactor(Xenon::Backend::ColorBlendFactor factor) noexcept
	{
		switch (factor)
		{
		case Xenon::Backend::ColorBlendFactor::Zero:
			return VK_BLEND_FACTOR_ZERO;

		case Xenon::Backend::ColorBlendFactor::One:
			return VK_BLEND_FACTOR_ONE;

		case Xenon::Backend::ColorBlendFactor::SourceColor:
			return VK_BLEND_FACTOR_SRC_COLOR;

		case Xenon::Backend::ColorBlendFactor::OneMinusSourceColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

		case Xenon::Backend::ColorBlendFactor::DestinationColor:
			return VK_BLEND_FACTOR_DST_COLOR;

		case Xenon::Backend::ColorBlendFactor::OneMinusDestinationColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

		case Xenon::Backend::ColorBlendFactor::SourceAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;

		case Xenon::Backend::ColorBlendFactor::OneMinusSourceAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		case Xenon::Backend::ColorBlendFactor::DestinationAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;

		case Xenon::Backend::ColorBlendFactor::OneMinusDestinationAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

		case Xenon::Backend::ColorBlendFactor::ConstantColor:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;

		case Xenon::Backend::ColorBlendFactor::OneMinusConstantColor:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

		case Xenon::Backend::ColorBlendFactor::ConstantAlpha:
			return VK_BLEND_FACTOR_CONSTANT_ALPHA;

		case Xenon::Backend::ColorBlendFactor::OneMinusConstantAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;

		case Xenon::Backend::ColorBlendFactor::SourceAlphaSaturate:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

		case Xenon::Backend::ColorBlendFactor::SourceOneColor:
			return VK_BLEND_FACTOR_SRC1_COLOR;

		case Xenon::Backend::ColorBlendFactor::OneMinusSourceOneColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

		case Xenon::Backend::ColorBlendFactor::SourceOneAlpha:
			return VK_BLEND_FACTOR_SRC1_ALPHA;

		case Xenon::Backend::ColorBlendFactor::OneMinusSourceOneAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

		default:
			XENON_LOG_ERROR("Invalid color blend factor! Defaulting to Zero.");
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	/**
	 * Get the blend operator.
	 *
	 * @param op The flint operator.
	 * @return The Vulkan blend operator.
	 */
	XENON_NODISCARD constexpr VkBlendOp GetBlendOp(Xenon::Backend::ColorBlendOperator op) noexcept
	{
		switch (op)
		{
		case Xenon::Backend::ColorBlendOperator::Add:
			return VK_BLEND_OP_ADD;

		case Xenon::Backend::ColorBlendOperator::Subtract:
			return VK_BLEND_OP_SUBTRACT;

		case Xenon::Backend::ColorBlendOperator::ReverseSubtract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;

		case Xenon::Backend::ColorBlendOperator::Minimum:
			return VK_BLEND_OP_MIN;

		case Xenon::Backend::ColorBlendOperator::Maximum:
			return VK_BLEND_OP_MAX;

		case Xenon::Backend::ColorBlendOperator::Zero:
			return VK_BLEND_OP_ZERO_EXT;

		case Xenon::Backend::ColorBlendOperator::Source:
			return VK_BLEND_OP_SRC_EXT;

		case Xenon::Backend::ColorBlendOperator::Destination:
			return VK_BLEND_OP_DST_EXT;

		case Xenon::Backend::ColorBlendOperator::SourceOver:
			return VK_BLEND_OP_SRC_OVER_EXT;

		case Xenon::Backend::ColorBlendOperator::DestinationOver:
			return VK_BLEND_OP_DST_OVER_EXT;

		case Xenon::Backend::ColorBlendOperator::SourceIn:
			return VK_BLEND_OP_SRC_IN_EXT;

		case Xenon::Backend::ColorBlendOperator::DestinationIn:
			return VK_BLEND_OP_DST_IN_EXT;

		case Xenon::Backend::ColorBlendOperator::SouceOut:
			return VK_BLEND_OP_SRC_OUT_EXT;

		case Xenon::Backend::ColorBlendOperator::DestinationOut:
			return VK_BLEND_OP_DST_OUT_EXT;

		case Xenon::Backend::ColorBlendOperator::SourceATOP:
			return VK_BLEND_OP_SRC_ATOP_EXT;

		case Xenon::Backend::ColorBlendOperator::DestinationATOP:
			return VK_BLEND_OP_DST_ATOP_EXT;

		case Xenon::Backend::ColorBlendOperator::XOR:
			return VK_BLEND_OP_XOR_EXT;

		case Xenon::Backend::ColorBlendOperator::Multiply:
			return VK_BLEND_OP_MULTIPLY_EXT;

		case Xenon::Backend::ColorBlendOperator::Screen:
			return VK_BLEND_OP_SCREEN_EXT;

		case Xenon::Backend::ColorBlendOperator::Overlay:
			return VK_BLEND_OP_OVERLAY_EXT;

		case Xenon::Backend::ColorBlendOperator::Darken:
			return VK_BLEND_OP_DARKEN_EXT;

		case Xenon::Backend::ColorBlendOperator::Lighten:
			return VK_BLEND_OP_LIGHTEN_EXT;

		case Xenon::Backend::ColorBlendOperator::ColorDodge:
			return VK_BLEND_OP_COLORDODGE_EXT;

		case Xenon::Backend::ColorBlendOperator::ColorBurn:
			return VK_BLEND_OP_COLORBURN_EXT;

		case Xenon::Backend::ColorBlendOperator::HardLight:
			return VK_BLEND_OP_HARDLIGHT_EXT;

		case Xenon::Backend::ColorBlendOperator::SoftLight:
			return VK_BLEND_OP_SOFTLIGHT_EXT;

		case Xenon::Backend::ColorBlendOperator::Difference:
			return VK_BLEND_OP_DIFFERENCE_EXT;

		case Xenon::Backend::ColorBlendOperator::Exclusion:
			return VK_BLEND_OP_EXCLUSION_EXT;

		case Xenon::Backend::ColorBlendOperator::Invert:
			return VK_BLEND_OP_INVERT_EXT;

		case Xenon::Backend::ColorBlendOperator::InvertRGB:
			return VK_BLEND_OP_INVERT_RGB_EXT;

		case Xenon::Backend::ColorBlendOperator::LinearDodge:
			return VK_BLEND_OP_LINEARDODGE_EXT;

		case Xenon::Backend::ColorBlendOperator::LinearBurn:
			return VK_BLEND_OP_LINEARBURN_EXT;

		case Xenon::Backend::ColorBlendOperator::VividLight:
			return VK_BLEND_OP_VIVIDLIGHT_EXT;

		case Xenon::Backend::ColorBlendOperator::LinearLight:
			return VK_BLEND_OP_LINEARLIGHT_EXT;

		case Xenon::Backend::ColorBlendOperator::PinLight:
			return VK_BLEND_OP_PINLIGHT_EXT;

		case Xenon::Backend::ColorBlendOperator::HardMix:
			return VK_BLEND_OP_HARDMIX_EXT;

		case Xenon::Backend::ColorBlendOperator::HSLHue:
			return VK_BLEND_OP_HSL_HUE_EXT;

		case Xenon::Backend::ColorBlendOperator::HSLSaturation:
			return VK_BLEND_OP_HSL_SATURATION_EXT;

		case Xenon::Backend::ColorBlendOperator::HSLColor:
			return VK_BLEND_OP_HSL_COLOR_EXT;

		case Xenon::Backend::ColorBlendOperator::HSLLuminosity:
			return VK_BLEND_OP_HSL_LUMINOSITY_EXT;

		case Xenon::Backend::ColorBlendOperator::Plus:
			return VK_BLEND_OP_PLUS_EXT;

		case Xenon::Backend::ColorBlendOperator::PlusClamped:
			return VK_BLEND_OP_PLUS_CLAMPED_EXT;

		case Xenon::Backend::ColorBlendOperator::PlusClampedAlpha:
			return VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT;

		case Xenon::Backend::ColorBlendOperator::PlusDarker:
			return VK_BLEND_OP_PLUS_DARKER_EXT;

		case Xenon::Backend::ColorBlendOperator::Minus:
			return VK_BLEND_OP_MINUS_EXT;

		case Xenon::Backend::ColorBlendOperator::MinusClamped:
			return VK_BLEND_OP_MINUS_CLAMPED_EXT;

		case Xenon::Backend::ColorBlendOperator::Contrast:
			return VK_BLEND_OP_CONTRAST_EXT;

		case Xenon::Backend::ColorBlendOperator::InvertOVG:
			return VK_BLEND_OP_INVERT_OVG_EXT;

		case Xenon::Backend::ColorBlendOperator::Red:
			return VK_BLEND_OP_RED_EXT;

		case Xenon::Backend::ColorBlendOperator::Green:
			return VK_BLEND_OP_GREEN_EXT;

		case Xenon::Backend::ColorBlendOperator::Blue:
			return VK_BLEND_OP_BLUE_EXT;

		default:
			XENON_LOG_ERROR("Invalid color blend operator! Defaulting to Add.");
			return VK_BLEND_OP_ADD;
		}
	}

	/**
	 * Get the color component flags.
	 *
	 * @param mask The color write mask.
	 * @return The VUlkan mask.
	 */
	XENON_NODISCARD constexpr VkColorComponentFlags GetComponentFlags(Xenon::Backend::ColorWriteMask mask) noexcept
	{
		VkColorComponentFlags flags = 0;
		if (mask & Xenon::Backend::ColorWriteMask::R) flags |= VK_COLOR_COMPONENT_R_BIT;
		if (mask & Xenon::Backend::ColorWriteMask::G) flags |= VK_COLOR_COMPONENT_G_BIT;
		if (mask & Xenon::Backend::ColorWriteMask::B) flags |= VK_COLOR_COMPONENT_B_BIT;
		if (mask & Xenon::Backend::ColorWriteMask::A) flags |= VK_COLOR_COMPONENT_A_BIT;

		return flags;
	}

	/**
	 * Get the element format from the component count and the size.
	 *
	 * @param componentCount The number of components.
	 * @param dataType The component data type.
	 * @return The Vulkan format.
	 */
	XENON_NODISCARD constexpr VkFormat GetElementFormat(uint8_t componentCount, Xenon::Backend::ComponentDataType dataType) noexcept
	{
		if (componentCount == 1)
		{
			switch (dataType)
			{
			case Xenon::Backend::ComponentDataType::Uint8:
				return VK_FORMAT_R8_UINT;

			case Xenon::Backend::ComponentDataType::Uint16:
				return VK_FORMAT_R16_UINT;

			case Xenon::Backend::ComponentDataType::Uint32:
				return VK_FORMAT_R32_UINT;

			case Xenon::Backend::ComponentDataType::Uint64:
				return VK_FORMAT_R64_UINT;

			case Xenon::Backend::ComponentDataType::Int8:
				return VK_FORMAT_R8_SINT;

			case Xenon::Backend::ComponentDataType::Int16:
				return VK_FORMAT_R16_SINT;

			case Xenon::Backend::ComponentDataType::Int32:
				return VK_FORMAT_R32_SINT;

			case Xenon::Backend::ComponentDataType::Int64:
				return VK_FORMAT_R64_SINT;

			case Xenon::Backend::ComponentDataType::Float:
				return VK_FORMAT_R32_SFLOAT;

			default:
				break;
			}
		}
		else if (componentCount == 2)
		{
			switch (dataType)
			{
			case Xenon::Backend::ComponentDataType::Uint8:
				return VK_FORMAT_R8G8_UINT;

			case Xenon::Backend::ComponentDataType::Uint16:
				return VK_FORMAT_R16G16_UINT;

			case Xenon::Backend::ComponentDataType::Uint32:
				return VK_FORMAT_R32G32_UINT;

			case Xenon::Backend::ComponentDataType::Uint64:
				return VK_FORMAT_R64G64_UINT;

			case Xenon::Backend::ComponentDataType::Int8:
				return VK_FORMAT_R8G8_SINT;

			case Xenon::Backend::ComponentDataType::Int16:
				return VK_FORMAT_R16G16_SINT;

			case Xenon::Backend::ComponentDataType::Int32:
				return VK_FORMAT_R32G32_SINT;

			case Xenon::Backend::ComponentDataType::Int64:
				return VK_FORMAT_R64G64_SINT;

			case Xenon::Backend::ComponentDataType::Float:
				return VK_FORMAT_R32G32_SFLOAT;

			default:
				break;
			}
		}
		else if (componentCount == 3)
		{
			switch (dataType)
			{
			case Xenon::Backend::ComponentDataType::Uint8:
				return VK_FORMAT_R8G8B8_UINT;

			case Xenon::Backend::ComponentDataType::Uint16:
				return VK_FORMAT_R16G16B16_UINT;

			case Xenon::Backend::ComponentDataType::Uint32:
				return VK_FORMAT_R32G32B32_UINT;

			case Xenon::Backend::ComponentDataType::Uint64:
				return VK_FORMAT_R64G64B64_UINT;

			case Xenon::Backend::ComponentDataType::Int8:
				return VK_FORMAT_R8G8B8_SINT;

			case Xenon::Backend::ComponentDataType::Int16:
				return VK_FORMAT_R16G16B16_SINT;

			case Xenon::Backend::ComponentDataType::Int32:
				return VK_FORMAT_R32G32B32_SINT;

			case Xenon::Backend::ComponentDataType::Int64:
				return VK_FORMAT_R64G64B64_SINT;

			case Xenon::Backend::ComponentDataType::Float:
				return VK_FORMAT_R32G32B32_SFLOAT;

			default:
				break;
			}
		}
		else if (componentCount == 4)
		{
			switch (dataType)
			{
			case Xenon::Backend::ComponentDataType::Uint8:
				return VK_FORMAT_R8G8B8A8_UNORM;

			case Xenon::Backend::ComponentDataType::Uint16:
				return VK_FORMAT_R16G16B16A16_UINT;

			case Xenon::Backend::ComponentDataType::Uint32:
				return VK_FORMAT_R32G32B32A32_UINT;

			case Xenon::Backend::ComponentDataType::Uint64:
				return VK_FORMAT_R64G64B64A64_UINT;

			case Xenon::Backend::ComponentDataType::Int8:
				return VK_FORMAT_R8G8B8A8_SNORM;

			case Xenon::Backend::ComponentDataType::Int16:
				return VK_FORMAT_R16G16B16A16_SINT;

			case Xenon::Backend::ComponentDataType::Int32:
				return VK_FORMAT_R32G32B32A32_SINT;

			case Xenon::Backend::ComponentDataType::Int64:
				return VK_FORMAT_R64G64B64A64_SINT;

			case Xenon::Backend::ComponentDataType::Float:
				return VK_FORMAT_R32G32B32A32_SFLOAT;

			default:
				break;
			}
		}

		XENON_LOG_ERROR("There are no available types for the given component count ({}) and component data type ({})!", componentCount, Xenon::EnumToInt(dataType));
		return VK_FORMAT_UNDEFINED;
	}
}

namespace Xenon
{
	namespace Backend
	{
		VulkanRasterizingPipeline::VulkanRasterizingPipeline(VulkanDevice* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler, VulkanRasterizer* pRasterizer, const RasterizingPipelineSpecification& specification)
			: RasterizingPipeline(pDevice, std::move(pCacheHandler), pRasterizer, specification)
			, VulkanDeviceBoundObject(pDevice)
			, m_pRasterizer(pRasterizer)
		{
			// Get the shader information.
			std::vector<VkPushConstantRange> pushConstants;

			if (m_Specification.m_VertexShader.getSPIRV().isValid())
			{
				GetShaderBindings(m_Specification.m_VertexShader, m_BindingMap, pushConstants, m_VertexInputBindings, m_VertexInputAttributes, ShaderType::Vertex);

				auto& createInfo = m_ShaderStageCreateInfo.emplace_back();
				createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				createInfo.pNext = nullptr;
				createInfo.flags = 0;
				createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				createInfo.pName = m_Specification.m_VertexShader.getSPIRV().getEntryPoint().data();
				createInfo.pSpecializationInfo = nullptr;

				VkShaderModuleCreateInfo moduleCreateInfo = {};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.pNext = nullptr;
				moduleCreateInfo.flags = 0;
				moduleCreateInfo.codeSize = m_Specification.m_VertexShader.getSPIRV().getBinarySizeInBytes();
				moduleCreateInfo.pCode = m_Specification.m_VertexShader.getSPIRV().getBinaryData();

				XENON_VK_ASSERT(pDevice->getDeviceTable().vkCreateShaderModule(pDevice->getLogicalDevice(), &moduleCreateInfo, nullptr, &createInfo.module), "Failed to create the vertex shader module!");
			}

			if (m_Specification.m_FragmentShader.getSPIRV().isValid())
			{
				GetShaderBindings(m_Specification.m_FragmentShader, m_BindingMap, pushConstants, m_VertexInputBindings, m_VertexInputAttributes, ShaderType::Fragment);

				auto& createInfo = m_ShaderStageCreateInfo.emplace_back();
				createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				createInfo.pNext = nullptr;
				createInfo.flags = 0;
				createInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				createInfo.pName = m_Specification.m_FragmentShader.getSPIRV().getEntryPoint().data();
				createInfo.pSpecializationInfo = nullptr;

				VkShaderModuleCreateInfo moduleCreateInfo = {};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.pNext = nullptr;
				moduleCreateInfo.flags = 0;
				moduleCreateInfo.codeSize = m_Specification.m_FragmentShader.getSPIRV().getBinarySizeInBytes();
				moduleCreateInfo.pCode = m_Specification.m_FragmentShader.getSPIRV().getBinaryData();

				XENON_VK_ASSERT(pDevice->getDeviceTable().vkCreateShaderModule(pDevice->getLogicalDevice(), &moduleCreateInfo, nullptr, &createInfo.module), "Failed to create the fragment shader module!");
			}

			// Get the layouts.
			const std::array<VkDescriptorSetLayout, 4> layouts = {
				pDevice->getDescriptorSetManager()->getDescriptorSetLayout(m_BindingMap[DescriptorType::UserDefined]),
				pDevice->getDescriptorSetManager()->getDescriptorSetLayout(m_BindingMap[DescriptorType::Material]),
				pDevice->getDescriptorSetManager()->getDescriptorSetLayout(m_BindingMap[DescriptorType::PerGeometry]),
				pDevice->getDescriptorSetManager()->getDescriptorSetLayout(m_BindingMap[DescriptorType::Scene])
			};

			// Create the pipeline layout.
			createPipelineLayout(layouts, std::move(pushConstants));

			// Setup the initial pipeline data.
			setupPipelineInfo();
		}

		VulkanRasterizingPipeline::~VulkanRasterizingPipeline()
		{
			for (const auto& info : m_ShaderStageCreateInfo)
				m_pDevice->getDeviceTable().vkDestroyShaderModule(m_pDevice->getLogicalDevice(), info.module, nullptr);

			for (const auto& [hash, pipeline] : m_Pipelines)
			{
				m_pDevice->getDeviceTable().vkDestroyPipelineCache(m_pDevice->getLogicalDevice(), pipeline.m_PipelineCache, nullptr);
				m_pDevice->getDeviceTable().vkDestroyPipeline(m_pDevice->getLogicalDevice(), pipeline.m_Pipeline, nullptr);
			}

			m_pDevice->getDeviceTable().vkDestroyPipelineLayout(m_pDevice->getLogicalDevice(), m_PipelineLayout, nullptr);
		}

		std::unique_ptr<Xenon::Backend::Descriptor> VulkanRasterizingPipeline::createDescriptor(DescriptorType type)
		{
			OPTICK_EVENT();
			return std::make_unique<VulkanDescriptor>(m_pDevice, m_BindingMap[type], type);
		}

		const VulkanRasterizingPipeline::PipelineStorage& VulkanRasterizingPipeline::getPipeline(const VertexSpecification& vertexSpecification)
		{
			OPTICK_EVENT();

			const auto hash = vertexSpecification.generateHash();

			auto lock = std::scoped_lock(m_Mutex);
			if (!m_Pipelines.contains(hash))
			{
				auto& pipeline = m_Pipelines[hash];

				// Load the pipeline cache.
				loadPipelineCache(hash, pipeline);

				// Setup the inputs.
				pipeline.m_InputBindingDescriptions = m_VertexInputBindings;
				pipeline.m_InputAttributeDescriptions = m_VertexInputAttributes;

				bool hasVertexData = false;
				for (auto& attribute : pipeline.m_InputAttributeDescriptions)
				{
					// Continue if we're in instance data.
					if (attribute.binding == 1)
						continue;

					const auto element = static_cast<InputElement>(attribute.location);
					if (vertexSpecification.isAvailable(element))
					{
						attribute.offset = vertexSpecification.offsetOf(element);
						attribute.format = GetElementFormat(
							GetAttributeDataTypeComponentCount(vertexSpecification.getElementAttributeDataType(element)),
							vertexSpecification.getElementComponentDataType(element)
						);

						hasVertexData = true;
					}
				}

				// Sort the inputs.
				XENON_RANGES(sort, pipeline.m_InputAttributeDescriptions, [](const auto& lhs, const auto& rhs) { return lhs.offset < rhs.offset; });

				// Setup the input bindings if we have vertex data (stride is not 0).
				if (hasVertexData)
				{
					auto& binding = pipeline.m_InputBindingDescriptions.emplace_back();
					binding.binding = 0;
					binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
					binding.stride = vertexSpecification.getSize();
				}

				// Create the pipeline.
				createPipeline(pipeline);

				// Save the pipeline cache.
				savePipelineCache(hash, pipeline);
			}

			return m_Pipelines[hash];
		}

		void VulkanRasterizingPipeline::recreate()
		{
			OPTICK_EVENT();

			for (auto& [hash, pipeline] : m_Pipelines)
			{
				createPipeline(pipeline);
				savePipelineCache(hash, pipeline);
			}
		}

		void VulkanRasterizingPipeline::createPipelineLayout(const std::array<VkDescriptorSetLayout, 4>& layouts, std::vector<VkPushConstantRange>&& pushConstantRanges)
		{
			OPTICK_EVENT();

			VkPipelineLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
			createInfo.pSetLayouts = layouts.data();
			createInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
			createInfo.pPushConstantRanges = pushConstantRanges.data();

			XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkCreatePipelineLayout(m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_PipelineLayout), "Failed to create the pipeline layout!");
		}

		void VulkanRasterizingPipeline::loadPipelineCache(uint64_t hash, PipelineStorage& pipeline) const
		{
			OPTICK_EVENT();

			std::vector<std::byte> cacheData;
			if (m_pCacheHandler)
				cacheData = m_pCacheHandler->load(hash ^ g_MagicNumber);

			else
				XENON_LOG_INFORMATION("A pipeline cache handler was not set to load the pipeline cache.");

			VkPipelineCacheCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.initialDataSize = cacheData.size();
			createInfo.pInitialData = cacheData.data();

			const auto result = m_pDevice->getDeviceTable().vkCreatePipelineCache(m_pDevice->getLogicalDevice(), &createInfo, nullptr, &pipeline.m_PipelineCache);

			// If the error is unknown, try again without the cache data.
			if (result == VK_ERROR_UNKNOWN)
			{
				XENON_LOG_ERROR("Unknown Vulkan error caught while creating the pipeline cache object! Trying without the cache data.");

				createInfo.initialDataSize = 0;
				createInfo.pInitialData = nullptr;

				XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkCreatePipelineCache(m_pDevice->getLogicalDevice(), &createInfo, nullptr, &pipeline.m_PipelineCache), "Failed to load the pipeline cache!");
			}
			else
			{
				XENON_VK_ASSERT(result, "Failed to load the pipeline cache!");
			}
		}

		void VulkanRasterizingPipeline::savePipelineCache(uint64_t hash, PipelineStorage& pipeline) const
		{
			OPTICK_EVENT();

			if (m_pCacheHandler)
			{
				size_t cacheSize = 0;
				XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkGetPipelineCacheData(m_pDevice->getLogicalDevice(), pipeline.m_PipelineCache, &cacheSize, nullptr), "Failed to get the pipeline cache size!");

				auto cacheData = std::vector<std::byte>(cacheSize);
				XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkGetPipelineCacheData(m_pDevice->getLogicalDevice(), pipeline.m_PipelineCache, &cacheSize, cacheData.data()), "Failed to get the pipeline cache data!");

				m_pCacheHandler->store(hash ^ g_MagicNumber, cacheData);
			}
			else
			{
				XENON_LOG_INFORMATION("A pipeline cache handler was not set to save the pipeline cache.");
			}
		}

		void VulkanRasterizingPipeline::setupPipelineInfo()
		{
			OPTICK_EVENT();

			// Input assembly state.
			m_InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			m_InputAssemblyStateCreateInfo.pNext = nullptr;
			m_InputAssemblyStateCreateInfo.flags = 0;
			m_InputAssemblyStateCreateInfo.primitiveRestartEnable = XENON_VK_BOOL(m_Specification.m_EnablePrimitiveRestart);
			m_InputAssemblyStateCreateInfo.topology = GetPrimitiveTopology(m_Specification.m_PrimitiveTopology);

			// Viewport state.
			m_ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			m_ViewportStateCreateInfo.pNext = nullptr;
			m_ViewportStateCreateInfo.flags = 0;
			m_ViewportStateCreateInfo.scissorCount = 1;
			m_ViewportStateCreateInfo.pScissors = nullptr;
			m_ViewportStateCreateInfo.viewportCount = 1;
			m_ViewportStateCreateInfo.pViewports = nullptr;

			// Tessellation state.
			m_TessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			m_TessellationStateCreateInfo.pNext = nullptr;
			m_TessellationStateCreateInfo.flags = 0;
			m_TessellationStateCreateInfo.patchControlPoints = m_Specification.m_TessellationPatchControlPoints;

			// Color blend state.
			if (m_pRasterizer->getAttachmentTypes() & AttachmentType::Color)
			{
				for (const auto& attachment : m_Specification.m_ColorBlendAttachments)
				{
					auto& vAttachmentState = m_CBASS.emplace_back();
					vAttachmentState.blendEnable = XENON_VK_BOOL(attachment.m_EnableBlend);
					vAttachmentState.alphaBlendOp = GetBlendOp(attachment.m_AlphaBlendOperator);
					vAttachmentState.colorBlendOp = GetBlendOp(attachment.m_BlendOperator);
					vAttachmentState.colorWriteMask = GetComponentFlags(attachment.m_ColorWriteMask);
					vAttachmentState.srcColorBlendFactor = GetBlendFactor(attachment.m_SrcBlendFactor);
					vAttachmentState.dstColorBlendFactor = GetBlendFactor(attachment.m_DstBlendFactor);
					vAttachmentState.srcAlphaBlendFactor = GetBlendFactor(attachment.m_SrcAlphaBlendFactor);
					vAttachmentState.dstAlphaBlendFactor = GetBlendFactor(attachment.m_DstAlphaBlendFactor);
				}
			}

			m_ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			m_ColorBlendStateCreateInfo.pNext = nullptr;
			m_ColorBlendStateCreateInfo.flags = 0;
			m_ColorBlendStateCreateInfo.logicOp = GetLogicOp(m_Specification.m_ColorBlendLogic);
			m_ColorBlendStateCreateInfo.logicOpEnable = XENON_VK_BOOL(m_Specification.m_EnableColorBlendLogic);

#ifdef XENON_PLATFORM_WINDOWS
			std::copy_n(std::execution::unseq, m_Specification.m_ColorBlendConstants.data(), 4, m_ColorBlendStateCreateInfo.blendConstants);

#else 
			std::copy_n(m_Specification.m_ColorBlendConstants.data(), 4, m_ColorBlendStateCreateInfo.blendConstants);

#endif

			m_ColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(m_CBASS.size());
			m_ColorBlendStateCreateInfo.pAttachments = m_CBASS.data();

			// Rasterization state.
			m_RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			m_RasterizationStateCreateInfo.pNext = nullptr;
			m_RasterizationStateCreateInfo.flags = 0;
			m_RasterizationStateCreateInfo.cullMode = GetCullMode(m_Specification.m_CullMode);
			m_RasterizationStateCreateInfo.depthBiasEnable = XENON_VK_BOOL(m_Specification.m_EnableDepthBias);
			m_RasterizationStateCreateInfo.depthBiasClamp = m_Specification.m_DepthBiasFactor;
			m_RasterizationStateCreateInfo.depthBiasConstantFactor = m_Specification.m_DepthConstantFactor;
			m_RasterizationStateCreateInfo.depthBiasSlopeFactor = m_Specification.m_DepthSlopeFactor;
			m_RasterizationStateCreateInfo.depthClampEnable = XENON_VK_BOOL(m_Specification.m_EnableDepthClamp);
			m_RasterizationStateCreateInfo.frontFace = GetFrontFace(m_Specification.m_FrontFace);
			m_RasterizationStateCreateInfo.lineWidth = m_Specification.m_RasterizerLineWidth;
			m_RasterizationStateCreateInfo.polygonMode = GetPolygonMode(m_Specification.m_PolygonMode);
			m_RasterizationStateCreateInfo.rasterizerDiscardEnable = XENON_VK_BOOL(m_Specification.m_EnableRasterizerDiscard);

			// Multisample state.
			m_MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			m_MultisampleStateCreateInfo.pNext = nullptr;
			m_MultisampleStateCreateInfo.flags = 0;
			m_MultisampleStateCreateInfo.alphaToCoverageEnable = XENON_VK_BOOL(m_Specification.m_EnableAlphaCoverage);
			m_MultisampleStateCreateInfo.alphaToOneEnable = XENON_VK_BOOL(m_Specification.m_EnableAlphaToOne);
			m_MultisampleStateCreateInfo.minSampleShading = m_Specification.m_MinSampleShading;
			m_MultisampleStateCreateInfo.pSampleMask = nullptr;	// TODO

			if (m_pRasterizer->getAttachmentTypes() & AttachmentType::Color)
				m_MultisampleStateCreateInfo.rasterizationSamples = VulkanDevice::ConvertSamplingCount(m_pRasterizer->getImageAttachment(AttachmentType::Color)->getSpecification().m_MultiSamplingCount);
			else
				m_MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			m_MultisampleStateCreateInfo.sampleShadingEnable = XENON_VK_BOOL(m_Specification.m_EnableSampleShading);

			// Depth stencil state.
			m_DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			m_DepthStencilStateCreateInfo.pNext = nullptr;
			m_DepthStencilStateCreateInfo.flags = 0;
			m_DepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			m_DepthStencilStateCreateInfo.depthTestEnable = XENON_VK_BOOL(m_Specification.m_EnableDepthTest);
			m_DepthStencilStateCreateInfo.depthWriteEnable = XENON_VK_BOOL(m_Specification.m_EnableDepthWrite);
			m_DepthStencilStateCreateInfo.depthCompareOp = GetCompareOp(m_Specification.m_DepthCompareLogic);

			// Dynamic state.
			m_DynamicStates = GetDynamicStates(m_Specification.m_DynamicStateFlags);

			m_DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			m_DynamicStateCreateInfo.pNext = VK_NULL_HANDLE;
			m_DynamicStateCreateInfo.flags = 0;
			m_DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
			m_DynamicStateCreateInfo.pDynamicStates = m_DynamicStates.data();
		}

		void VulkanRasterizingPipeline::createPipeline(PipelineStorage& pipeline) const
		{
			OPTICK_EVENT();

			if (pipeline.m_Pipeline != VK_NULL_HANDLE)
				m_pDevice->getDeviceTable().vkDestroyPipeline(m_pDevice->getLogicalDevice(), pipeline.m_Pipeline, nullptr);

			VkPipelineVertexInputStateCreateInfo inputState = {};
			inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			inputState.flags = 0;
			inputState.pNext = nullptr;
			inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline.m_InputBindingDescriptions.size());
			inputState.pVertexBindingDescriptions = pipeline.m_InputBindingDescriptions.data();
			inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline.m_InputAttributeDescriptions.size());
			inputState.pVertexAttributeDescriptions = pipeline.m_InputAttributeDescriptions.data();

			// Pipeline create info.
			VkGraphicsPipelineCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.stageCount = static_cast<uint32_t>(m_ShaderStageCreateInfo.size());
			createInfo.pStages = m_ShaderStageCreateInfo.data();
			createInfo.pVertexInputState = &inputState;
			createInfo.pInputAssemblyState = &m_InputAssemblyStateCreateInfo;
			createInfo.pTessellationState = &m_TessellationStateCreateInfo;
			createInfo.pViewportState = &m_ViewportStateCreateInfo;
			createInfo.pRasterizationState = &m_RasterizationStateCreateInfo;
			createInfo.pMultisampleState = &m_MultisampleStateCreateInfo;
			createInfo.pDepthStencilState = &m_DepthStencilStateCreateInfo;
			createInfo.pColorBlendState = &m_ColorBlendStateCreateInfo;
			createInfo.pDynamicState = &m_DynamicStateCreateInfo;
			createInfo.layout = m_PipelineLayout;
			createInfo.renderPass = m_pRasterizer->getRenderPass();
			createInfo.subpass = 0;	// TODO
			createInfo.basePipelineHandle = VK_NULL_HANDLE;
			createInfo.basePipelineIndex = 0;

			XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkCreateGraphicsPipelines(m_pDevice->getLogicalDevice(), pipeline.m_PipelineCache, 1, &createInfo, nullptr, &pipeline.m_Pipeline), "Failed to create the pipeline!");
		}
	}
}