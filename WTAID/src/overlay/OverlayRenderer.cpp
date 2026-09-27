#define NOMINMAX
#include "overlay/OverlayRenderer.h"
#include <algorithm>
#include <cmath>
#include <utility>

// ---------------------------------------------------------------------------
// Вспомогательная функция: COLORREF → D2D1_COLOR_F
// ---------------------------------------------------------------------------
static D2D1_COLOR_F colorRefToD2D(COLORREF cr) {
    return D2D1::ColorF(
        static_cast<float>(GetRValue(cr)) / 255.0f,
        static_cast<float>(GetGValue(cr)) / 255.0f,
        static_cast<float>(GetBValue(cr)) / 255.0f,
        1.0f
    );
}

// ---------------------------------------------------------------------------
// Вспомогательная функция: UTF-8 → wstring
// ---------------------------------------------------------------------------
static std::wstring utf8ToWString(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    if (!wstr.empty() && wstr.back() == L'\0') {
        wstr.pop_back();
    }
    return wstr;
}

// ---------------------------------------------------------------------------
// Конструктор
// ---------------------------------------------------------------------------
OverlayRenderer::OverlayRenderer() {}

// ---------------------------------------------------------------------------
// Деструктор
// ---------------------------------------------------------------------------
OverlayRenderer::~OverlayRenderer() {
    shutdown();
}

// ---------------------------------------------------------------------------
// Инициализация D2D ресурсов
// ---------------------------------------------------------------------------
bool OverlayRenderer::initialize(HWND hwnd, int width, int height) {
    hwnd_ = hwnd;
    windowWidth_ = width;
    windowHeight_ = height;

    // Создаём D2D Factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    __uuidof(ID2D1Factory), nullptr,
                                    reinterpret_cast<void**>(&d2dFactory_));
    if (FAILED(hr)) return false;

    // Создаём DWrite Factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(hr)) return false;

    // Создаём текстовые форматы
    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"ru-RU", &fontRegular_);
    if (FAILED(hr)) return false;

    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"ru-RU", &fontBold_);
    if (FAILED(hr)) return false;

    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"ru-RU", &fontSmall_);
    if (FAILED(hr)) return false;

    // Выравнивание текста
    fontRegular_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    fontRegular_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fontRegular_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    
    fontBold_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    fontBold_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fontBold_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    
    fontSmall_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    fontSmall_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fontSmall_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    // Создаём HwndRenderTarget для D2D
    D2D1_SIZE_U d2dSize = D2D1::SizeU(static_cast<UINT>(width), static_cast<UINT>(height));
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(hwnd, d2dSize);
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE)
    );

    hr = d2dFactory_->CreateHwndRenderTarget(props, hwndProps, &renderTarget_);
    if (FAILED(hr)) return false;

    // Создаём кисти
    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0, 0, 0, 1), &brushBlack_);
    if (FAILED(hr)) return false;

    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(1, 1, 1, 1), &brushWhite_);
    if (FAILED(hr)) return false;

    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.235f, 0.627f, 0.996f, 1), &brushBlue_);
    if (FAILED(hr)) return false;

    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(1, 0.27f, 0.27f, 1), &brushRed_);
    if (FAILED(hr)) return false;

    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(1, 0.7f, 0, 1), &brushYellow_);
    if (FAILED(hr)) return false;

    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.55f, 0.55f, 0.55f, 1), &brushGray_);
    if (FAILED(hr)) return false;

    isInitialized_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// Обновление размеров окна
