/**
 * Notes for myself
 * 1. How to add a new uniform buffer?
 *   - Add the uniform buffer in the shaders that use it
 *   - If it is a uniform buffer, add the new struct with the same layout as the uniform
 *     somewhere visible to this file
 *   - Add an instance in the DeferredRenderer class
 *   - Create a VkBuffer and allocate device memory to interface with the device
 *   - Increase descriptor pool size accordingly
 *   - Create a VkDescriptorSetLayoutBinding to put it into descriptor set layouts that use it
 *   - Create a VkDescriptorBufferInfo and VkWriteDescriptorSet to write the uniform descriptor
 *     into the sets that use it
 *   - Allocate host memory in DeferredRenderer::createUniformBuffers
 *   - Update it in DeferredRenderer::updateUniformBuffers and copy data to device memory
 *
 * 2. How to add a new texture sampled in shaders?
 *   - Load in the texture
 *   - Create a staging buffer and copy pixel data to it. Buffer usage need to have VK_BUFFER_USAGE_TRANSFER_SRC_BIT set.
 *     Memory properties are VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
 *   - Create a device local image with the same format and transfer data from staging buffer to it
 *   - Transfer image layout from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 *   - Create a VkImageView and VkSampler
 *   - Increase descriptor pool size
 *   - Add new VkDescriptorSetLayoutBindings to the descriptor set layouts that use the texture
 *   - Add new VkDescritorImageInfos and VkWriteDescriptorSets to the descriptor sets that use it
 *   - Add uniform Sampler2D's to shaders and bind desired descriptor sets before draw calls
 *
 * 3. How to create an image and use it as an attachment?
 *   - Create a VkImage, VkDeviceMemory, and VkImageView
 *   - Image format is usually VK_FORMAT_R8G8B8A8_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, or VK_FORMAT_R32G32B32A32_SFLOAT.
 *     For depth image, you need to find a format that is supported by your device and prefer the one with higher precision
 *   - Image usage is VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, or VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
 *     depend on how you want to use it
 *   - Attach it to the framebuffers of the render passes that use this attachment
 *   - Add VkAttachmentDescriptions and VkAttachmentReferences to the render passes and subpasses that use it
 *     specify initial, final layouts, and the layouts the image should have during each subpass that uses it
 *   - Configure pipelines accordingly. For example, enable depth test or color blending
 *   - If the attachment will be used as an input attachment, you will also need to add VkDescriptorSetLayoutBindings and
 *     VkWriteDescriptorSets to the layouts and sets that use it. In the shaders, you will need to add a uniform subpassInput
 *     with the correct input_attachment_index specified in the layout qualifier. The index is the attachment's index in the
 *     pInputAttachments array of VkSubpassDescription. And instead of using texture(), use subpassLoad() to read the input
 *     attachment. Input attachment doesn't support magnification, minification, or sampling. So subpasses that use input
 *     attachments should render into attachments with the same resolution as input attachments
 *   - Provide a clear value when recording command buffers if the loadOp is VK_LOAD_OP_CLEAR
 */

#pragma once

#include <array>

#include "vbase.h"

#define BRDF_LUT_SIZE 256
#define ALL_UNIFORM_BLOB_SIZE (64 * 1024)
#define NUM_LIGHTS 2

#define BRDF_BASE_DIR "../textures/BRDF_LUTs/"
#define BRDF_NAME "FSchlick_DGGX_GSmith.dds"

#define PROBE_BASE_DIR "../textures/Environment/PaperMill/"
//#define PROBE_BASE_DIR "../textures/Environment/Factory/"
//#define PROBE_BASE_DIR "../textures/Environment/MonValley/"
//#define PROBE_BASE_DIR "../textures/Environment/Canyon/"

#define MODEL_NAMES { "Cerberus" }
//#define MODEL_NAMES { "Jeep_Wagoneer" }
//#define MODEL_NAMES { "9mm_Pistol" }
//#define MODEL_NAMES { "Drone_Body", "Drone_Legs", "Floor" }
//#define MODEL_NAMES { "Combat_Helmet" }
//#define MODEL_NAMES { "Bug_Ship" }
//#define MODEL_NAMES { "Knight_Base", "Knight_Helmet", "Knight_Chainmail", "Knight_Skirt", "Knight_Sword", "Knight_Armor" }


struct CubeMapCameraUniformBuffer
{
	glm::mat4 V[6];
	glm::mat4 P;
};

struct TransMatsUniformBuffer
{
	glm::mat4 VP;
};

struct PointLight
{
	glm::vec4 position;
	glm::vec3 color;
	float radius;
};

// due to std140 padding for uniform buffer object
// only use data types that are vec4 or multiple of vec4's
struct LightingPassUniformBuffer
{
	glm::vec4 eyePos;
	PointLight pointLights[NUM_LIGHTS];
};

struct DisplayInfoUniformBuffer
{
	typedef int DisplayMode_t;
	
	DisplayMode_t displayMode;
};


class DeferredRenderer : public VBaseGraphics
{
public:
	DeferredRenderer()
	{
		m_verNumMajor = 0;
		m_verNumMinor = 1;

		VkPhysicalDeviceProperties props;
		m_vulkanManager.getPhysicalDeviceProperties(&props);
		m_allUniformHostData.setAlignment(props.limits.minUniformBufferOffsetAlignment);
	}

	virtual void run();

protected:
	uint32_t m_specEnvPrefilterRenderPass;
	uint32_t m_geomAndLightRenderPass;
	std::vector<uint32_t> m_bloomRenderPasses;
	uint32_t m_finalOutputRenderPass;

	uint32_t m_brdfLutDescriptorSetLayout;
	uint32_t m_specEnvPrefilterDescriptorSetLayout;
	uint32_t m_skyboxDescriptorSetLayout;
	uint32_t m_geomDescriptorSetLayout;
	uint32_t m_lightingDescriptorSetLayout;
	uint32_t m_bloomDescriptorSetLayout;
	uint32_t m_finalOutputDescriptorSetLayout;

	uint32_t m_brdfLutPipelineLayout;
	uint32_t m_diffEnvPrefilterPipelineLayout;
	uint32_t m_specEnvPrefilterPipelineLayout;
	uint32_t m_skyboxPipelineLayout;
	uint32_t m_geomPipelineLayout;
	uint32_t m_lightingPipelineLayout;
	std::vector<uint32_t> m_bloomPipelineLayouts;
	uint32_t m_finalOutputPipelineLayout;

	uint32_t m_brdfLutPipeline;
	uint32_t m_diffEnvPrefilterPipeline;
	uint32_t m_specEnvPrefilterPipeline;
	uint32_t m_skyboxPipeline;
	uint32_t m_geomPipeline;
	uint32_t m_lightingPipeline;
	std::vector<uint32_t> m_bloomPipelines;
	uint32_t m_finalOutputPipeline;

