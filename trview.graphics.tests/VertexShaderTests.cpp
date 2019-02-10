#include "CppUnitTest.h"

#include <trview.graphics/VertexShader.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace trview
{
    namespace graphics
    {
        namespace tests
        {
            TEST_CLASS(VertexShaderTests)
            {
                // Tests that trying to create a vertex shader with no data throws.
                TEST_METHOD(EmptyData)
                {
                    graphics::Device device;
                    Assert::ExpectException<std::exception>([&device]() { VertexShader{ device, std::vector<uint8_t>(), std::vector<D3D11_INPUT_ELEMENT_DESC>(1) }; });
                }

                // Tests that trying to create a vertex shader with no input descriptions throws.
                TEST_METHOD(EmptyInputDescription)
                {
                    graphics::Device device;
                    Assert::ExpectException<std::exception>([&device]() { VertexShader{ device, std::vector<uint8_t>(1), std::vector<D3D11_INPUT_ELEMENT_DESC>() }; });
                }
            };
        }
    }
}