// ---------------------------------------------------------------------------
void OverlayRenderer::updateWindowSize() {
    if (!hwnd_) return;
    
    if (renderTarget_) {
        renderTarget_->Release();
        renderTarget_ = nullptr;
    }
    
    if (d2dFactory_ && hwnd_) {
        RECT rect;
        GetClientRect(hwnd_, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        if (width > 0 && height > 0) {
            windowWidth_ = width;
            windowHeight_ = height;
            
            D2D1_SIZE_U d2dSize = D2D1::SizeU(static_cast<UINT>(width), static_cast<UINT>(height));
            D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(hwnd_, d2dSize);
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE)
            );
            
            HRESULT hr = d2dFactory_->CreateHwndRenderTarget(props, hwndProps, &renderTarget_);
            if (SUCCEEDED(hr) && renderTarget_) {
                if (brushBlack_) { brushBlack_->Release(); brushBlack_ = nullptr; }
                if (brushWhite_) { brushWhite_->Release(); brushWhite_ = nullptr; }
                if (brushBlue_) { brushBlue_->Release(); brushBlue_ = nullptr; }
                if (brushRed_) { brushRed_->Release(); brushRed_ = nullptr; }
                if (brushYellow_) { brushYellow_->Release(); brushYellow_ = nullptr; }
                if (brushGray_) { brushGray_->Release(); brushGray_ = nullptr; }
                
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &brushBlack_);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &brushWhite_);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.235f, 0.627f, 0.996f, 1), &brushBlue_);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(1, 0.27f, 0.27f, 1), &brushRed_);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(1, 0.7f, 0, 1), &brushYellow_);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.55f, 0.55f, 1), &brushGray_);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Установка позиции оверлея (сдвиг в пикселях)
// ---------------------------------------------------------------------------
void OverlayRenderer::setPositionDelta(int dx, int dy) {
    if (!hwnd_) return;
    
    overlayPosX_ += dx;
    overlayPosY_ += dy;
    
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    SetWindowPos(hwnd_, HWND_TOPMOST, overlayPosX_, overlayPosY_,
                 width, height, SWP_NOZORDER);
}

// ---------------------------------------------------------------------------
// Обработка клика по слоту индикатора
// ---------------------------------------------------------------------------
bool OverlayRenderer::handleSlotClick(int mouseX, int mouseY) {
    for (size_t s = 0; s < slotRects_.size(); ++s) {
        const auto& sectionRects = slotRects_[s];
        for (size_t sl = 0; sl < sectionRects.size(); ++sl) {
            const auto& rect = sectionRects[sl];
            if (mouseX >= rect.left && mouseX <= rect.right &&
                mouseY >= rect.top && mouseY <= rect.bottom) {
                selectedSlot_ = {static_cast<int>(s), static_cast<int>(sl)};
                if (slotClickCallback_) {
                    slotClickCallback_(static_cast<int>(s), static_cast<int>(sl));
                }
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Установка смещения для выделенного слота
// ---------------------------------------------------------------------------
void OverlayRenderer::setSelectedSlotOffset(int dx, int dy) {
    (void)dx;
    (void)dy;
}

// ---------------------------------------------------------------------------
// Освобождение ресурсов
// ---------------------------------------------------------------------------
void OverlayRenderer::shutdown() {
    if (brushBlack_) { brushBlack_->Release(); brushBlack_ = nullptr; }
    if (brushWhite_) { brushWhite_->Release(); brushWhite_ = nullptr; }
    if (brushBlue_) { brushBlue_->Release(); brushBlue_ = nullptr; }
    if (brushRed_) { brushRed_->Release(); brushRed_ = nullptr; }
    if (brushYellow_) { brushYellow_->Release(); brushYellow_ = nullptr; }
    if (brushGray_) { brushGray_->Release(); brushGray_ = nullptr; }

    if (fontRegular_) { fontRegular_->Release(); fontRegular_ = nullptr; }
    if (fontBold_) { fontBold_->Release(); fontBold_ = nullptr; }
    if (fontSmall_) { fontSmall_->Release(); fontSmall_ = nullptr; }

    if (renderTarget_) { renderTarget_->Release(); renderTarget_ = nullptr; }
    if (writeFactory_) { writeFactory_->Release(); writeFactory_ = nullptr; }
    if (d2dFactory_) { d2dFactory_->Release(); d2dFactory_ = nullptr; }

    hwnd_ = nullptr;
    isInitialized_ = false;
}

// ---------------------------------------------------------------------------
// Показать оверлей
// ---------------------------------------------------------------------------
void OverlayRenderer::show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        isVisible_ = true;
    }
}

// ---------------------------------------------------------------------------
// Скрыть оверлей
// ---------------------------------------------------------------------------
void OverlayRenderer::hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
        isVisible_ = false;
    }
}

