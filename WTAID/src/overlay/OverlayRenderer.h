#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <memory>
#include <utility>

#include "ui/GUI.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxgi.lib")

// Forward declaration для GUIFrame и ColorScheme
class GUI;
typedef std::function<void(int sectionIdx, int slotIdx)> SlotClickCallback;
typedef std::function<void(int sectionIdx, int slotIdx, int dx, int dy)> SlotMoveCallback;

// ---------------------------------------------------------------------------
// Структура для рендеринга одного текстового элемента
// ---------------------------------------------------------------------------
struct TextElement
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
    float width = 600.0f;
    float height = 20.0f;
    float fontSize = 16.0f;
    D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
    bool bold = false;
    bool center = false;
    int slotIndex = -1;
    int slotOffsetX = 0;
    int slotOffsetY = 0;
};

// ---------------------------------------------------------------------------
// Структура для рендеринга прогресс-бара
// ---------------------------------------------------------------------------
struct ProgressBar
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 200.0f;
    float height = 20.0f;
    float fraction = 0.0f;
    D2D1_COLOR_F bgColor = D2D1::ColorF(0.137f, 0.165f, 0.204f, 1.0f);
    D2D1_COLOR_F fillColor = D2D1::ColorF(0.235f, 0.627f, 0.996f, 1.0f);
    D2D1_COLOR_F borderColor = D2D1::ColorF(0.196f, 0.235f, 0.294f, 1.0f);
    int slotOffsetX = 0;
    int slotOffsetY = 0;
};

// ---------------------------------------------------------------------------
// Структура для рендеринга секции
// ---------------------------------------------------------------------------
struct RenderSection
{
    std::wstring title;
    float x = 0.0f;
    float y = 0.0f;
    float width = 600.0f;
    float height = 200.0f;
    D2D1_COLOR_F bgColor = D2D1::ColorF(0.098f, 0.118f, 0.149f, 0.85f);
    D2D1_COLOR_F borderColor = D2D1::ColorF(0.196f, 0.235f, 0.294f, 1.0f);
    std::vector<TextElement> labels;
    std::vector<TextElement> values;
    std::vector<ProgressBar> bars;
    std::vector<D2D1_RECT_F> slotRects;
    std::vector<std::pair<int, int>> slotOffsets;
};

// ---------------------------------------------------------------------------
// Информация о выделенном слоте
// ---------------------------------------------------------------------------
struct SelectedSlotInfo {
    int sectionIdx = -1;
    int slotIdx = -1;
    int offsetX = 0;
    int offsetY = 0;
};

// ---------------------------------------------------------------------------
// Основной рендерер оверлея на DirectX 11 + Direct2D
// ---------------------------------------------------------------------------
class OverlayRenderer
{
public:
    OverlayRenderer();
    ~OverlayRenderer();

    OverlayRenderer(const OverlayRenderer&) = delete;
    OverlayRenderer& operator=(const OverlayRenderer&) = delete;

    bool initialize(HWND hwnd, int width, int height);
    void render(const GUIFrame& frame, const ColorScheme& colors);
    void show();
    void hide();
    void shutdown();
    HWND getHWND() const { return hwnd_; }
    bool isInitialized() const { return isInitialized_; }
    void updateWindowSize();
    void setPositionDelta(int dx, int dy);
    void setSlotClickCallback(SlotClickCallback callback) { slotClickCallback_ = callback; }
    void setSlotMoveCallback(SlotMoveCallback callback) { slotMoveCallback_ = callback; }
    bool handleSlotClick(int mouseX, int mouseY);
    void setSelectedSlotOffset(int offsetX, int offsetY);
    std::pair<int, int> getSelectedSlot() const { return selectedSlot_; }
    void setSelectedSlot(int sectionIdx, int slotIdx) { selectedSlot_ = {sectionIdx, slotIdx}; }
    void clearSelectedSlot() { selectedSlot_ = {-1, -1}; }
    SlotMoveCallback getSlotMoveCallback() const { return slotMoveCallback_; }
    void setSelectedSlotInfos(const std::vector<SelectedSlotInfo>& infos) { selectedSlotInfos_ = infos; }
    const std::vector<SelectedSlotInfo>& getSelectedSlotInfos() const { return selectedSlotInfos_; }

private:
    bool createDeviceResources();
    bool createWindowSizeDependentResources();
    void renderFrame(const GUIFrame& frame, const ColorScheme& colors);
    void renderBackground(ID2D1HwndRenderTarget* renderTarget, int width, int height);
    void renderSections(ID2D1HwndRenderTarget* renderTarget, 
                        const std::vector<RenderSection>& sections,
                        const ColorScheme& colors);

    IDXGIFactory1* factory_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11Texture2D* backBuffer_ = nullptr;
    ID2D1HwndRenderTarget* renderTarget_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    ID2D1Factory* d2dFactory_ = nullptr;
    IDWriteTextFormat* fontRegular_ = nullptr;
    IDWriteTextFormat* fontBold_ = nullptr;
    IDWriteTextFormat* fontSmall_ = nullptr;
    ID2D1SolidColorBrush* brushBlack_ = nullptr;
    ID2D1SolidColorBrush* brushWhite_ = nullptr;
    ID2D1SolidColorBrush* brushBlue_ = nullptr;
    ID2D1SolidColorBrush* brushRed_ = nullptr;
    ID2D1SolidColorBrush* brushYellow_ = nullptr;
    ID2D1SolidColorBrush* brushGray_ = nullptr;
    HWND hwnd_ = nullptr;
    int windowWidth_ = 720;
    int windowHeight_ = 680;
    int overlayPosX_ = 0;
    int overlayPosY_ = 0;
    bool isInitialized_ = false;
    bool isVisible_ = false;
    SlotClickCallback slotClickCallback_ = nullptr;
    SlotMoveCallback slotMoveCallback_ = nullptr;
    std::vector<std::vector<D2D1_RECT_F>> slotRects_;
    std::pair<int, int> selectedSlot_ = {-1, -1};
    std::vector<SelectedSlotInfo> selectedSlotInfos_;
};
