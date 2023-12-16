#include "ShaderUtils.h"

VkShaderModule ShaderCreation::createShaderModule(VkDevice logicalDevice, const char* shaderCodeFile) {
        std::vector<char> shaderCode;
        try {
                shaderCode = readFile(shaderCodeFile);
                return createShaderModule(logicalDevice, shaderCode);
        } catch (const std::exception& e) {
                FATAL("Failed to read shader file: {}", e.what());
        }
        return nullptr;
}
VkShaderModule ShaderCreation::createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderCode) {
        INFO("Creating shader module.");
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
        VkShaderModule shaderModule;

        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                FATAL("Failed to create shader module!");
        }
        return shaderModule;
}
