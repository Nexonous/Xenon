// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#define XENON_NAME_CONCAT(first, second)	first##second

#define XENON_SETUP_DESCRIPTOR(set, bindingIndex)	[[vk::binding(bindingIndex, set)]]

#define XENON_DESCRIPTOR_TYPE_USER_DEFINED	0
#define XENON_DESCRIPTOR_TYPE_MATERIAL		1
#define XENON_DESCRIPTOR_TYPE_SCENE			2

#define XENON_SCENE_DESCRIPTOR_BINDING_CAMERA					0
#define XENON_SCENE_DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE	1
#define XENON_SCENE_DESCRIPTOR_BINDING_RENDER_TARGET			2

#define XENON_DESCRIPTOR_SPACE(index)		XENON_NAME_CONCAT(space, index)

#define XENON_SETUP_CAMERA(camera, name)																		\
	XENON_SETUP_DESCRIPTOR(XENON_DESCRIPTOR_TYPE_SCENE, XENON_SCENE_DESCRIPTOR_BINDING_CAMERA)					\
	cbuffer name : register(b0, XENON_DESCRIPTOR_SPACE(XENON_DESCRIPTOR_TYPE_SCENE)) { camera name; }

#define XENON_SETUP_ACCELERATION_STRUCTURE(name)																\
	XENON_SETUP_DESCRIPTOR(XENON_DESCRIPTOR_TYPE_SCENE, XENON_SCENE_DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE) 	\
	RaytracingAccelerationStructure name : register(t1, XENON_DESCRIPTOR_SPACE(XENON_DESCRIPTOR_TYPE_SCENE))

#define XENON_SETUP_RENDER_TARGET_IMAGE(type, name)																\
	XENON_SETUP_DESCRIPTOR(XENON_DESCRIPTOR_TYPE_SCENE, XENON_SCENE_DESCRIPTOR_BINDING_RENDER_TARGET)			\
	RWTexture2D<type> name : register(u2, XENON_DESCRIPTOR_SPACE(XENON_DESCRIPTOR_TYPE_SCENE))	

float4x4 GetIdentityMatrix()
{
	float4x4 identity =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};

	return identity;
}

/**
 * Light source structure.
 * This structure contains information about a single light source (point or directional).
 */
struct LightSource
{
	float4 m_Color;
	float3 m_Position;
	float3 m_Direction;

	float m_Intensity;
	float m_FieldAngle;
};

/**
 * Scene information structure.
 * This structure contains information about the current scene.
 */
struct SceneInformation
{
	uint m_LightSourceCount;
};

#endif // COMMON_HLSLI