#pragma once
#include <cstdint>
#include <string>

class UIBase; // forward declaration
class UIWorld; // forward declaration

// 컴포넌트의 부모
struct IUIComponent
{
    virtual ~IUIComponent() = default;

    // 주인 포인터
    UIBase* Owner = nullptr;
    unsigned long OwnerID = 0;
    
    // World 포인터 (이름 기반 조회용)
    UIWorld* World = nullptr;

    // 추가된 경우
    virtual void OnAdded() {};
    virtual void Update() {};
    virtual void Render() {};

    // 편의 함수: Owner 이름 조회 (선언만, 구현은 IUIComponent.cpp에서)
    const std::string& GetOwnerName() const;

    //  
    long unsigned id{ 0 };
    long unsigned owner{ 0 };
};
