#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <concepts>
#include <typeindex>
#include <typeinfo>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <utility>
//#include "UIComponent/UITransformClass.h"
#include "IUIComponent.h"
#include "UIRenderStruct.h"

// Core delegate wrapper 
#include "Core/Delegate.h"
#include "Core/InputSystem.h"
#include "UITransform.h"
#include "UIBase.h"
#include "UICompDelegate.h"
#include "UI_ImageComponent.h"
#include "UI_ScriptComponent.h"
#include "UIScriptSystem.h"
#include "UIImage.h"
#include "UIButton.h"
#include "UIGaugeBar.h"
#include "Core/Logger.h"

// Forward declaration
class UIButton;

// ============================================================================
// UI Script Entry: UIWorld에서 여러 스크립트를 관리하기 위한 구조체
// (World의 ScriptComponent와 유사한 구조)
// ============================================================================
struct UIScriptEntry
{
	std::string scriptName;                    // 스크립트 이름 (동적 생성에 사용)
	std::unique_ptr<IUIScript> instance;     // 실제 스크립트 인스턴스
	bool enabled{ true };                      // 실행 여부
	bool awoken{ false };                     // OnAdded 호출 여부
	bool started{ false };                     // OnStart 호출 여부
};


// !!!추가 필요!!!
// 부모에 ID 기반 코드를 추가해야 함
// void SetParent()를 추가할 때 부모의 rect를 자식에게 전달해야 하는지 확인 필요
// -> collider 같은 rect를 전달할 필요가 있는지 확인

class UIBase;
class EditorCore;

// ============================================================================
// UIWorld: UI버전 world
// ============================================================================
class UIWorld
{
	friend class UISceneManager;
	friend class UILayoutSystem;
	friend class UIHitTestSystem;
	friend class UIEventSystem;
	friend class UIRenderSystem;
	friend class UIImageSystem;
	friend class UIScriptSystem;
	friend class EditorCore;

public:
	~UIWorld()
	{
		Clear();
	}

private:
	// UI 엔티티 저장소
	std::unordered_map<long unsigned, std::unique_ptr<UIBase>> pUIObjStorage; // UI Object 저장소
	std::vector<long unsigned> m_rootID; // 루트 노드 UI Object의 ID

	// 컴포넌트 저장소 (월드 소유 고정 타입)
	//std::unordered_map<unsigned long, std::unique_ptr<UITextComponent>> m_compStorage;      // UITextComponent 저장소
	std::unordered_map<unsigned long, std::unique_ptr<UITransform>> m_transformStorage;     // UITransform 저장소
	std::unordered_map<unsigned long, std::unique_ptr<UI_ImageComponent>> m_imageComponentStorage; // UI_ImageComponent 저장소
	std::unordered_map<unsigned long, std::unique_ptr<UI_ScriptComponent>> m_scriptComponentStorage; // UI_ScriptComponent 저장소 (레거시, 단일 스크립트용)
	
	// UI Script 저장소 (여러 스크립트 지원, World의 m_scripts와 유사)
	std::unordered_map<unsigned long, std::vector<UIScriptEntry>> m_scripts; // UI Script 저장소

	// 이름 레지스트리 (양방향 매핑)
	std::unordered_map<unsigned long, std::string> m_idToName;      // ID -> Name
	std::unordered_map<std::string, unsigned long> m_nameToId;      // Name -> ID

	// 공통 델리게이트: 컴포넌트 생성/조회/삭제를 위임하는 델리게이트
	CompDelegates m_worldDelegates{};

	// World Epoch: 유효성 체크를 위한 월드 버전 번호
	unsigned long long m_worldEpoch{ 1 };

	long unsigned nowInteger{ 1 };

	UIRenderStruct* m_UIRenderStruct{ nullptr };

public:
	// 초기화
	void Initialize(UIRenderStruct* UIRst);

	// ----- UI 엔티티 생성/조회 (World 소유) -----
	// UI 엔티티 생성
	template<typename T, typename... Args>
		requires std::derived_from<T, UIBase>
	T* CreateEntity(Args&&... args);