// ---------------------------------------------------------------------------
// Отрисовка кадра (публичный метод)
// ---------------------------------------------------------------------------
void OverlayRenderer::render(const GUIFrame& frame, const ColorScheme& colors) {
    if (!isInitialized_ || !renderTarget_ || !hwnd_) return;
    if (!isVisible_) return;

    renderFrame(frame, colors);
}

// ---------------------------------------------------------------------------
// Внутренняя отрисовка кадра
// ---------------------------------------------------------------------------
void OverlayRenderer::renderFrame(const GUIFrame& frame, const ColorScheme& colors) {
    int width, height;
    RECT rect;
    GetClientRect(hwnd_, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    
    if (width <= 0 || height <= 0) return;
    
    windowWidth_ = width;
    windowHeight_ = height;

    // Очищаем координаты слотов
    slotRects_.clear();

    // Начинаем отрисовку D2D
    renderTarget_->BeginDraw();
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    
    // Очищаем рендер-таргет полностью прозрачным
    renderTarget_->Clear(D2D1::ColorF(0, 0, 0, 0));

    // Формируем секции из frame данных
    std::vector<RenderSection> sections;
    float currentY = 10.0f;

    for (const auto& section : frame.sections) {
        RenderSection rs;
        rs.x = 10.0f;
        rs.y = currentY;
        rs.width = static_cast<float>(width) - 20.0f;

        float sectionY = 0.0f;
        int slotIdx = 0;

        for (const auto& slot : section.slots) {
            int slotOffsetX = slot.offsetX;
            int slotOffsetY = slot.offsetY;
            
            slotOffsetX = static_cast<int>(std::round(static_cast<float>(slotOffsetX) / 10.0f) * 10.0f);
            slotOffsetY = static_cast<int>(std::round(static_cast<float>(slotOffsetY) / 10.0f) * 10.0f);
            
            const int margin = 10;
            const int minWidth = 20;
            const int maxX = windowWidth_ - minWidth - margin;
            const int maxY = windowHeight_ - 200;
            
            if (slotOffsetX < -maxX) slotOffsetX = -maxX;
            if (slotOffsetX > maxX) slotOffsetX = maxX;
            // Ограничение только на перемещение вверх
            if (slotOffsetY < -maxY) slotOffsetY = -maxY;
            
            TextElement labelEl;
            labelEl.text = utf8ToWString(slot.label);
            labelEl.x = 10.0f;
            labelEl.y = sectionY;
            labelEl.slotOffsetX = slotOffsetX;
            labelEl.slotOffsetY = slotOffsetY;
            labelEl.width = 150.0f;
            labelEl.height = 24.0f;
            labelEl.fontSize = 14.0f;
            labelEl.color = colorRefToD2D(colors.textLabel);
            labelEl.slotIndex = slotIdx;
            rs.labels.push_back(labelEl);

            TextElement valueEl;
            valueEl.text = utf8ToWString(slot.value);
            valueEl.x = labelEl.x + labelEl.width + 20.0f;
            valueEl.y = sectionY;
            valueEl.slotOffsetX = slotOffsetX;
            valueEl.slotOffsetY = slotOffsetY;
            valueEl.width = static_cast<float>(width) - valueEl.x - 20.0f;
            valueEl.height = 24.0f;
            valueEl.fontSize = 16.0f;
            valueEl.color = slot.alert ? colorRefToD2D(colors.textAlert)
                                        : colorRefToD2D(colors.textValue);
            valueEl.bold = slot.alert;
            valueEl.slotIndex = slotIdx;
            rs.values.push_back(valueEl);
            
            slotIdx++;

            float slotAbsX = rs.x + std::min(labelEl.x, valueEl.x);
            float slotAbsY = rs.y + sectionY;
            float slotAbsWidth = std::max(labelEl.x + labelEl.width, valueEl.x + valueEl.width) - slotAbsX;
            float slotAbsHeight = 24.0f;
            
            if (slot.fraction != 0.0f) {
                slotAbsHeight = 34.0f;
            }
            
            D2D1_RECT_F slotRect = D2D1::RectF(slotAbsX, slotAbsY,
                                                slotAbsX + slotAbsWidth,
                                                slotAbsY + slotAbsHeight);
            rs.slotRects.push_back(slotRect);
            rs.slotOffsets.push_back({slotOffsetX, slotOffsetY});

            if (slot.fraction != 0.0f) {
                ProgressBar bar;
                bar.x = labelEl.x;
                bar.y = sectionY + 22.0f;
                bar.width = labelEl.width;
                bar.height = 5.0f;
                bar.fraction = std::max(0.0f, std::min(1.0f, slot.fraction));
                bar.slotOffsetX = slotOffsetX;
                bar.slotOffsetY = slotOffsetY;
                
                D2D1_COLOR_F fillColor;
                if (slot.alert) {
                    fillColor = colorRefToD2D(colors.barAlert);
                } else if (slot.fraction > 0.9f) {
                    fillColor = colorRefToD2D(colors.barWarning);
                } else {
                    fillColor = colorRefToD2D(colors.barNormal);
                }
                bar.fillColor = fillColor;
                bar.bgColor = colorRefToD2D(colors.barBg);
                bar.borderColor = colorRefToD2D(colors.bgPanelBorder);
                rs.bars.push_back(bar);
                sectionY += 34.0f;
            } else {
                sectionY += 24.0f;
            }
        }

        rs.height = sectionY + 10.0f;
        sections.push_back(rs);
        currentY += rs.height + 10.0f;
    }

    // Сохраняем координаты слотов для определения клика
    for (const auto& section : sections) {
        slotRects_.push_back(section.slotRects);
    }

    // Рендерим секции (алерты уже в слотах)
    renderSections(renderTarget_, sections, colors);

    // Завершаем отрисовку D2D
    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        renderTarget_->Release();
        renderTarget_ = nullptr;
        return;
    }
    if (FAILED(hr)) return;
}

