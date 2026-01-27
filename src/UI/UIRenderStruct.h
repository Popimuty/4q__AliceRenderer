#pragma once

#include <windows.h>   // UINT
#include <d2d1_1.h>    // ID2D1Factory1, ID2D1DeviceContext, ID2D1Bitmap1, ID2D1SolidColorBrush
#include <dwrite.h>    // IDWriteFactory
#include <wincodec.h>
#include <wrl/client.h>
#include <string>
#include <stdexcept>
using Microsoft::WRL::ComPtr;

struct UIRenderStruct
{
    ComPtr<ID2D1Factory1>        m_d2DFactory;
    ComPtr<ID2D1Device>          m_d2DDevice;
    ComPtr<ID2D1DeviceContext>   m_d2DdevCon;
    ComPtr<IDWriteFactory>       m_D3DWFactory;
    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<IWICImagingFactory>   m_wicImageFactory;
    ComPtr<ID2D1Bitmap1>         m_d2dTargetBitmap;
    UINT m_width{ 0 };
    UINT m_height{ 0 };
    
    // 뷰포트 정보 (실제 그려지는 화면 영역)
    // letterbox/pillarbox가 있을 때 윈도우 전체가 아닌 실제 렌더링 영역을 나타냄
    float m_viewportX{ 0.0f };      // 뷰포트 왼쪽 상단 X (윈도우 클라이언트 좌표 기준)
    float m_viewportY{ 0.0f };      // 뷰포트 왼쪽 상단 Y (윈도우 클라이언트 좌표 기준)
    float m_viewportWidth{ 0.0f };  // 뷰포트 너비
    float m_viewportHeight{ 0.0f }; // 뷰포트 높이
    
    // ImGui 뷰포트 오프셋 (ImGui 창 내부의 게임 뷰포트 시작 위치)
    float m_imGuiOffsetX{ 0.0f };   // ImGui 창 내부 오프셋 X
    float m_imGuiOffsetY{ 0.0f };   // ImGui 창 내부 오프셋 Y
};