	ImageWrapper m_depthImage;
	ImageWrapper m_lightingResultImage; // VK_FORMAT_R16G16B16A16_SFLOAT
	const uint32_t m_numGBuffers = 3;
	const std::vector<VkFormat> m_gbufferFormats =
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R8G8B8A8_UNORM
	};
	std::vector<ImageWrapper> m_gbufferImages; // GB1: VK_FORMAT_R32G32B32A32_SFLOAT, GB2: VK_FORMAT_R32G32B32A32_SFLOAT, GB3: VK_FORMAT_R8G8B8A8_UNORM
	const uint32_t m_numPostEffectImages = 2;
	const std::vector<VkFormat> m_postEffectImageFormats =
	{
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R16G16B16A16_SFLOAT
	};
	std::vector<ImageWrapper> m_postEffectImages; // Image1: VK_FORMAT_R16G16B16A16_SFLOAT, Image2: VK_FORMAT_R16G16B16A16_SFLOAT

	rj::helper_functions::UniformBlob<ALL_UNIFORM_BLOB_SIZE> m_allUniformHostData;
	CubeMapCameraUniformBuffer *m_uCubeViews = nullptr;
	TransMatsUniformBuffer *m_uTransMats = nullptr;
	LightingPassUniformBuffer *m_uLightInfo = nullptr;
	DisplayInfoUniformBuffer *m_uDisplayInfo = nullptr;
	BufferWrapper m_allUniformBuffer;

	uint32_t m_brdfLutDescriptorSet;
	uint32_t m_specEnvPrefilterDescriptorSet;
	uint32_t m_skyboxDescriptorSet;
	std::vector<uint32_t> m_geomDescriptorSets; // one set per model
	uint32_t m_lightingDescriptorSet;
	std::vector<uint32_t> m_bloomDescriptorSets;
	uint32_t m_finalOutputDescriptorSet;

	uint32_t m_diffEnvPrefilterFramebuffer;
	std::vector<uint32_t> m_specEnvPrefilterFramebuffers;
	uint32_t m_geomAndLightingFramebuffer;
	std::vector<uint32_t> m_postEffectFramebuffers;

	uint32_t m_imageAvailableSemaphore;
	uint32_t m_geomAndLightingCompleteSemaphore;
	uint32_t m_postEffectSemaphore;
	uint32_t m_finalOutputFinishedSemaphore;
	uint32_t m_renderFinishedSemaphore;

	uint32_t m_brdfLutFence;
	uint32_t m_envPrefilterFence;

	uint32_t m_brdfLutCommandBuffer;
	uint32_t m_envPrefilterCommandBuffer;
	uint32_t m_geomAndLightingCommandBuffer;
	uint32_t m_postEffectCommandBuffer;


	virtual const std::string &getWindowTitle();

	virtual void createRenderPasses();
	virtual void createDescriptorSetLayouts();
	virtual void createComputePipelines();
	virtual void createGraphicsPipelines();
	virtual void createCommandPools();
	virtual void createComputeResources();
	virtual void createDepthResources();
	virtual void createColorAttachmentResources();
	virtual void loadAndPrepareAssets();
	virtual void createUniformBuffers();
	virtual void createDescriptorPools();
	virtual void createDescriptorSets();
	virtual void createFramebuffers();
	virtual void createCommandBuffers();
	virtual void createSynchronizationObjects(); // semaphores, fences, etc. go in here

	virtual void updateUniformBuffers();
	virtual void drawFrame();

	// Helpers
	virtual void createSpecEnvPrefilterRenderPass();
	virtual void createGeometryAndLightingRenderPass();
	virtual void createBloomRenderPasses();
	virtual void createFinalOutputRenderPass();

	virtual void createBrdfLutDescriptorSetLayout();
	virtual void createSpecEnvPrefilterDescriptorSetLayout();
	virtual void createSkyboxDescriptorSetLayout();
	virtual void createStaticMeshDescriptorSetLayout();
	virtual void createGeomPassDescriptorSetLayout();
	virtual void createLightingPassDescriptorSetLayout();
	virtual void createBloomDescriptorSetLayout();
	virtual void createFinalOutputDescriptorSetLayout();

	virtual void createBrdfLutPipeline();
	virtual void createDiffEnvPrefilterPipeline();
	virtual void createSpecEnvPrefilterPipeline();
	virtual void createSkyboxPipeline();
	virtual void createStaticMeshPipeline();
	virtual void createGeomPassPipeline();
	virtual void createLightingPassPipeline();
	virtual void createBloomPipelines();
	virtual void createFinalOutputPassPipeline();

	// Descriptor sets cannot be altered once they are bound until execution of all related
	// commands complete. So each model will need a different descriptor set because they use
	// different textures
	virtual void createBrdfLutDescriptorSet();
	virtual void createSpecEnvPrefilterDescriptorSet();
	virtual void createSkyboxDescriptorSet();
	virtual void createStaticMeshDescriptorSet();
	virtual void createGeomPassDescriptorSets();
	virtual void createLightingPassDescriptorSets();
	virtual void createBloomDescriptorSets();
	virtual void createFinalOutputPassDescriptorSets();

	virtual void createBrdfLutCommandBuffer();
	virtual void createEnvPrefilterCommandBuffer();
	virtual void createGeomAndLightingCommandBuffer();
	virtual void createPostEffectCommandBuffer();
	virtual void createPresentCommandBuffers();

	virtual void prefilterEnvironmentAndComputeBrdfLut();
	virtual void savePrecomputationResults();

	virtual VkFormat findDepthFormat();
};


#ifdef DEFERED_RENDERER_IMPLEMENTATION

const std::string &DeferredRenderer::getWindowTitle()
{
	m_windowTitle = "Laugh Engine";
	return m_windowTitle;
}

void DeferredRenderer::run()
{
	//system("pause");
	initVulkan();
	prefilterEnvironmentAndComputeBrdfLut();
	mainLoop();
	savePrecomputationResults();
}

void DeferredRenderer::updateUniformBuffers()
{
	// cube map views and projection
	m_uCubeViews->V[0] = glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));	// +X
	m_uCubeViews->V[1] = glm::lookAt(glm::vec3(0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));	// -X
	m_uCubeViews->V[2] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));	// +Y
	m_uCubeViews->V[3] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f));	// -Y
	m_uCubeViews->V[4] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, -1.f, 0.f));	// +Z
	m_uCubeViews->V[5] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, -1.f, 0.f));	// -Z
	m_uCubeViews->P = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);

	// update final output pass info
	*m_uDisplayInfo =
	{
		m_displayMode
	};

	// update transformation matrices
	glm::mat4 V, P;
	m_camera.getViewProjMatrix(V, P);

	m_uTransMats->VP = P * V;

	// update lighting info
	m_uLightInfo->pointLights[0] =
	{
		glm::vec4(1.f, 2.f, 2.f, 1.f),
		glm::vec3(4.f, 4.f, 4.f),
		5.f
	};
	m_uLightInfo->pointLights[1] =
	{
		glm::vec4(-0.5f, 2.f, -2.f, 1.f),
		glm::vec3(1.5f, 1.5f, 1.5f),
		5.f
	};
	m_uLightInfo->eyePos = glm::vec4(m_camera.position, 1.0f);

	// update per model information
	for (auto &model : m_models)
	{
		model.updateHostUniformBuffer();
	}

	void* data = m_vulkanManager.mapBuffer(m_allUniformBuffer.buffer);
	memcpy(data, &m_allUniformHostData, m_allUniformBuffer.size);
	m_vulkanManager.unmapBuffer(m_allUniformBuffer.buffer);
}

void DeferredRenderer::drawFrame()
{
	uint32_t imageIndex;

	// acquired image may not be renderable because the presentation engine is still using it
	// when @m_imageAvailableSemaphore is signaled, presentation is complete and the image can be used for rendering
	VkResult result = m_vulkanManager.swapChainNextImageIndex(&imageIndex, m_imageAvailableSemaphore, std::numeric_limits<uint32_t>::max());

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	m_vulkanManager.beginQueueSubmit(VK_QUEUE_GRAPHICS_BIT);

	m_vulkanManager.queueSubmitNewSubmit({ m_geomAndLightingCommandBuffer }, { m_imageAvailableSemaphore },
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_geomAndLightingCompleteSemaphore });

	m_vulkanManager.queueSubmitNewSubmit({ m_postEffectCommandBuffer }, { m_geomAndLightingCompleteSemaphore },
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_postEffectSemaphore });

	m_vulkanManager.queueSubmitNewSubmit({ m_presentCommandBuffers[imageIndex] }, { m_postEffectSemaphore },
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_finalOutputFinishedSemaphore });

	m_vulkanManager.endQueueSubmit();

	result = m_vulkanManager.queuePresent({ m_finalOutputFinishedSemaphore }, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}
}

void DeferredRenderer::createRenderPasses()
{
	createSpecEnvPrefilterRenderPass();
	createGeometryAndLightingRenderPass();
	createBloomRenderPasses();
	createFinalOutputRenderPass();
}

void DeferredRenderer::createDescriptorSetLayouts()
{
	createBrdfLutDescriptorSetLayout();
	createSpecEnvPrefilterDescriptorSetLayout();
	createGeomPassDescriptorSetLayout();
	createLightingPassDescriptorSetLayout();
	createBloomDescriptorSetLayout();
	createFinalOutputDescriptorSetLayout();
}

void DeferredRenderer::createComputePipelines()
{
	createBrdfLutPipeline();
}

void DeferredRenderer::createGraphicsPipelines()
{
	createDiffEnvPrefilterPipeline();
	createSpecEnvPrefilterPipeline();
	createGeomPassPipeline();
	createLightingPassPipeline();
	createBloomPipelines();
	createFinalOutputPassPipeline();
}

void DeferredRenderer::createCommandPools()
{
	m_graphicsCommandPool = m_vulkanManager.createCommandPool(VK_QUEUE_GRAPHICS_BIT);
	m_computeCommandPool = m_vulkanManager.createCommandPool(VK_QUEUE_COMPUTE_BIT);
}