	// UI 엔티티 생성 + 부모 연결
	template<typename T, typename... Args>
		requires std::derived_from<T, UIBase>
	T* CreateChildEntity(long unsigned parentID, Args&&... args);

	// 템플릿 특수화 선언 (명시적 선언으로 링크 보장)
	template<>
	UIGaugeBar* CreateEntity<UIGaugeBar>();
	
	template<>
	UIButton* CreateEntity<UIButton>();

	// UI 엔티티 삭제 (자식까지 전체 삭제 포함)
	bool DestroyEntity(long unsigned int handle);

	// UI 엔티티 조회
	UIBase* Get(long unsigned int handle);
	const UIBase* Get(long unsigned int handle) const;

	// ----- 이름 기반 조회 -----
	// 이름 등록 (중복 시 자동 유니크화)
	bool RegisterName(unsigned long id, const std::string& name);
	// 이름 변경
	bool Rename(unsigned long id, const std::string& newName);
	// ID로 이름 조회
	const std::string* GetNameById(unsigned long id) const;
	// 이름으로 ID 조회 (없으면 0)
	unsigned long GetIdByName(const std::string& name) const;
	// 이름으로 UIBase 조회
	UIBase* GetByName(const std::string& name);
	const UIBase* GetByName(const std::string& name) const;

	// 전체 월드 클리어
	void Clear();

	// 씬 전용 UI만 클리어 (현재는 전체 클리어)
	void ClearSceneUI() { Clear(); } // 현재는 전체 클리어
	 
	// World Epoch 조회 (유효성 체크용)
	unsigned long long GetWorldEpoch() const { return m_worldEpoch; }

	// 루트 ID 목록 조회 (렌더링용)
	const std::vector<long unsigned>& GetRootIDs() const { return m_rootID; }

	// 부모 설정
	template<typename T, typename K>
		requires std::derived_from<T, UIBase> || std::derived_from<K, UIBase>
	void SetParent(T* parentUI, K* childUI);

	// ----- 컴포넌트 관련 함수 -----
	void BindWorldDelegates(const CompDelegates& d) { m_worldDelegates = d; }
	CompDelegates& GetDelegates() { return m_worldDelegates; }

	// UITextComponent 생성/조회/삭제
	//UITextComponent* CreateTextComponent(unsigned long ownerID);
	///UITextComponent* FindTextComponent(unsigned long ownerID);
	//void RemoveTextComponent(unsigned long ownerID);

	// UITransform 생성/조회/삭제
	UITransform* CreateTransformComponent(unsigned long ownerID);
	UITransform* FindTransformComponent(unsigned long ownerID);
	const UITransform* FindTransformComponent(unsigned long ownerID) const;
	void RemoveTransformComponent(unsigned long ownerID);

	// UI_ImageComponent 생성/조회/삭제
	UI_ImageComponent* CreateImageComponent(unsigned long ownerID);
	UI_ImageComponent* FindImageComponent(unsigned long ownerID);
	const UI_ImageComponent* FindImageComponent(unsigned long ownerID) const;
	void RemoveImageComponent(unsigned long ownerID);

	// UI_ScriptComponent 생성/조회/삭제 (레거시, 단일 스크립트용)
	UI_ScriptComponent* CreateScriptComponent(unsigned long ownerID);
	UI_ScriptComponent* FindScriptComponent(unsigned long ownerID);
	const UI_ScriptComponent* FindScriptComponent(unsigned long ownerID) const;
	void RemoveScriptComponent(unsigned long ownerID);
	
	// ============================================================================
	// UI Script 관리 (여러 스크립트 지원, World의 Script 시스템과 유사)
	// ============================================================================
	/// UI 엔티티에 스크립트를 추가합니다.
	UIScriptEntry& AddUIScript(unsigned long ownerID, const std::string& scriptName);
	
	/// UI 엔티티의 스크립트 목록을 반환합니다.
	std::vector<UIScriptEntry>* GetUIScripts(unsigned long ownerID);
	const std::vector<UIScriptEntry>* GetUIScripts(unsigned long ownerID) const;
	
	/// UI 엔티티에서 스크립트를 제거합니다.
	void RemoveUIScript(unsigned long ownerID, std::size_t index);
	