// ---------------------------------------------------------------------------
// Отрисовка секций
// ---------------------------------------------------------------------------
void OverlayRenderer::renderSections(ID2D1HwndRenderTarget* renderTarget,
                                      const std::vector<RenderSection>& sections,
                                      const ColorScheme& colors) {
    for (const auto& section : sections) {
        for (const auto& label : section.labels) {
            // Если это заголовок (bold и большой шрифт)
            if (label.bold && label.fontSize >= 16.0f) {
                // Фон заголовка
                D2D1_RECT_F titleBgRect = D2D1::RectF(
                    section.x, section.y + label.y - 4,
                    section.x + section.width, section.y + label.y + 24
                );
                
                ID2D1SolidColorBrush* titleBgBrush;
                D2D1_COLOR_F titleBgColor = D2D1::ColorF(
                    static_cast<float>(GetRValue(colors.bgPanel)) / 255.0f,
                    static_cast<float>(GetGValue(colors.bgPanel)) / 255.0f,
                    static_cast<float>(GetBValue(colors.bgPanel)) / 255.0f,
                    0.6f // полупрозрачный
                );
                renderTarget->CreateSolidColorBrush(titleBgColor, &titleBgBrush);
                renderTarget->FillRectangle(titleBgRect, titleBgBrush);
                titleBgBrush->Release();
                
                // Разделительная линия
                ID2D1SolidColorBrush* lineBrush;
                D2D1_COLOR_F lineColor = D2D1::ColorF(
                    static_cast<float>(GetRValue(colors.bgPanelBorder)) / 255.0f,
                    static_cast<float>(GetGValue(colors.bgPanelBorder)) / 255.0f,
                    static_cast<float>(GetBValue(colors.bgPanelBorder)) / 255.0f,
                    0.8f
                );
                renderTarget->CreateSolidColorBrush(lineColor, &lineBrush);
                renderTarget->DrawLine(
                    D2D1::Point2F(section.x, section.y + label.y + 22),
                    D2D1::Point2F(section.x + section.width, section.y + label.y + 22),
                    lineBrush, 1.0f
                );
                lineBrush->Release();
            }
            
            D2D1_RECT_F textRect = D2D1::RectF(
                label.x, label.y, label.x + label.width, label.y + 24.0f
            );

            IDWriteTextFormat* format = label.bold ? fontBold_ : fontRegular_;
            if (label.fontSize <= 12.0f) format = fontSmall_;

            ID2D1SolidColorBrush* labelBrush;
            renderTarget->CreateSolidColorBrush(label.color, &labelBrush);
            
            D2D1_RECT_F labelRect = D2D1::RectF(
                section.x + label.x + static_cast<float>(label.slotOffsetX), section.y + label.y + static_cast<float>(label.slotOffsetY),
                section.x + label.x + label.width + static_cast<float>(label.slotOffsetX), section.y + label.y + label.height + static_cast<float>(label.slotOffsetY)
            );
            renderTarget->DrawText(
                label.text.c_str(),
                static_cast<UINT32>(label.text.length()),
                format, labelRect, labelBrush
            );
            labelBrush->Release();
        }

        for (const auto& value : section.values) {
            D2D1_RECT_F valueRect = D2D1::RectF(
                section.x + value.x + static_cast<float>(value.slotOffsetX), section.y + value.y + static_cast<float>(value.slotOffsetY),
                section.x + value.x + value.width + static_cast<float>(value.slotOffsetX), section.y + value.y + value.height + static_cast<float>(value.slotOffsetY)
            );

            IDWriteTextFormat* format = value.bold ? fontBold_ : fontRegular_;

            ID2D1SolidColorBrush* valueBrush;
            renderTarget->CreateSolidColorBrush(value.color, &valueBrush);
            renderTarget->DrawText(
                value.text.c_str(),
                static_cast<UINT32>(value.text.length()),
                format, valueRect, valueBrush
            );
            valueBrush->Release();
        }

        for (const auto& bar : section.bars) {
            D2D1_RECT_F barRect = D2D1::RectF(
                section.x + bar.x + static_cast<float>(bar.slotOffsetX), section.y + bar.y + static_cast<float>(bar.slotOffsetY),
                section.x + bar.x + bar.width + static_cast<float>(bar.slotOffsetX), section.y + bar.y + bar.height + static_cast<float>(bar.slotOffsetY)
            );

            ID2D1SolidColorBrush* barBgBrush;
            renderTarget->CreateSolidColorBrush(bar.bgColor, &barBgBrush);
            renderTarget->FillRectangle(barRect, barBgBrush);
            barBgBrush->Release();

            if (bar.fraction > 0.0f) {
                D2D1_RECT_F fillRect = D2D1::RectF(
                    section.x + bar.x + static_cast<float>(bar.slotOffsetX), section.y + bar.y + static_cast<float>(bar.slotOffsetY),
                    section.x + bar.x + bar.width * bar.fraction + static_cast<float>(bar.slotOffsetX), section.y + bar.y + bar.height + static_cast<float>(bar.slotOffsetY)
                );
                ID2D1SolidColorBrush* fillBrush;
                renderTarget->CreateSolidColorBrush(bar.fillColor, &fillBrush);
                renderTarget->FillRectangle(fillRect, fillBrush);
                fillBrush->Release();
            }

            ID2D1SolidColorBrush* barBorderBrush;
            renderTarget->CreateSolidColorBrush(bar.borderColor, &barBorderBrush);
            renderTarget->DrawRectangle(barRect, barBorderBrush, 1.0f);
            barBorderBrush->Release();
        }
    }
}