void DeferredRenderer::createComputeResources()
{
	using namespace rj::helper_functions;

	// BRDF LUT
	std::string brdfFileName = "";
	if (fileExist(BRDF_BASE_DIR BRDF_NAME))
	{
		brdfFileName = BRDF_BASE_DIR BRDF_NAME;
	}

	m_bakedBRDFs.resize(1, {});

	if (brdfFileName != "")
	{
		// Do not generate mip levels for BRDF LUTs
		loadTexture2D(&m_bakedBRDFs[0], &m_vulkanManager, brdfFileName, false, false);
		m_bakedBRDFs[0].samplers.resize(1);
		m_bakedBRDFs[0].samplers[0] = m_vulkanManager.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		m_bakedBrdfReady = true;
	}
	else
	{
		m_bakedBRDFs[0].format = VK_FORMAT_R32G32_SFLOAT;
		m_bakedBRDFs[0].width = BRDF_LUT_SIZE;
		m_bakedBRDFs[0].height = BRDF_LUT_SIZE;
		m_bakedBRDFs[0].depth = 1;
		m_bakedBRDFs[0].mipLevelCount = 1;
		m_bakedBRDFs[0].layerCount = 1;

		m_bakedBRDFs[0].image = m_vulkanManager.createImage2D(m_bakedBRDFs[0].width, m_bakedBRDFs[0].height, m_bakedBRDFs[0].format,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		m_vulkanManager.transitionImageLayout(m_bakedBRDFs[0].image, VK_IMAGE_LAYOUT_GENERAL);

		m_bakedBRDFs[0].imageViews.resize(1);
		m_bakedBRDFs[0].imageViews[0] = m_vulkanManager.createImageView2D(m_bakedBRDFs[0].image, VK_IMAGE_ASPECT_COLOR_BIT);

		m_bakedBRDFs[0].samplers.resize(1);
		m_bakedBRDFs[0].samplers[0] = m_vulkanManager.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		m_shouldSaveBakedBrdf = true;
	}
}

void DeferredRenderer::createDepthResources()
{
	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	VkFormat depthFormat = findDepthFormat();

	m_depthImage.format = depthFormat;
	m_depthImage.width = swapChainExtent.width;
	m_depthImage.height = swapChainExtent.height;
	m_depthImage.depth = 1;
	m_depthImage.mipLevelCount = 1;
	m_depthImage.layerCount = 1;

	m_depthImage.image = m_vulkanManager.createImage2D(m_depthImage.width, m_depthImage.height, m_depthImage.format,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_depthImage.imageViews.resize(1);
	m_depthImage.imageViews[0] = m_vulkanManager.createImageView2D(m_depthImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	m_vulkanManager.transitionImageLayout(m_depthImage.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	m_depthImage.samplers.resize(1);
	m_depthImage.samplers[0] = m_vulkanManager.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void DeferredRenderer::createColorAttachmentResources()
{
	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();

	// Gbuffer images
	m_gbufferImages.resize(m_numGBuffers);
	for (uint32_t i = 0; i < m_numGBuffers; ++i)
	{
		auto &image = m_gbufferImages[i];
		image.format = m_gbufferFormats[i];
		image.width = swapChainExtent.width;
		image.height = swapChainExtent.height;
		image.depth = 1;
		image.mipLevelCount = 1;
		image.layerCount = 1;

		image.image = m_vulkanManager.createImage2D(image.width, image.height, image.format,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		image.imageViews.resize(1);
		image.imageViews[0] = m_vulkanManager.createImageView2D(image.image, VK_IMAGE_ASPECT_COLOR_BIT);

		image.samplers.resize(1);
		image.samplers[0] = m_vulkanManager.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}

	// Lighting result image
	m_lightingResultImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_lightingResultImage.width = swapChainExtent.width;
	m_lightingResultImage.height = swapChainExtent.height;
	m_lightingResultImage.depth = 1;
	m_lightingResultImage.mipLevelCount = 1;
	m_lightingResultImage.layerCount = 1;

	m_lightingResultImage.image = m_vulkanManager.createImage2D(m_lightingResultImage.width, m_lightingResultImage.height, m_lightingResultImage.format,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_lightingResultImage.imageViews.resize(1);
	m_lightingResultImage.imageViews[0] = m_vulkanManager.createImageView2D(m_lightingResultImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	m_lightingResultImage.samplers.resize(1);
	m_lightingResultImage.samplers[0] = m_vulkanManager.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	// post effects
	m_postEffectImages.resize(m_numPostEffectImages);
	for (uint32_t i = 0; i < m_numPostEffectImages; ++i)
	{
		auto &image = m_postEffectImages[i];
		image.format = m_postEffectImageFormats[i];
		image.width = swapChainExtent.width;
		image.height = swapChainExtent.height;
		image.depth = 1;
		image.mipLevelCount = 1;
		image.layerCount = 1;

		image.image = m_vulkanManager.createImage2D(image.width, image.height, image.format,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		image.imageViews.resize(1);
		image.imageViews[0] = m_vulkanManager.createImageView2D(image.image, VK_IMAGE_ASPECT_COLOR_BIT);

		image.samplers.resize(1);
		image.samplers[0] = m_vulkanManager.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}
}

void DeferredRenderer::loadAndPrepareAssets()
{
	using namespace rj::helper_functions;

	// TODO: implement scene file to allow flexible model loading
	std::string skyboxFileName = "../models/sky_sphere.obj";
	std::string unfilteredProbeFileName = PROBE_BASE_DIR "Unfiltered_HDR.dds";
	std::string specProbeFileName = "";
	if (fileExist(PROBE_BASE_DIR "Specular_HDR.dds"))
	{
		specProbeFileName = PROBE_BASE_DIR "Specular_HDR.dds";
	}
	std::string diffuseProbeFileName = "";
	if (fileExist(PROBE_BASE_DIR "Diffuse_HDR.dds"))
	{
		diffuseProbeFileName = PROBE_BASE_DIR "Diffuse_HDR.dds";
	}

	m_skybox.load(skyboxFileName, unfilteredProbeFileName, specProbeFileName, diffuseProbeFileName);

	std::vector<std::string> modelNames = MODEL_NAMES;
	m_models.resize(modelNames.size(), { &m_vulkanManager });

	for (size_t i = 0; i < m_models.size(); ++i)
	{
		const std::string &name = modelNames[i];

		std::string modelFileName = "../models/" + name + ".obj";
		std::string albedoMapName = "../textures/" + name + "/A.dds";
		std::string normalMapName = "../textures/" + name + "/N.dds";
		std::string roughnessMapName = "../textures/" + name + "/R.dds";
		std::string metalnessMapName = "../textures/" + name + "/M.dds";
		std::string aoMapName = "";
		if (fileExist("../textures/" + name + "/AO.dds"))
		{
			aoMapName = "../textures/" + name + "/AO.dds";
		}

		m_models[i].load(modelFileName, albedoMapName, normalMapName, roughnessMapName, metalnessMapName, aoMapName);
		m_models[i].worldRotation = glm::quat(glm::vec3(0.f, glm::pi<float>(), 0.f));
	}
}

void DeferredRenderer::createUniformBuffers()
{
	// host
	m_uCubeViews = reinterpret_cast<CubeMapCameraUniformBuffer *>(m_allUniformHostData.alloc(sizeof(CubeMapCameraUniformBuffer)));
	m_uTransMats = reinterpret_cast<TransMatsUniformBuffer *>(m_allUniformHostData.alloc(sizeof(TransMatsUniformBuffer)));
	m_uLightInfo = reinterpret_cast<LightingPassUniformBuffer *>(m_allUniformHostData.alloc(sizeof(LightingPassUniformBuffer)));
	m_uDisplayInfo = reinterpret_cast<DisplayInfoUniformBuffer *>(m_allUniformHostData.alloc(sizeof(DisplayInfoUniformBuffer)));

	for (auto &model : m_models)
	{
		model.uPerModelInfo = reinterpret_cast<PerModelUniformBuffer *>(m_allUniformHostData.alloc(sizeof(PerModelUniformBuffer)));
	}

	// device
	m_allUniformBuffer.size = m_allUniformHostData.size();
	m_allUniformBuffer.offset = 0;

	m_allUniformBuffer.buffer = m_vulkanManager.createBuffer(m_allUniformBuffer.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void DeferredRenderer::createDescriptorPools()
{
	m_vulkanManager.beginCreateDescriptorPool(8 + m_models.size());

	m_vulkanManager.descriptorPoolAddDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6 + 2 * m_models.size());
	m_vulkanManager.descriptorPoolAddDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 14 + m_models.size() * VMesh::numMapsPerMesh + m_bakedBRDFs.size());
	m_vulkanManager.descriptorPoolAddDescriptors(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4);
	m_vulkanManager.descriptorPoolAddDescriptors(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);

	m_descriptorPool = m_vulkanManager.endCreateDescriptorPool();
}

void DeferredRenderer::createDescriptorSets()
{
	m_vulkanManager.resetDescriptorPool(m_descriptorPool);

	// create descriptor sets
	std::vector<uint32_t> layouts;
	layouts.push_back(m_brdfLutDescriptorSetLayout);
	layouts.push_back(m_specEnvPrefilterDescriptorSetLayout);
	layouts.push_back(m_skyboxDescriptorSetLayout);
	layouts.push_back(m_lightingDescriptorSetLayout);
	layouts.push_back(m_finalOutputDescriptorSetLayout);
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		layouts.push_back(m_geomDescriptorSetLayout);
	}
	for (uint32_t i = 0; i < 3; ++i)
	{
		layouts.push_back(m_bloomDescriptorSetLayout);
	}

	std::vector<uint32_t> sets = m_vulkanManager.allocateDescriptorSets(m_descriptorPool, layouts);

	uint32_t idx = 0;
	m_brdfLutDescriptorSet = sets[idx++];
	m_specEnvPrefilterDescriptorSet = sets[idx++];
	m_skyboxDescriptorSet = sets[idx++];
	m_lightingDescriptorSet = sets[idx++];
	m_finalOutputDescriptorSet = sets[idx++];
	m_geomDescriptorSets.resize(m_models.size());
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		m_geomDescriptorSets[i] = sets[idx++];
	}
	m_bloomDescriptorSets.resize(3);
	for (uint32_t i = 0; i < 3; ++i)
	{
		m_bloomDescriptorSets[i] = sets[idx++];
	}

	// geometry pass descriptor set will be updated for every model
	// so there is no need to pre-initialize it
	createBrdfLutDescriptorSet();
	createSpecEnvPrefilterDescriptorSet();
	createGeomPassDescriptorSets();
	createLightingPassDescriptorSets();
	createBloomDescriptorSets();
	createFinalOutputPassDescriptorSets();
}

void DeferredRenderer::createFramebuffers()
{
	// Used in final output pass
	m_finalOutputFramebuffers = m_vulkanManager.createSwapChainFramebuffers(m_finalOutputRenderPass);

	// Diffuse irradiance map pass
	if (!m_skybox.diffMapReady)
	{
		m_diffEnvPrefilterFramebuffer = m_vulkanManager.createFramebuffer(m_specEnvPrefilterRenderPass, { m_skybox.diffuseIrradianceMap.imageViews[0] });
	}

	// Specular irradiance map pass
	if (!m_skybox.specMapReady)
	{
		m_specEnvPrefilterFramebuffers.resize(m_skybox.specularIrradianceMap.mipLevelCount);
		for (uint32_t level = 0; level < m_skybox.specularIrradianceMap.mipLevelCount; ++level)
		{
			m_specEnvPrefilterFramebuffers[level] =
				m_vulkanManager.createFramebuffer(m_specEnvPrefilterRenderPass, { m_skybox.specularIrradianceMap.imageViews[level + 1] });
		}
	}

	// Used in geometry and lighting pass
	{
		std::vector<uint32_t> attachmentViews =
		{
			m_depthImage.imageViews[0],
			m_gbufferImages[0].imageViews[0],
			m_gbufferImages[1].imageViews[0],
			m_gbufferImages[2].imageViews[0],
			m_lightingResultImage.imageViews[0]
		};

		m_geomAndLightingFramebuffer = m_vulkanManager.createFramebuffer(m_geomAndLightRenderPass, attachmentViews);
	}

	// Bloom
	{
		m_postEffectFramebuffers.resize(3);

		m_postEffectFramebuffers[0] = m_vulkanManager.createFramebuffer(m_bloomRenderPasses[0], { m_postEffectImages[0].imageViews[0] });
		m_postEffectFramebuffers[1] = m_vulkanManager.createFramebuffer(m_bloomRenderPasses[0], { m_postEffectImages[1].imageViews[0] });
		m_postEffectFramebuffers[2] = m_vulkanManager.createFramebuffer(m_bloomRenderPasses[1], { m_lightingResultImage.imageViews[0] });
	}
}

void DeferredRenderer::createCommandBuffers()
{
	// Allocate graphics command buffers
	m_vulkanManager.resetCommandPool(m_graphicsCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	m_presentCommandBuffers.resize(m_finalOutputFramebuffers.size());

	std::vector<uint32_t> commandBuffers = m_vulkanManager.allocateCommandBuffers(m_graphicsCommandPool,
		static_cast<uint32_t>(m_presentCommandBuffers.size()) + 3);

	int idx;
	for (idx = 0; idx < m_presentCommandBuffers.size(); ++idx)
	{
		m_presentCommandBuffers[idx] = commandBuffers[idx];
	}
	m_geomAndLightingCommandBuffer = commandBuffers[idx++];
	m_envPrefilterCommandBuffer = commandBuffers[idx++];
	m_postEffectCommandBuffer = commandBuffers[idx++];

	// Create command buffers for different purposes
	createEnvPrefilterCommandBuffer();
	createGeomAndLightingCommandBuffer();
	createPostEffectCommandBuffer();
	createPresentCommandBuffers();

	// compute command buffers
	m_vulkanManager.resetCommandPool(m_computeCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	commandBuffers = m_vulkanManager.allocateCommandBuffers(m_computeCommandPool, 1);
	m_brdfLutCommandBuffer = commandBuffers[0];

	createBrdfLutCommandBuffer();
}

void DeferredRenderer::createSynchronizationObjects()
{
	m_imageAvailableSemaphore = m_vulkanManager.createSemaphore();
	m_geomAndLightingCompleteSemaphore = m_vulkanManager.createSemaphore();
	m_postEffectSemaphore = m_vulkanManager.createSemaphore();
	m_finalOutputFinishedSemaphore = m_vulkanManager.createSemaphore();
	m_renderFinishedSemaphore = m_vulkanManager.createSemaphore();

	m_brdfLutFence = m_vulkanManager.createFence();
	m_envPrefilterFence = m_vulkanManager.createFence();
}

void DeferredRenderer::createSpecEnvPrefilterRenderPass()
{
	m_vulkanManager.beginCreateRenderPass();

	m_vulkanManager.renderPassAddAttachment(m_skybox.specularIrradianceMap.format,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	m_vulkanManager.renderPassAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	m_vulkanManager.renderPassAddSubpassDependency(0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

	m_specEnvPrefilterRenderPass = m_vulkanManager.endCreateRenderPass();
}

void DeferredRenderer::createGeometryAndLightingRenderPass()
{
	m_vulkanManager.beginCreateRenderPass();

	// --- Attachments used in this render pass
	// Depth
	// Clear only happens in the FIRST subpass that uses this attachment
	// VK_IMAGE_LAYOUT_UNDEFINED as initial layout means that we don't care about the initial layout of this attachment image (content may not be preserved)
	m_vulkanManager.renderPassAddAttachment(m_depthImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// World space normal + albedo
	// Normal has been perturbed by normal mapping
	m_vulkanManager.renderPassAddAttachment(m_gbufferImages[0].format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// World postion
	m_vulkanManager.renderPassAddAttachment(m_gbufferImages[1].format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// RMAI
	m_vulkanManager.renderPassAddAttachment(m_gbufferImages[2].format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Lighting result
	m_vulkanManager.renderPassAddAttachment(m_lightingResultImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// --- Reference to render pass attachments used in each subpass
	// --- Subpasses
	// Geometry subpass
	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.subpassAddColorAttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.subpassAddColorAttachmentReference(3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.subpassAddDepthAttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	// Lighting subpass
	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.subpassAddInputAttachmentReference(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_vulkanManager.subpassAddInputAttachmentReference(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_vulkanManager.subpassAddInputAttachmentReference(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_vulkanManager.subpassAddInputAttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	m_vulkanManager.subpassAddDepthAttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	// --- Subpass dependencies
	m_vulkanManager.renderPassAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	m_vulkanManager.renderPassAddSubpassDependency(0, 1,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

	m_vulkanManager.renderPassAddSubpassDependency(1, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	// --- Create render pass
	m_geomAndLightRenderPass = m_vulkanManager.endCreateRenderPass();
}

void DeferredRenderer::createBloomRenderPasses()
{
	m_bloomRenderPasses.resize(2);

	// --- Bloom render pass 1 (brightness and blur passes): will clear framebuffer
	m_vulkanManager.beginCreateRenderPass();

	m_vulkanManager.renderPassAddAttachment(m_postEffectImages[0].format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	m_vulkanManager.renderPassAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

	m_bloomRenderPasses[0] = m_vulkanManager.endCreateRenderPass();

	// --- Bloom render pass 2 (Merge pass): will not clear framebuffer
	m_vulkanManager.beginCreateRenderPass();

	m_vulkanManager.renderPassAddAttachment(m_lightingResultImage.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD);

	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	m_vulkanManager.renderPassAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

	m_bloomRenderPasses[1] = m_vulkanManager.endCreateRenderPass();
}

void DeferredRenderer::createFinalOutputRenderPass()
{
	m_vulkanManager.beginCreateRenderPass();

	m_vulkanManager.renderPassAddAttachment(m_vulkanManager.getSwapChainImageFormat(),
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	m_vulkanManager.beginDescribeSubpass();
	m_vulkanManager.subpassAddColorAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_vulkanManager.endDescribeSubpass();

	m_vulkanManager.renderPassAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	m_finalOutputRenderPass = m_vulkanManager.endCreateRenderPass();
}

void DeferredRenderer::createBrdfLutDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	m_brdfLutDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createSpecEnvPrefilterDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// 6 View matrices + projection matrix
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);

	// HDR probe a.k.a. radiance environment map with mips
	m_vulkanManager.setLayoutAddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_specEnvPrefilterDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createGeomPassDescriptorSetLayout()
{
	createStaticMeshDescriptorSetLayout();
	createSkyboxDescriptorSetLayout();
}

void DeferredRenderer::createSkyboxDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// Transformation matrices
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	// Albedo map
	m_vulkanManager.setLayoutAddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_skyboxDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createStaticMeshDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// Transformation matrices
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	// Per model information
	m_vulkanManager.setLayoutAddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	// Albedo map
	m_vulkanManager.setLayoutAddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// Normal map
	m_vulkanManager.setLayoutAddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// Roughness map
	m_vulkanManager.setLayoutAddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// Metalness map
	m_vulkanManager.setLayoutAddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// AO map
	m_vulkanManager.setLayoutAddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_geomDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createLightingPassDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// Light information
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// gbuffer 1
	m_vulkanManager.setLayoutAddBinding(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);

	// gbuffer 2
	m_vulkanManager.setLayoutAddBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);

	// gbuffer 3
	m_vulkanManager.setLayoutAddBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);

	// depth image
	m_vulkanManager.setLayoutAddBinding(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);

	// diffuse irradiance map
	m_vulkanManager.setLayoutAddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// specular irradiance map (prefiltered environment map)
	m_vulkanManager.setLayoutAddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// BRDF LUT
	m_vulkanManager.setLayoutAddBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_lightingDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createBloomDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// input image
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_bloomDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createFinalOutputDescriptorSetLayout()
{
	m_vulkanManager.beginCreateDescriptorSetLayout();

	// Final image, gbuffer, depth image
	m_vulkanManager.setLayoutAddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_vulkanManager.setLayoutAddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_vulkanManager.setLayoutAddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_vulkanManager.setLayoutAddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_vulkanManager.setLayoutAddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// Uniform buffer
	m_vulkanManager.setLayoutAddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_finalOutputDescriptorSetLayout = m_vulkanManager.endCreateDescriptorSetLayout();
}

void DeferredRenderer::createBrdfLutPipeline()
{
	const std::string csFileName = "../shaders/brdf_lut_pass/brdf_lut.comp.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_brdfLutDescriptorSetLayout });
	m_brdfLutPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateComputePipeline(m_brdfLutPipelineLayout);
	m_vulkanManager.computePipelineAddShaderStage(csFileName);
	m_brdfLutPipeline = m_vulkanManager.endCreateComputePipeline();
}

void DeferredRenderer::createDiffEnvPrefilterPipeline()
{
	const std::string vsFileName = "../shaders/env_prefilter_pass/env_prefilter.vert.spv";
	const std::string gsFileName = "../shaders/env_prefilter_pass/env_prefilter.geom.spv";
	const std::string fsFileName = "../shaders/env_prefilter_pass/diff_env_prefilter.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_specEnvPrefilterDescriptorSetLayout });
	m_diffEnvPrefilterPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_diffEnvPrefilterPipelineLayout, m_specEnvPrefilterRenderPass, 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, gsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	auto bindingDesc = Vertex::getBindingDescription();
	m_vulkanManager.graphicsPipelineAddBindingDescription(bindingDesc.binding, bindingDesc.stride, bindingDesc.inputRate);
	auto attrDescs = Vertex::getAttributeDescriptions();
	for (const auto &attrDesc : attrDescs)
	{
		m_vulkanManager.graphicsPipelineAddAttributeDescription(attrDesc.location, attrDesc.binding, attrDesc.format, attrDesc.offset);
	}

	m_vulkanManager.graphicsPipelineConfigureRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_vulkanManager.graphicsPipelineAddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	m_vulkanManager.graphicsPipelineAddDynamicState(VK_DYNAMIC_STATE_SCISSOR);

	m_diffEnvPrefilterPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createSpecEnvPrefilterPipeline()
{
	const std::string vsFileName = "../shaders/env_prefilter_pass/env_prefilter.vert.spv";
	const std::string gsFileName = "../shaders/env_prefilter_pass/env_prefilter.geom.spv";
	const std::string fsFileName = "../shaders/env_prefilter_pass/spec_env_prefilter.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_specEnvPrefilterDescriptorSetLayout });
	m_vulkanManager.pipelineLayoutAddPushConstantRange(0, sizeof(float), VK_SHADER_STAGE_FRAGMENT_BIT);
	m_specEnvPrefilterPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_specEnvPrefilterPipelineLayout, m_specEnvPrefilterRenderPass, 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, gsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	auto bindingDesc = Vertex::getBindingDescription();
	m_vulkanManager.graphicsPipelineAddBindingDescription(bindingDesc.binding, bindingDesc.stride, bindingDesc.inputRate);
	auto attrDescs = Vertex::getAttributeDescriptions();
	for (const auto &attrDesc : attrDescs)
	{
		m_vulkanManager.graphicsPipelineAddAttributeDescription(attrDesc.location, attrDesc.binding, attrDesc.format, attrDesc.offset);
	}

	m_vulkanManager.graphicsPipelineConfigureRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_vulkanManager.graphicsPipelineAddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	m_vulkanManager.graphicsPipelineAddDynamicState(VK_DYNAMIC_STATE_SCISSOR);

	m_specEnvPrefilterPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createGeomPassPipeline()
{
	createSkyboxPipeline();
	createStaticMeshPipeline();
}

void DeferredRenderer::createSkyboxPipeline()
{
	const std::string vsFileName = "../shaders/geom_pass/skybox.vert.spv";
	const std::string fsFileName = "../shaders/geom_pass/skybox.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_skyboxDescriptorSetLayout });
	m_vulkanManager.pipelineLayoutAddPushConstantRange(0, sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);
	m_skyboxPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_skyboxPipelineLayout, m_geomAndLightRenderPass, 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	auto bindingDesc = Vertex::getBindingDescription();
	m_vulkanManager.graphicsPipelineAddBindingDescription(bindingDesc.binding, bindingDesc.stride, bindingDesc.inputRate);
	auto attrDescs = Vertex::getAttributeDescriptions();
	for (const auto &attrDesc : attrDescs)
	{
		m_vulkanManager.graphicsPipelineAddAttributeDescription(attrDesc.location, attrDesc.binding, attrDesc.format, attrDesc.offset);
	}

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);
	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);
	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_skyboxPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createStaticMeshPipeline()
{
	const std::string vsFileName = "../shaders/geom_pass/geom.vert.spv";
	const std::string fsFileName = "../shaders/geom_pass/geom.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_geomDescriptorSetLayout });
	m_vulkanManager.pipelineLayoutAddPushConstantRange(0, 2 * sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);
	m_geomPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_geomPipelineLayout, m_geomAndLightRenderPass, 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	auto bindingDesc = Vertex::getBindingDescription();
	m_vulkanManager.graphicsPipelineAddBindingDescription(bindingDesc.binding, bindingDesc.stride, bindingDesc.inputRate);
	auto attrDescs = Vertex::getAttributeDescriptions();
	for (const auto &attrDesc : attrDescs)
	{
		m_vulkanManager.graphicsPipelineAddAttributeDescription(attrDesc.location, attrDesc.binding, attrDesc.format, attrDesc.offset);
	}

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);
	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);
	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_geomPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createLightingPassPipeline()
{
	const std::string vsFileName = "../shaders/fullscreen.vert.spv";
	const std::string fsFileName = "../shaders/lighting_pass/lighting.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_lightingDescriptorSetLayout });
	m_vulkanManager.pipelineLayoutAddPushConstantRange(0, sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);
	m_lightingPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_lightingPipelineLayout, m_geomAndLightRenderPass, 1);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	// Use specialization constants to pass number of lights to the shader
	uint32_t specializationData = NUM_LIGHTS;
	m_vulkanManager.graphicsPipelineAddSpecializationConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0, sizeof(uint32_t), &specializationData);
	
	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_ALWAYS); // TODO: reenable it when proxy is implemented

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_lightingPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createBloomPipelines()
{
	const std::string vsFileName = "../shaders/fullscreen.vert.spv";
	const std::string fsFileName1 = "../shaders/bloom_pass/brightness_mask.frag.spv";
	const std::string fsFileName2 = "../shaders/bloom_pass/gaussian_blur.frag.spv";
	const std::string fsFileName3 = "../shaders/bloom_pass/merge.frag.spv";

	// --- Pipeline layouts
	m_bloomPipelineLayouts.resize(2);

	// brightness_mask and merge
	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_bloomDescriptorSetLayout });
	m_bloomPipelineLayouts[0] = m_vulkanManager.endCreatePipelineLayout();

	// gaussian blur
	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_bloomDescriptorSetLayout });
	m_vulkanManager.pipelineLayoutAddPushConstantRange(0, sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);
	m_bloomPipelineLayouts[1] = m_vulkanManager.endCreatePipelineLayout();

	// --- Pipelines
	m_bloomPipelines.resize(3);

	// brightness mask
	m_vulkanManager.beginCreateGraphicsPipeline(m_bloomPipelineLayouts[0], m_bloomRenderPasses[0], 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName1);

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_bloomPipelines[0] = m_vulkanManager.endCreateGraphicsPipeline();

	// gaussian blur
	m_vulkanManager.beginCreateGraphicsPipeline(m_bloomPipelineLayouts[1], m_bloomRenderPasses[0], 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName2);

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_bloomPipelines[1] = m_vulkanManager.endCreateGraphicsPipeline();

	// merge
	m_vulkanManager.beginCreateGraphicsPipeline(m_bloomPipelineLayouts[0], m_bloomRenderPasses[1], 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName3);

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_TRUE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD);

	m_bloomPipelines[2] = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createFinalOutputPassPipeline()
{
	const std::string vsFileName = "../shaders/fullscreen.vert.spv";
	const std::string fsFileName = "../shaders/final_output_pass/final_output.frag.spv";

	m_vulkanManager.beginCreatePipelineLayout();
	m_vulkanManager.pipelineLayoutAddDescriptorSetLayouts({ m_finalOutputDescriptorSetLayout });
	m_finalOutputPipelineLayout = m_vulkanManager.endCreatePipelineLayout();

	m_vulkanManager.beginCreateGraphicsPipeline(m_finalOutputPipelineLayout, m_finalOutputRenderPass, 0);

	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vsFileName);
	m_vulkanManager.graphicsPipelineAddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fsFileName);

	VkExtent2D swapChainExtent = m_vulkanManager.getSwapChainExtent();
	m_vulkanManager.graphicsPipelineAddViewportAndScissor(0.f, 0.f,
		static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));

	m_vulkanManager.graphicsPipelineConfigureDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	m_vulkanManager.graphicsPipeLineAddColorBlendAttachment(VK_FALSE);

	m_finalOutputPipeline = m_vulkanManager.endCreateGraphicsPipeline();
}

void DeferredRenderer::createBrdfLutDescriptorSet()
{
	if (m_bakedBrdfReady) return;

	std::vector<rj::DescriptorSetUpdateImageInfo> updateInfos(1);
	updateInfos[0].layout = VK_IMAGE_LAYOUT_GENERAL;
	updateInfos[0].imageViewName = m_bakedBRDFs[0].imageViews[0];
	updateInfos[0].samplerName = std::numeric_limits<uint32_t>::max();

	m_vulkanManager.beginUpdateDescriptorSet(m_brdfLutDescriptorSet);
	m_vulkanManager.descriptorSetAddImageDescriptor(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, updateInfos);
	m_vulkanManager.endUpdateDescriptorSet();
}

void DeferredRenderer::createSpecEnvPrefilterDescriptorSet()
{
	if (m_skybox.diffMapReady && m_skybox.specMapReady) return;

	std::vector<rj::DescriptorSetUpdateBufferInfo> bufferInfos(1);
	bufferInfos[0].bufferName = m_allUniformBuffer.buffer;
	bufferInfos[0].offset = m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_uCubeViews));
	bufferInfos[0].sizeInBytes = sizeof(CubeMapCameraUniformBuffer);

	std::vector<rj::DescriptorSetUpdateImageInfo> imageInfos(1);
	imageInfos[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[0].imageViewName = m_skybox.radianceMap.imageViews[0];
	imageInfos[0].samplerName = m_skybox.radianceMap.samplers[0];

	m_vulkanManager.beginUpdateDescriptorSet(m_specEnvPrefilterDescriptorSet);
	m_vulkanManager.descriptorSetAddBufferDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfos);
	m_vulkanManager.descriptorSetAddImageDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);
	m_vulkanManager.endUpdateDescriptorSet();
}

void DeferredRenderer::createGeomPassDescriptorSets()
{
	createSkyboxDescriptorSet();
	createStaticMeshDescriptorSet();
}

void DeferredRenderer::createSkyboxDescriptorSet()
{
	std::vector<rj::DescriptorSetUpdateBufferInfo> bufferInfos(1);
	bufferInfos[0].bufferName = m_allUniformBuffer.buffer;
	bufferInfos[0].offset = m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_uTransMats));
	bufferInfos[0].sizeInBytes = sizeof(DisplayInfoUniformBuffer);

	std::vector<rj::DescriptorSetUpdateImageInfo> imageInfos(1);
	imageInfos[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[0].imageViewName = m_skybox.radianceMap.imageViews[0];
	imageInfos[0].samplerName = m_skybox.radianceMap.samplers[0];

	m_vulkanManager.beginUpdateDescriptorSet(m_skyboxDescriptorSet);
	m_vulkanManager.descriptorSetAddBufferDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfos);
	m_vulkanManager.descriptorSetAddImageDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);
	m_vulkanManager.endUpdateDescriptorSet();
}

void DeferredRenderer::createStaticMeshDescriptorSet()
{
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		std::vector<rj::DescriptorSetUpdateBufferInfo> bufferInfos(1);
		std::vector<rj::DescriptorSetUpdateImageInfo> imageInfos(1);

		m_vulkanManager.beginUpdateDescriptorSet(m_geomDescriptorSets[i]);

		bufferInfos[0].bufferName = m_allUniformBuffer.buffer;
		bufferInfos[0].offset = m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_uTransMats));
		bufferInfos[0].sizeInBytes = sizeof(TransMatsUniformBuffer);
		m_vulkanManager.descriptorSetAddBufferDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfos);

		bufferInfos[0].offset = m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_models[i].uPerModelInfo));
		bufferInfos[0].sizeInBytes = sizeof(PerModelUniformBuffer);
		m_vulkanManager.descriptorSetAddBufferDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfos);

		imageInfos[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[0].imageViewName = m_models[i].albedoMap.imageViews[0];
		imageInfos[0].samplerName = m_models[i].albedoMap.samplers[0];
		m_vulkanManager.descriptorSetAddImageDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		imageInfos[0].imageViewName = m_models[i].normalMap.imageViews[0];
		imageInfos[0].samplerName = m_models[i].normalMap.samplers[0];
		m_vulkanManager.descriptorSetAddImageDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		imageInfos[0].imageViewName = m_models[i].roughnessMap.imageViews[0];
		imageInfos[0].samplerName = m_models[i].roughnessMap.samplers[0];
		m_vulkanManager.descriptorSetAddImageDescriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		imageInfos[0].imageViewName = m_models[i].metalnessMap.imageViews[0];
		imageInfos[0].samplerName = m_models[i].metalnessMap.samplers[0];
		m_vulkanManager.descriptorSetAddImageDescriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		imageInfos[0].imageViewName = m_models[i].aoMap.image == std::numeric_limits<uint32_t>::max() ? m_models[i].albedoMap.imageViews[0] : m_models[i].aoMap.imageViews[0];
		imageInfos[0].samplerName = m_models[i].aoMap.image == std::numeric_limits<uint32_t>::max() ? m_models[i].albedoMap.samplers[0] : m_models[i].aoMap.samplers[0];
		m_vulkanManager.descriptorSetAddImageDescriptor(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		m_vulkanManager.endUpdateDescriptorSet();
	}
}

void DeferredRenderer::createLightingPassDescriptorSets()
{
	std::vector<rj::DescriptorSetUpdateBufferInfo> bufferInfos(1);
	std::vector<rj::DescriptorSetUpdateImageInfo> imageInfos(1);

	m_vulkanManager.beginUpdateDescriptorSet(m_lightingDescriptorSet);

	bufferInfos[0].bufferName = m_allUniformBuffer.buffer;
	bufferInfos[0].offset = m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_uLightInfo));
	bufferInfos[0].sizeInBytes = sizeof(LightingPassUniformBuffer);
	m_vulkanManager.descriptorSetAddBufferDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfos);

	imageInfos[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[0].imageViewName = m_gbufferImages[0].imageViews[0];
	imageInfos[0].samplerName = m_gbufferImages[0].samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, imageInfos);

	imageInfos[0].imageViewName = m_gbufferImages[1].imageViews[0];
	imageInfos[0].samplerName = m_gbufferImages[1].samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, imageInfos);

	imageInfos[0].imageViewName = m_gbufferImages[2].imageViews[0];
	imageInfos[0].samplerName = m_gbufferImages[2].samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, imageInfos);

	imageInfos[0].imageViewName = m_depthImage.imageViews[0];
	imageInfos[0].samplerName = m_depthImage.samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, imageInfos);

	imageInfos[0].imageViewName = m_skybox.diffuseIrradianceMap.imageViews[0];
	imageInfos[0].samplerName = m_skybox.diffuseIrradianceMap.samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

	imageInfos[0].imageViewName = m_skybox.specularIrradianceMap.imageViews[0];
	imageInfos[0].samplerName = m_skybox.specularIrradianceMap.samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

	imageInfos[0].imageViewName = m_bakedBRDFs[0].imageViews[0];
	imageInfos[0].samplerName = m_bakedBRDFs[0].samplers[0];
	m_vulkanManager.descriptorSetAddImageDescriptor(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

	m_vulkanManager.endUpdateDescriptorSet();
}