	/// 전체 UI Script 컨테이너 (UIScriptSystem에서 사용)
	const std::unordered_map<unsigned long, std::vector<UIScriptEntry>>& GetAllUIScriptsInWorld() const { return m_scripts; }
	std::unordered_map<unsigned long, std::vector<UIScriptEntry>>& GetAllUIScriptsInWorld() { return m_scripts; }

	// 컴포넌트 생성 (델리게이트 wrapper)
	template<class T, class... Args>
		requires std::derived_from<T, IUIComponent>
	T* CreateComponent(unsigned long ownerID, Args&&... args);

	// 컴포넌트 조회 (델리게이트 wrapper)
	template<class T>
		requires std::derived_from<T, IUIComponent>
	T* TryGetComponent(unsigned long ownerID);
	
	template<class T>
		requires std::derived_from<T, IUIComponent>
	const T* TryGetComponent(unsigned long ownerID) const;

	template<class T>
		requires std::derived_from<T, IUIComponent>
	T& GetComponent(unsigned long ownerID);

	// 컴포넌트 삭제
	template<class T>
		requires std::derived_from<T, IUIComponent>
	void RemoveComponent(unsigned long ownerID);

	// 재귀 함수: 모든 UIBase의 함수를 호출하며 전체 트리 순회
	template <typename Func, typename... Args>
	void Traverse(UIBase* node, Func action, Args... args)
	{
		if (!node) return;

		for (auto childID : node->childIDStorage)
		{
			if (auto* child = Get(childID))
			{
				action(child, args...);
				Traverse(child, action, args...);
			}
		}
	}

private:
	bool DeleteChildObjects(long unsigned int ID);
};

// ============================================================================
// UILayoutSystem: 트리/Transform 업데이트 시스템
// ============================================================================
class UILayoutSystem
{
public:
	static void UpdateTransforms(UIWorld& world);
	static void UpdateUI(UIWorld& world, float deltaTime);

private:
	static void UpdateTransformChild(UIWorld& world, UIBase* node, const D2D1::Matrix3x2F& parentWorld);
	static void UpdateUIChild(UIWorld& world, UIBase* node, float deltaTime);
};

// ============================================================================
// UIHitTestSystem: 마우스 검색 시스템
// ============================================================================
class UIHitTestSystem
{
public:
	static long unsigned FindUIUnderPointer(UIWorld& world, XMFLOAT2 MousePos, UIRenderStruct* renderStruct);

private:
	static void AABBRoot(UIWorld& world, XMFLOAT2 MousePos, UIBase* node, std::vector<long unsigned>& IDStorage);
	static void UIRotRoot(UIWorld& world, XMFLOAT2 MousePos, UIBase* node, std::vector<long unsigned>& hitMouseID);
};

// ============================================================================
// UIEventSystem: 이벤트 처리 시스템 (마우스 입력 및 UI 이벤트)
// ============================================================================
class UIEventSystem
{
public:
	// 마우스 클릭/드래그 이벤트 처리
	// viewportWidth, viewportHeight: UI 좌표 변환용 (UIWorldManager에서 전달)
	// editorMode: 사용되지 않음 (호환성을 위해 유지)
	static void UpdateMouseEvents(UIWorld& world, Alice::InputSystem* inputSystem, UIRenderStruct* renderStruct, 
		UINT viewportWidth, UINT viewportHeight, bool editorMode = false);

private:
	// 드래그 시작 임계값 (픽셀)
	static constexpr float DRAG_THRESHOLD = 5.0f;
	
	// 두 점 사이의 거리 계산
	static float Distance(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b);
};

// ============================================================================
// UIRenderSystem: 렌더링 시스템
// ============================================================================
class UIRenderSystem
{
public:
	static void Render(UIWorld& world, UIRenderStruct* renderStruct);
	static void RenderRoot(UIWorld& world, UIRenderStruct* renderStruct);

private:
	static void RenderRootChild(UIWorld& world, UIBase* node);
};

// ============================================================================
// UIImageSystem: UI_ImageComponent 업데이트/렌더링 시스템
// ============================================================================
class UIImageSystem
{
public:
	// 모든 UI_ImageComponent를 업데이트
	static void Update(UIWorld& world);

