#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

class VulcanTriangle 
{

private:
    GLFWwindow* window = nullptr;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    VkInstance instance = nullptr;

public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow() 
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, false);
        glfwWindowHint(GLFW_RESIZABLE, false);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulcan", nullptr, nullptr);
    }

    void initVulkan() 
    {
        createInstance();
    }

    void createInstance()
    {
        VkApplicationInfo appInfo{};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Vulcan";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "Empty Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t requiredExtCount = 0;
        const char** requiredExt = glfwGetRequiredInstanceExtensions(&requiredExtCount);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = requiredExtCount;
        createInfo.ppEnabledExtensionNames = requiredExt;
        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
        
        uint32_t availableExtCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, nullptr);

        std::vector<VkExtensionProperties> availableExt(availableExtCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, availableExt.data());

        checkRequiredExtensionsPresent(availableExt, requiredExt, requiredExtCount);
    }

    void checkRequiredExtensionsPresent(std::vector<VkExtensionProperties> availableExt, const char** requiredExt, int requiredExtCount)
    {
        for (auto i = 0; i < requiredExtCount; ++i) 
        {
            bool found = false;

            for (const auto& extension : availableExt) 
            {
                if (strcmp(requiredExt[i], extension.extensionName)) 
                {
                    found = true;
                }
            }

            if (!found) 
            {
                throw std::runtime_error("missing vulkan extension");
            }
        }

        std::cout << "extension requirement fulfilled" << std::endl;
    }

    void mainLoop() 
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() 
    {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

int main() 
{
    VulcanTriangle app;

    try 
    {
        app.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}