void DeferredRenderer::createBloomDescriptorSets()
{
	std::array<VkDescriptorImageInfo, 3> imageInfos =
	{
		m_lightingResultImage.getDescriptorInfo(),
		m_postEffectImages[0].getDescriptorInfo(),
		m_postEffectImages[1].getDescriptorInfo()
	};

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_bloomDescriptorSets[0];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &imageInfos[0];

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = m_bloomDescriptorSets[1];
	descriptorWrites[1].dstBinding = 0;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfos[1];

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = m_bloomDescriptorSets[2];
	descriptorWrites[2].dstBinding = 0;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &imageInfos[2];

	vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DeferredRenderer::createFinalOutputPassDescriptorSets()
{
	VkDescriptorBufferInfo bufferInfo = m_allUniformBuffer.getDescriptorInfo(
		m_allUniformHostData.offsetOf(reinterpret_cast<const char *>(m_uDisplayInfo)),
		sizeof(DisplayInfoUniformBuffer));

	std::array<VkDescriptorImageInfo, 5> imageInfos;
	imageInfos[0] = m_lightingResultImage.getDescriptorInfo();
	imageInfos[1] = m_gbufferImages[0].getDescriptorInfo();
	imageInfos[2] = m_gbufferImages[1].getDescriptorInfo();
	imageInfos[3] = m_gbufferImages[2].getDescriptorInfo();
	imageInfos[4] = m_depthImage.getDescriptorInfo();

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_finalOutputDescriptorSet;
	descriptorWrites[0].dstBinding = 5;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = m_finalOutputDescriptorSet;
	descriptorWrites[1].dstBinding = 0;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = static_cast<uint32_t>(imageInfos.size());
	descriptorWrites[1].pImageInfo = imageInfos.data();

	vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DeferredRenderer::createBrdfLutCommandBuffer()
{
	if (m_bakedBrdfReady) return;

	vkQueueWaitIdle(m_computeQueue);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(m_brdfLutCommandBuffer, &beginInfo);

	vkCmdBindPipeline(m_brdfLutCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_brdfLutPipeline);
	vkCmdBindDescriptorSets(m_brdfLutCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_brdfLutPipelineLayout, 0, 1, &m_brdfLutDescriptorSet, 0, nullptr);

	const uint32_t bsx = 16, bsy = 16;
	const uint32_t brdfLutSizeX = BRDF_LUT_SIZE, brdfLutSizeY = BRDF_LUT_SIZE;
	vkCmdDispatch(m_brdfLutCommandBuffer, brdfLutSizeX / bsx, brdfLutSizeY / bsy, 1);

	if (vkEndCommandBuffer(m_brdfLutCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void DeferredRenderer::createEnvPrefilterCommandBuffer()
{
	if (m_skybox.diffMapReady && m_skybox.specMapReady) return;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(m_envPrefilterCommandBuffer, &beginInfo);

	VkBuffer vertexBuffers[] = { m_skybox.vertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(m_envPrefilterCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_envPrefilterCommandBuffer, m_skybox.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_specEnvPrefilterRenderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { { 0.f, 0.f, 0.f, 0.f } };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport = {};
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {};

	// Layered rendering allows render into multiple layers but only one mip level
	// In order to render all mips, multiple passes are required
	// Diffuse prefilter pass
	renderPassInfo.framebuffer = m_diffEnvPrefilterFramebuffer;
	renderPassInfo.renderArea.extent = { DIFF_IRRADIANCE_MAP_SIZE, DIFF_IRRADIANCE_MAP_SIZE };

	vkCmdBeginRenderPass(m_envPrefilterCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_envPrefilterCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_diffEnvPrefilterPipeline);
	vkCmdBindDescriptorSets(m_envPrefilterCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_diffEnvPrefilterPipelineLayout, 0, 1, &m_specEnvPrefilterDescriptorSet, 0, nullptr);

	viewport.width = DIFF_IRRADIANCE_MAP_SIZE;
	viewport.height = DIFF_IRRADIANCE_MAP_SIZE;
	vkCmdSetViewport(m_envPrefilterCommandBuffer, 0, 1, &viewport);

	scissor.extent = { DIFF_IRRADIANCE_MAP_SIZE, DIFF_IRRADIANCE_MAP_SIZE };
	vkCmdSetScissor(m_envPrefilterCommandBuffer, 0, 1, &scissor);

	vkCmdDrawIndexed(m_envPrefilterCommandBuffer, static_cast<uint32_t>(m_skybox.indexBuffer.numElements), 1, 0, 0, 0);

	vkCmdEndRenderPass(m_envPrefilterCommandBuffer);

	// Specular prefitler pass
	uint32_t mipLevels = m_skybox.specularIrradianceMap.mipLevels;
	float roughness = 0.f;
	float roughnessDelta = 1.f / static_cast<float>(mipLevels - 1);

	for (uint32_t level = 0; level < mipLevels; ++level)
	{
		uint32_t faceWidth = SPEC_IRRADIANCE_MAP_SIZE / (1 << level);
		uint32_t faceHeight = faceWidth;

		renderPassInfo.framebuffer = m_specEnvPrefilterFramebuffers[level];
		renderPassInfo.renderArea.extent = { faceWidth, faceHeight };

		vkCmdBeginRenderPass(m_envPrefilterCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_envPrefilterCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_specEnvPrefilterPipeline);

		vkCmdBindDescriptorSets(m_envPrefilterCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_specEnvPrefilterPipelineLayout, 0, 1, &m_specEnvPrefilterDescriptorSet, 0, nullptr);
		vkCmdPushConstants(m_envPrefilterCommandBuffer, m_specEnvPrefilterPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &roughness);

		viewport.width = faceWidth;
		viewport.height = faceHeight;
		vkCmdSetViewport(m_envPrefilterCommandBuffer, 0, 1, &viewport);

		scissor.extent = { faceWidth, faceHeight };
		vkCmdSetScissor(m_envPrefilterCommandBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(m_envPrefilterCommandBuffer, static_cast<uint32_t>(m_skybox.indexBuffer.numElements), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_envPrefilterCommandBuffer);

		roughness += roughnessDelta;
	}

	if (vkEndCommandBuffer(m_envPrefilterCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void DeferredRenderer::createGeomAndLightingCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(m_geomAndLightingCommandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_geomAndLightRenderPass;
	renderPassInfo.framebuffer = m_geomAndLightingFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChain.swapChainExtent;

	std::array<VkClearValue, 5> clearValues = {};
	clearValues[0].depthStencil = { 1.0f, 0 };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // g-buffer 1
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // g-buffer 2
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // g-buffer 3
	clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // lighting result

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_geomAndLightingCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Geometry pass
	{
		vkCmdBindPipeline(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline);

		VkBuffer vertexBuffers[] = { m_skybox.vertexBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(m_geomAndLightingCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_geomAndLightingCommandBuffer, m_skybox.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipelineLayout, 0, 1, &m_skyboxDescriptorSet, 0, nullptr);
		vkCmdPushConstants(m_geomAndLightingCommandBuffer, m_skyboxPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &m_skybox.materialType);

		vkCmdDrawIndexed(m_geomAndLightingCommandBuffer, static_cast<uint32_t>(m_skybox.indexBuffer.numElements), 1, 0, 0, 0);
	}

	vkCmdBindPipeline(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_geomPipeline);

	for (uint32_t j = 0; j < m_models.size(); ++j)
	{
		VkBuffer vertexBuffers[] = { m_models[j].vertexBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(m_geomAndLightingCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_geomAndLightingCommandBuffer, m_models[j].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_geomPipelineLayout, 0, 1, &m_geomDescriptorSets[j], 0, nullptr);

		struct
		{
			uint32_t materialId;
			uint32_t hasAoMap;
		} pushConst;
		pushConst.materialId = m_models[j].materialType;
		pushConst.hasAoMap = m_models[j].aoMap.image.isvalid();

		vkCmdPushConstants(m_geomAndLightingCommandBuffer, m_geomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConst), &pushConst);

		vkCmdDrawIndexed(m_geomAndLightingCommandBuffer, static_cast<uint32_t>(m_models[j].indexBuffer.numElements), 1, 0, 0, 0);
	}

	// Lighting pass
	vkCmdNextSubpass(m_geomAndLightingCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPipeline);
	vkCmdBindDescriptorSets(m_geomAndLightingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPipelineLayout, 0, 1, &m_lightingDescriptorSet, 0, nullptr);
	vkCmdPushConstants(m_geomAndLightingCommandBuffer, m_lightingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof(uint32_t), &m_skybox.specularIrradianceMap.mipLevels);

	vkCmdDraw(m_geomAndLightingCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_geomAndLightingCommandBuffer);

	if (vkEndCommandBuffer(m_geomAndLightingCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void DeferredRenderer::createPostEffectCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(m_postEffectCommandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_bloomRenderPasses[0]; // clear
	renderPassInfo.framebuffer = m_postEffectFramebuffers[0];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChain.swapChainExtent;

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// brightness mask
	vkCmdBeginRenderPass(m_postEffectCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelines[0]);
	vkCmdBindDescriptorSets(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelineLayouts[0], 0, 1, &m_bloomDescriptorSets[0], 0, nullptr);

	vkCmdDraw(m_postEffectCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_postEffectCommandBuffer);

	// gaussian blur
	for (uint32_t i = 0; i < 5; ++i)
	{
		// horizontal
		renderPassInfo.framebuffer = m_postEffectFramebuffers[1];

		vkCmdBeginRenderPass(m_postEffectCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelines[1]);
		vkCmdBindDescriptorSets(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelineLayouts[1], 0, 1, &m_bloomDescriptorSets[1], 0, nullptr);

		uint32_t isHorizontal = VK_TRUE;
		vkCmdPushConstants(m_postEffectCommandBuffer, m_bloomPipelineLayouts[1], VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(uint32_t), &isHorizontal);

		vkCmdDraw(m_postEffectCommandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(m_postEffectCommandBuffer);

		// vertical
		renderPassInfo.framebuffer = m_postEffectFramebuffers[0];

		vkCmdBeginRenderPass(m_postEffectCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelines[1]);
		vkCmdBindDescriptorSets(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelineLayouts[1], 0, 1, &m_bloomDescriptorSets[2], 0, nullptr);

		isHorizontal = VK_FALSE;
		vkCmdPushConstants(m_postEffectCommandBuffer, m_bloomPipelineLayouts[1], VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(uint32_t), &isHorizontal);

		vkCmdDraw(m_postEffectCommandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(m_postEffectCommandBuffer);
	}

	// merge
	renderPassInfo.renderPass = m_bloomRenderPasses[1]; // no clear
	renderPassInfo.framebuffer = m_postEffectFramebuffers[2]; // lighting result image
	renderPassInfo.clearValueCount = 0;
	renderPassInfo.pClearValues = nullptr;

	vkCmdBeginRenderPass(m_postEffectCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelines[2]);
	vkCmdBindDescriptorSets(m_postEffectCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomPipelineLayouts[0], 0, 1, &m_bloomDescriptorSets[1], 0, nullptr);

	vkCmdDraw(m_postEffectCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_postEffectCommandBuffer);

	if (vkEndCommandBuffer(m_postEffectCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void DeferredRenderer::createPresentCommandBuffers()
{
	// Final ouput pass
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	for (uint32_t i = 0; i < m_presentCommandBuffers.size(); ++i)
	{
		vkBeginCommandBuffer(m_presentCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_finalOutputRenderPass;
		renderPassInfo.framebuffer = m_finalOutputFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChain.swapChainExtent;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_presentCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Geometry pass
		vkCmdBindPipeline(m_presentCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_finalOutputPipeline);
		vkCmdBindDescriptorSets(m_presentCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_finalOutputPipelineLayout, 0, 1, &m_finalOutputDescriptorSet, 0, nullptr);
		vkCmdDraw(m_presentCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_presentCommandBuffers[i]);

		if (vkEndCommandBuffer(m_presentCommandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void DeferredRenderer::prefilterEnvironmentAndComputeBrdfLut()
{
	// References:
	//   [1] http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
	//   [2] https://github.com/derkreature/IBLBaker

	// Set up cube map camera
	updateUniformBuffers();

	// Bake BRDF terms
	std::vector<VkFence> fences;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_brdfLutCommandBuffer;
	
	auto tStart = std::chrono::high_resolution_clock::now();

	if (!m_bakedBrdfReady)
	{
		if (vkQueueSubmit(m_computeQueue, 1, &submitInfo, m_brdfLutFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit brdf lut command buffer.");
		}
		fences.push_back(m_brdfLutFence);
	}

	// Prefilter radiance map
	submitInfo.pCommandBuffers = &m_envPrefilterCommandBuffer;

	if (!m_skybox.diffMapReady || !m_skybox.specMapReady)
	{
		if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_envPrefilterFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit environment prefilter command buffer.");
		}
		fences.push_back(m_envPrefilterFence);
	}

	if (fences.size() > 0)
	{
		vkWaitForFences(m_device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(m_device, static_cast<uint32_t>(fences.size()), fences.data());

		if (!m_bakedBrdfReady)
		{
			transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
				m_bakedBRDFs[0].image, m_bakedBRDFs[0].format, m_bakedBRDFs[0].mipLevels,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_bakedBrdfReady = true;
		}

		if (!m_skybox.diffMapReady)
		{
			transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
				m_skybox.diffuseIrradianceMap.image, m_skybox.diffuseIrradianceMap.format, 6, m_skybox.diffuseIrradianceMap.mipLevels,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		if (!m_skybox.specMapReady)
		{
			transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
				m_skybox.specularIrradianceMap.image, m_skybox.specularIrradianceMap.format, 6, m_skybox.specularIrradianceMap.mipLevels,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		auto tEnd = std::chrono::high_resolution_clock::now();
		std::cout << "Took " << std::chrono::duration<float, std::milli>(tEnd - tStart).count() << "ms to do precalculation.\n";
	}
}

void DeferredRenderer::savePrecomputationResults()
{
	// read back computation results and save to disk
	if (m_shouldSaveBakedBrdf)
	{
		transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
			m_bakedBRDFs[0].image, m_bakedBRDFs[0].format, m_bakedBRDFs[0].mipLevels,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		BufferWrapper stagingBuffer{ m_device };
		VkDeviceSize sizeInBytes = BRDF_LUT_SIZE * BRDF_LUT_SIZE * sizeof(glm::vec2);

		createBuffer(m_physicalDevice, m_device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer.buffer, stagingBuffer.bufferMemory);

		VkBufferImageCopy imageCopyRegion = {};
		imageCopyRegion.imageExtent = { BRDF_LUT_SIZE, BRDF_LUT_SIZE, 1 };
		imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.imageSubresource.layerCount = 1;
		imageCopyRegion.imageSubresource.mipLevel = 0;

		// Tiling mode difference is handled by Vulkan automatically
		copyImageToBuffer(m_device, m_graphicsCommandPool, m_graphicsQueue, { imageCopyRegion }, m_bakedBRDFs[0].image, stagingBuffer.buffer);

		void* data;
		std::vector<glm::vec2> hostBrdfLutPixels(BRDF_LUT_SIZE * BRDF_LUT_SIZE);
		vkMapMemory(m_device, stagingBuffer.bufferMemory, 0, sizeInBytes, 0, &data);
		memcpy(&hostBrdfLutPixels[0], data, sizeInBytes);
		vkUnmapMemory(m_device, stagingBuffer.bufferMemory);

		saveImage2D(BRDF_BASE_DIR BRDF_NAME,
			BRDF_LUT_SIZE, BRDF_LUT_SIZE, sizeof(glm::vec2), 1, gli::FORMAT_RG32_SFLOAT_PACK32, hostBrdfLutPixels.data());
	}

	if (m_skybox.shouldSaveDiffMap)
	{
		transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
			m_skybox.diffuseIrradianceMap.image, m_skybox.diffuseIrradianceMap.format, 6, m_skybox.diffuseIrradianceMap.mipLevels,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		BufferWrapper stagingBuffer{ m_device };
		VkDeviceSize sizeInBytes = compute2DImageSizeInBytes(
			DIFF_IRRADIANCE_MAP_SIZE, DIFF_IRRADIANCE_MAP_SIZE, sizeof(glm::vec4), m_skybox.diffuseIrradianceMap.mipLevels, 6);

		createBuffer(m_physicalDevice, m_device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer.buffer, stagingBuffer.bufferMemory);

		VkBufferImageCopy imageCopyRegion = {};
		imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.imageSubresource.mipLevel = 0;
		imageCopyRegion.imageSubresource.baseArrayLayer = 0;
		imageCopyRegion.imageSubresource.layerCount = 6;
		imageCopyRegion.imageExtent.width = DIFF_IRRADIANCE_MAP_SIZE;
		imageCopyRegion.imageExtent.height = DIFF_IRRADIANCE_MAP_SIZE;
		imageCopyRegion.imageExtent.depth = 1;
		imageCopyRegion.bufferOffset = 0;

		copyImageToBuffer(m_device, m_graphicsCommandPool, m_graphicsQueue, { imageCopyRegion }, m_skybox.diffuseIrradianceMap.image, stagingBuffer.buffer);

		void* data;
		std::vector<glm::vec4> hostPixels(sizeInBytes / sizeof(glm::vec4));
		vkMapMemory(m_device, stagingBuffer.bufferMemory, 0, sizeInBytes, 0, &data);
		memcpy(&hostPixels[0], data, sizeInBytes);
		vkUnmapMemory(m_device, stagingBuffer.bufferMemory);

		saveImageCube(PROBE_BASE_DIR "Diffuse_HDR.dds",
			DIFF_IRRADIANCE_MAP_SIZE, DIFF_IRRADIANCE_MAP_SIZE, sizeof(glm::vec4),
			m_skybox.diffuseIrradianceMap.mipLevels, gli::FORMAT_RGBA32_SFLOAT_PACK32, hostPixels.data());
	}

	if (m_skybox.shouldSaveSpecMap)
	{
		transitionImageLayout(m_device, m_graphicsCommandPool, m_graphicsQueue,
			m_skybox.specularIrradianceMap.image, m_skybox.specularIrradianceMap.format, 6, m_skybox.specularIrradianceMap.mipLevels,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		BufferWrapper stagingBuffer{ m_device };
		VkDeviceSize sizeInBytes = compute2DImageSizeInBytes(
			SPEC_IRRADIANCE_MAP_SIZE, SPEC_IRRADIANCE_MAP_SIZE, sizeof(glm::vec4), m_skybox.specularIrradianceMap.mipLevels, 6);

		createBuffer(m_physicalDevice, m_device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer.buffer, stagingBuffer.bufferMemory);

		std::vector<VkBufferImageCopy> imageCopyRegions;
		VkDeviceSize offset = 0;

		for (uint32_t face = 0; face < 6; ++face)
		{
			for (uint32_t level = 0; level < m_skybox.specularIrradianceMap.mipLevels; ++level)
			{
				uint32_t faceWidth = SPEC_IRRADIANCE_MAP_SIZE / (1 << level);
				uint32_t faceHeight = faceWidth;

				VkBufferImageCopy region = {};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = level;
				region.imageSubresource.baseArrayLayer = face;
				region.imageSubresource.layerCount = 1;
				region.imageExtent.width = faceWidth;
				region.imageExtent.height = faceHeight;
				region.imageExtent.depth = 1;
				region.bufferOffset = offset;

				imageCopyRegions.push_back(region);

				offset += faceWidth * faceHeight * sizeof(glm::vec4);
			}
		}

		copyImageToBuffer(m_device, m_graphicsCommandPool, m_graphicsQueue, imageCopyRegions, m_skybox.specularIrradianceMap.image, stagingBuffer.buffer);

		void* data;
		std::vector<glm::vec4> hostPixels(sizeInBytes / sizeof(glm::vec4));
		vkMapMemory(m_device, stagingBuffer.bufferMemory, 0, sizeInBytes, 0, &data);
		memcpy(&hostPixels[0], data, sizeInBytes);
		vkUnmapMemory(m_device, stagingBuffer.bufferMemory);

		saveImageCube(PROBE_BASE_DIR "Specular_HDR.dds",
			SPEC_IRRADIANCE_MAP_SIZE, SPEC_IRRADIANCE_MAP_SIZE, sizeof(glm::vec4),
			m_skybox.specularIrradianceMap.mipLevels, gli::FORMAT_RGBA32_SFLOAT_PACK32, hostPixels.data());
	}

	vkDeviceWaitIdle(m_device);
}

VkFormat DeferredRenderer::findDepthFormat()
{
	return m_vulkanManager.chooseSupportedFormatFromCandidates(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

#endif