	// 모든 UI_ImageComponent를 렌더링
	static void Render(UIWorld& world, UIRenderStruct* renderStruct);

private:
	// 루트부터 시작하여 모든 엔티티의 ImageComponent 업데이트
	static void UpdateRoot(UIWorld& world);
	static void UpdateRootChild(UIWorld& world, UIBase* node);
	
	// 루트부터 시작하여 모든 엔티티의 ImageComponent 렌더링
	static void RenderRoot(UIWorld& world, UIRenderStruct* renderStruct);
	static void RenderRootChild(UIWorld& world, UIBase* node, UIRenderStruct* renderStruct);
};

// ============================================================================
// UIInputSystem: UI_InputComponent 업데이트 시스템
// ============================================================================
class UIInputSystem
{
public:
	// 모든 UI_InputComponent를 업데이트
	static void Update(UIWorld& world, Alice::InputSystem& input);

private:
	// 루트부터 시작하여 모든 엔티티의 InputComponent 업데이트
	static void UpdateRoot(UIWorld& world, Alice::InputSystem& input);
	static void UpdateRootChild(UIWorld& world, UIBase* node, Alice::InputSystem& input);
};

// ============================================================================
// UISceneManager: UI 씬/매니저 전환을 관리 (팩토리 + 월드 관리)
// ============================================================================

class UISceneManager
{
public:
	~UISceneManager()
	{
		// UIWorld의 소멸자에서 Clear() 호출됨
	}

private:
	ID3D11Device* pDev = nullptr;
	ID3D11DeviceContext* pDevCon = nullptr;
	Alice::InputSystem* m_InputSystem{ nullptr };
	UIRenderStruct* m_UIRenderStruct{ nullptr };

	// UIWorld 인스턴스
	UIWorld m_world;

	// 씬 전환 플래그 (현재 미사용)
	bool isLayerChange = false;
	void SortCanvas() {}; // 추후 추가 예정!!!!

public:
	void initalize(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, UIRenderStruct* UIRst, Alice::InputSystem* tmpSystem);
	void Update(float deltaTime);
	void Render();

	// UIWorld 접근
	UIWorld& GetWorld() { return m_world; }
	const UIWorld& GetWorld() const { return m_world; }

	// ----- UI 엔티티 생성/조회 (외부 호출용) -----
	template<typename T, typename... Args>
	T* CreateUIObjects(Args&&... args) requires std::derived_from<T, UIBase>
	{ 
		return m_world.CreateEntity<T>(std::forward<Args>(args)...); 
	}

	template<typename T, typename... Args>
	T* CreateChildUIObjects(long unsigned parentID, Args&&... args) requires std::derived_from<T, UIBase> 
	{ return m_world.CreateChildEntity<T>(parentID, std::forward<Args>(args)...); }

	UIBase* Get(long unsigned int ID) { return m_world.Get(ID); }
	bool DeleteUIObjects(long unsigned int ID) { return m_world.DestroyEntity(ID); }

	void BindWorldDelegates(const CompDelegates& d) { m_world.BindWorldDelegates(d); }

	template<class T, class... Args>
		requires std::derived_from<T, IUIComponent>
	T* CreateComponent(unsigned long ownerID, Args&&... args) { return m_world.CreateComponent<T>(ownerID, std::forward<Args>(args)...); }

	template<class T>
		requires std::derived_from<T, IUIComponent>
	T* TryGetComponent(unsigned long ownerID) { return m_world.TryGetComponent<T>(ownerID); }
	
	template<class T>
		requires std::derived_from<T, IUIComponent>
	const T* TryGetComponent(unsigned long ownerID) const { return m_world.TryGetComponent<T>(ownerID); }

	template<class T>
		requires std::derived_from<T, IUIComponent>
	T& GetComponent(unsigned long ownerID) { return m_world.GetComponent<T>(ownerID); }

	template<class T>
		requires std::derived_from<T, IUIComponent>
	void RemoveComponent(unsigned long ownerID) { m_world.RemoveComponent<T>(ownerID); }

private:
	// UIButton의 InputComponent 업데이트 헬퍼 (재귀적)
	void UpdateButtonInputRecursive(UIBase* node);
};


