#pragma once

#define NOMINMAX

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <locale>
#include <numeric>
#include <optional>
#include <set>
#include <stack>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SimpleMath.h>
#include <DirectXCollision.h>

#pragma warning(push)
#pragma warning(disable : 4127)
#include <external/nlohmann/json.hpp>
#pragma warning(pop)

#include <external/boost/di.hpp>

#include <windows.h>
#include <WinHttp.h>
#include <shellapi.h>
#include <wrl/client.h>
#include <d3d11.h>

#include <trview.common/Algorithms.h>
#include <trview.common/Colour.h>
#include <trview.common/Event.h>
#include <trview.common/Json.h>
#include <trview.common/Maths.h>
#include <trview.common/MessageHandler.h>
#include <trview.common/Point.h>
#include <trview.common/Resources.h>
#include <trview.common/Size.h>
#include <trview.common/Strings.h>
#include <trview.common/Timer.h>
#include <trview.common/Window.h>
#include <trview.common/Windows/IDialogs.h>
#include <trview.common/Windows/IShortcuts.h>
#include <trview.graphics/RasterizerStateStore.h>
#include <trview.graphics/IDevice.h>
#include <trview.graphics/IDeviceWindow.h>
#include <trview.graphics/IRenderTarget.h>
#include <trview.graphics/IShader.h>
#include <trview.graphics/IShaderStorage.h>
#include <trview.graphics/Texture.h>
#include <trview.graphics/PixelShaderStore.h>
#include <trview.graphics/VertexShaderStore.h>
#include <trview.input/IMouse.h>

#include <trview.ui/Button.h>
#include <trview.ui/Label.h>
#include <trview.ui/Checkbox.h>
#include <trview.ui/GroupBox.h>
#include <trview.ui/NumericUpDown.h>
#include <trview.ui/Listbox.h>
#include <trview.ui/TextArea.h>
#include <trview.ui/Slider.h>
#include <trview.ui/Window.h>
#include <trview.ui/Image.h>
#include <trview.ui/Layouts/StackLayout.h>
#include <trview.ui/ILoader.h>

#include <trview.ui.render/IRenderer.h>
#include <trview.ui.render/IMapRenderer.h>