// Object 생성
template<typename T, typename... Args>
	requires std::derived_from<T, UIBase>
T* UIWorld::CreateEntity(Args&&... args)
{
	assert(m_UIRenderStruct && "UIWorld::Initialize() must be called before CreateEntity");

	// 일반 템플릿 사용 확인 (UIButton이면 특수화가 호출되어야 함)
	const char* typeName = typeid(T).name();
	ALICE_LOG_INFO("[UIWorld] CreateEntity<T> called (일반 템플릿): type=%s", typeName);

	auto pUIObj = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	T* ObjPtr = pUIObj.get();
	long unsigned entityID = this->nowInteger;
	ObjPtr->SetID(entityID);

	pUIObjStorage.emplace(entityID, std::move(pUIObj));
	m_rootID.push_back(entityID); // 루트 추가
	
	// 이름 자동 등록 (기본값: "UI_<ID>")
	std::string defaultName = "UI_" + std::to_string(entityID);
	RegisterName(entityID, defaultName);
	ObjPtr->SetName(defaultName);
	
	this->nowInteger++;

	ObjPtr->Initalize(*m_UIRenderStruct, m_worldDelegates);
	return ObjPtr;
}


// 부모 설정 + objetc 생성
template<typename T, typename... Args>
	requires std::derived_from<T, UIBase>
T* UIWorld::CreateChildEntity(long unsigned parentID, Args&&... args)
{
	assert(m_UIRenderStruct && "UIWorld::Initialize() must be called before CreateChildEntity");

	const char* typeName = typeid(T).name();
	ALICE_LOG_INFO("[UIWorld] CreateChildEntity<T> called: parentID=%lu, type=%s", parentID, typeName);

	auto pUIObj = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	long unsigned childID = this->nowInteger++; // 새 ID 발급
	pUIObj->ID = childID;

	ALICE_LOG_INFO("[UIWorld] CreateChildEntity: childID=%lu assigned (parentID=%lu)", childID, parentID);
	
	// 이름 자동 등록 (기본값: "UI_<ID>")
	std::string defaultName = "UI_" + std::to_string(childID);
	RegisterName(childID, defaultName);
	pUIObj->SetName(defaultName);

	UIBase* parentNode = Get(parentID);
	assert(parentNode && "CreateChildEntity: parentNode must not be null");

	// 부모 연결
	parentNode->childIDStorage.push_back(childID);
	pUIObj->parentID = parentID;

	// child가 root에 포함되지 않도록 확인 (이미 parent가 있으므로 root가 아님)
	// m_rootID에서 제거 (혹시 모를 경우 대비)
	auto it = std::find(m_rootID.begin(), m_rootID.end(), childID);
	if (it != m_rootID.end())
	{
		ALICE_LOG_WARN("[UIWorld] CreateChildEntity: childID=%lu was in m_rootID, removing it", childID);
		m_rootID.erase(it);
	}

	T* ObjPtr = pUIObj.get();
	
	// emplace 전에 중복 확인 (ID 충돌 방지)
	if (pUIObjStorage.find(childID) != pUIObjStorage.end())
	{
		
		assert(false && "CreateChildEntity: ID collision detected");
		return nullptr;
	}
	
	pUIObjStorage.emplace(childID, std::move(pUIObj));

	ALICE_LOG_INFO("[UIWorld] CreateChildEntity: child stored successfully - childID=%lu, parentID=%lu, type=%s", 
		childID, parentID, typeName);

	// 자식도 초기화
	ObjPtr->Initalize(*m_UIRenderStruct, m_worldDelegates);

	return ObjPtr;
}

// 부모 설정
template<typename T, typename K>
	requires std::derived_from<T, UIBase> || std::derived_from<K, UIBase>
void UIWorld::SetParent(T* parentUI, K* childUI)
{
	assert(parentUI && childUI && "SetParent: parentUI and childUI must not be null");

	// 기존 부모가 있으면 그 부모의 children에서 제거
	if (childUI->parentID != 0)
	{
		UIBase* oldParent = Get(childUI->parentID);
		if (oldParent)
		{
			auto& oldParentChildren = oldParent->childIDStorage;
			oldParentChildren.erase(
				std::remove(oldParentChildren.begin(), oldParentChildren.end(), childUI->ID),
				oldParentChildren.end()
			);
		}
	}

	// 새로운 부모 연결
	parentUI->childIDStorage.push_back(childUI->ID);
	childUI->parentID = parentUI->ID;

	// m_rootID에서 child 제거 (이미 부모가 있으므로 root가 아님)
	m_rootID.erase(std::remove_if(m_rootID.begin(), m_rootID.end(),
		[&](long unsigned ID) {
			return ID == childUI->ID;
		}), m_rootID.end());
}


// 
template<class T>
	requires std::derived_from<T, IUIComponent>
T* UIWorld::TryGetComponent(unsigned long ownerID)
{
	// if constexpr (std::is_same_v<T, UITextComponent>)
	// 	return FindTextComponent(ownerID);

	if constexpr (std::is_same_v<T, UITransform>)
		return FindTransformComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ImageComponent>)
		return FindImageComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ScriptComponent>)
		return FindScriptComponent(ownerID);

	// Delegates를 통한 조회
	if (!m_worldDelegates.FindComponent.IsBound()) return nullptr;
	void* raw = m_worldDelegates.FindComponent.Execute(ownerID, typeid(T));
	return static_cast<T*>(raw);
}

template<class T>
	requires std::derived_from<T, IUIComponent>
const T* UIWorld::TryGetComponent(unsigned long ownerID) const
{
	if constexpr (std::is_same_v<T, UITransform>)
		return FindTransformComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ImageComponent>)
		return FindImageComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ScriptComponent>)
		return FindScriptComponent(ownerID);

	// Delegates를 통한 조회
	if (!m_worldDelegates.FindComponent.IsBound()) return nullptr;
	void* raw = m_worldDelegates.FindComponent.Execute(ownerID, typeid(T));
	return static_cast<const T*>(raw);
}

template<class T>
	requires std::derived_from<T, IUIComponent>
T& UIWorld::GetComponent(unsigned long ownerID)
{
	T* p = TryGetComponent<T>(ownerID);
	assert(p && "Component not found");
	return *p;
}

template<class T>
	requires std::derived_from<T, IUIComponent>
void UIWorld::RemoveComponent(unsigned long ownerID)
{
	// if constexpr (std::is_same_v<T, UITextComponent>)
	// {
	// 	RemoveTextComponent(ownerID);
	// 	return;
	// }

	if constexpr (std::is_same_v<T, UITransform>)
	{
		RemoveTransformComponent(ownerID);
		return;
	}

	if constexpr (std::is_same_v<T, UI_ImageComponent>)
	{
		RemoveImageComponent(ownerID);
		return;
	}

	if constexpr (std::is_same_v<T, UI_ScriptComponent>)
	{
		RemoveScriptComponent(ownerID);
		return;
	}

	if (m_worldDelegates.RemoveComponent.IsBound())
		m_worldDelegates.RemoveComponent.Execute(ownerID, typeid(T));
}

template<class T, class... Args>
	requires std::derived_from<T, IUIComponent>
T* UIWorld::CreateComponent(unsigned long ownerID, Args&&... args)
{
	// if constexpr (std::is_same_v<T, UITextComponent>)
	// 	return CreateTextComponent(ownerID);

	if constexpr (std::is_same_v<T, UITransform>)
		return CreateTransformComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ImageComponent>)
		return CreateImageComponent(ownerID);

	if constexpr (std::is_same_v<T, UI_ScriptComponent>)
		return CreateScriptComponent(ownerID);

	assert(m_worldDelegates.AddComponent.IsBound() && "World AddComponent delegate not bound");
	auto factory = [&]() -> void*
		{
			return static_cast<void*>(new T(std::forward<Args>(args)...));
		};
	void* raw = m_worldDelegates.AddComponent.Execute(ownerID, typeid(T), factory);
	return static_cast<T*>(raw);
}


