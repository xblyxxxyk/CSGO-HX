#include "includes.h"
#include "Windows.h"
#include <vector>
#include <iostream>
#include "vector.h"
#include <thread>
#include <iomanip>
#ifdef _WIN64
#define GWL_WNDPROC GWLP_WNDPROC
#endif


namespace offset {
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDEF97C;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4E051DC;
	constexpr ::std::ptrdiff_t m_iGlowIndex = 0x10488;
	constexpr ::std::ptrdiff_t dwGlowObjectManager = 0x535FCB8;
	constexpr ::std::ptrdiff_t dwClientState = 0x59F19C;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;

	constexpr ::std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
	constexpr ::std::ptrdiff_t m_bDormant = 0xED;
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_iHealth = 0x100;
	constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr ::std::ptrdiff_t m_vecViewOffset = 0x108;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr ::std::ptrdiff_t m_bSpottedByMask = 0x980;
	constexpr ::std::ptrdiff_t m_bSpotted = 0x93D;
}

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

DWORD GetPointerAddress(DWORD ptr, std::vector<DWORD> offsets)
{

	DWORD addr = ptr;
	for (int i = 0; i < offsets.size(); ++i) {
		addr = *(DWORD*)addr;
		addr += offsets[i];
	}
	return addr;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EndScene oEndScene = NULL;
WNDPROC oWndProc;
static HWND window = NULL;
DWORD csgobase = (DWORD)GetModuleHandle("csgo.exe");
DWORD clientbase = (DWORD)GetModuleHandle("client.dll");
DWORD enginebase = (DWORD)GetModuleHandle("engine.dll");
void InitImGui(LPDIRECT3DDEVICE9 pDevice)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
}
bool Aimbot = false;
float FOV = 50.0f;
int smoothValue = 2;
bool showMenu = true;
bool Radar = false;
bool Wireframe = false;
bool BunnyHop = false;
bool Glow = false;
bool init = false;
bool wasKeyDown = false;


void Holdless() {
	SHORT keyState = GetAsyncKeyState(VK_INSERT);
	bool isKeyDown = (keyState & 0x8000) != 0;

	if (isKeyDown && !wasKeyDown) {
		showMenu = !showMenu;
	}

	wasKeyDown = isKeyDown;
}
ImDrawList* drawList = ImGui::GetBackgroundDrawList();
ImU32 textColor = IM_COL32(255, 255, 255, 255);
ImVec2 textPos(400, 400);
void AimAt() {
	const auto& localPlayer = *reinterpret_cast<std::uintptr_t*>(clientbase + offset::dwLocalPlayer);
	const auto& localTeam = *reinterpret_cast<std::int32_t*>(localPlayer + offset::m_iTeamNum);
	const auto localEyePosition = *reinterpret_cast<Vector3*>(localPlayer + offset::m_vecOrigin) + *reinterpret_cast<Vector3*>(localPlayer + offset::m_vecViewOffset);
	const auto& clientState = *reinterpret_cast<std::uintptr_t*>(enginebase + offset::dwClientState);
	const auto& viewAngles = *reinterpret_cast<Vector3*>(clientState + offset::dwClientState_ViewAngles);
	const auto& aimPunch = *reinterpret_cast<Vector3*>(localPlayer + offset::m_aimPunchAngle) * 2;
	auto RealFOV = FOV;
	auto RealANGLE = Vector3{ };

	for (auto i = 1; i <= 32; i++)
	{
		const auto& player = *reinterpret_cast<std::uintptr_t*>(clientbase + offset::dwEntityList + i * 0x10);

		if (!player)
			continue;
		if (*reinterpret_cast<std::int32_t*>(player + offset::m_iTeamNum) == localTeam)
			continue;
		if (*reinterpret_cast<bool*>(player + offset::m_bDormant))
			continue;
		if (*reinterpret_cast<std::int32_t*>(player + offset::m_iHealth) <= 0)
			continue;
		if (!*reinterpret_cast<bool*>(player + offset::m_bSpottedByMask) || !*reinterpret_cast<bool*>(player + offset::m_bSpotted))
			continue;
		const auto boneMatrix = *reinterpret_cast<std::uintptr_t*>(player + offset::m_dwBoneMatrix);
		const auto playerHeadPosition = Vector3{
			*reinterpret_cast<float*>(boneMatrix + 0x30 * 8 + 0x0C),
			*reinterpret_cast<float*>(boneMatrix + 0x30 * 8 + 0x1C),
			*reinterpret_cast<float*>(boneMatrix + 0x30 * 8 + 0x2C)
		};
		const auto angle = CalculateAngle(
			localEyePosition,
			playerHeadPosition,
			viewAngles + aimPunch
		);

		const auto fov = std::hypot(angle.x, angle.y);

		if (fov < RealFOV)
		{
			RealFOV = fov;
			RealANGLE = angle;
		}
	}

	if (!RealANGLE.IsZero()) {
		Vector3* viewAngles = reinterpret_cast<Vector3*>(clientState + offset::dwClientState_ViewAngles);
		if (viewAngles != nullptr) {
			*viewAngles = RealANGLE;
		}
	}

}
void SetImGuiStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.40f, 0.41f, 0.42f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.51f, 0.52f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.40f, 0.41f, 0.42f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.46f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.36f, 0.37f, 1.00f);

	style.WindowPadding = ImVec2(15, 15);
	style.WindowRounding = 5.0f;
	style.FramePadding = ImVec2(5, 5);
	style.FrameRounding = 4.0f;
	style.ItemSpacing = ImVec2(12, 8);
	style.ItemInnerSpacing = ImVec2(8, 6);
	style.IndentSpacing = 25.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 5.0f;
	style.GrabRounding = 3.0f;
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

}
long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (!init)
	{
		InitImGui(pDevice);
		init = true;
	}
	   Holdless();
	   SetImGuiStyle();
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (showMenu) {
			ImGui::SetNextWindowSize(ImVec2(400, 250));
			ImGui::Begin("CSGO-HXXXXXXXXXXXXX by blyscyk");
			ImGui::Checkbox("Aimbot [CAPS LOCK]", &Aimbot);
			ImGui::SliderFloat("FOV", &FOV, 1.0f, 300.0f);
			ImGui::Checkbox("Wireframe ESP", &Wireframe);
			ImGui::Checkbox("Glow ESP", &Glow);
			ImGui::Checkbox("Radar ESP", &Radar);
			ImGui::Checkbox("Bunny Hop", &BunnyHop);
			ImGui::End();
		}

		BYTE* wire = (BYTE*)GetPointerAddress(clientbase + 0x1D41E8, { 0x0 });
		if (Wireframe) {
			*wire = 50;
		}
		else {
			*wire = 49;
		}
		BYTE* bunny = (BYTE*)GetPointerAddress(clientbase + 0x525EB5C, { 0x0, 0x7E0 });
		if (BunnyHop) {
			*bunny = 33;
		}
		else {
			*bunny = 32;
		}

		if (Aimbot) {
			if (GetAsyncKeyState(VK_CAPITAL)) {
				AimAt();
			}
		}
		if (Glow) {
			const auto localPlayer = *reinterpret_cast<uintptr_t*>(clientbase + offset::dwLocalPlayer);
			const auto glowObject = *reinterpret_cast<uintptr_t*>(clientbase + offset::dwGlowObjectManager);

			for (auto i = 0; i < 64; i++)
			{
				const auto entity = *reinterpret_cast<uintptr_t*>(clientbase + offset::dwEntityList + i * 0x10);
				if (!entity)
					continue;
				if (*reinterpret_cast<uintptr_t*>(entity + offset::m_iTeamNum) == *reinterpret_cast<uintptr_t*>(localPlayer + offset::m_iTeamNum))
					continue;
				const auto glowIndex = *reinterpret_cast<uintptr_t*>(entity + offset::m_iGlowIndex);

				float* Glow1 = reinterpret_cast<float*>(glowObject + (glowIndex * 0x38) + 0x8);
				float* Glow2 = reinterpret_cast<float*>(glowObject + (glowIndex * 0x38) + 0xC);
				float* Glow3 = reinterpret_cast<float*>(glowObject + (glowIndex * 0x38) + 0x10);
				float* Glow4 = reinterpret_cast<float*>(glowObject + (glowIndex * 0x38) + 0x14);
				bool* Glow5 = reinterpret_cast<bool*>(glowObject + (glowIndex * 0x38) + 0x27);
				bool* Glow6 = reinterpret_cast<bool*>(glowObject + (glowIndex * 0x38) + 0x28);
				*Glow1 = 1.0f;
				*Glow2 = 0.0f;
				*Glow3 = 0.0f;
				*Glow4 = 1.0f;
				*Glow5 = true;
				*Glow6 = true;
			}
		}
		if (Radar) {
			const auto localPlayer = *reinterpret_cast<uintptr_t*>(clientbase + offset::dwLocalPlayer);
			const auto localTeam = *reinterpret_cast<uintptr_t*>(clientbase + offset::m_iTeamNum);

			for (auto i = 1; i <= 64; i++) {
				const auto entity = *reinterpret_cast<uintptr_t*>(clientbase + offset::dwEntityList + i * 0x10);
				if (!entity)
					continue;
				if (*reinterpret_cast<uintptr_t*>(entity + offset::m_iTeamNum) == localTeam)
					continue;

				uintptr_t* ApplyRadar = reinterpret_cast<uintptr_t*>(entity + offset::m_bSpotted);
				*ApplyRadar = true;
			}
		}
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		return oEndScene(pDevice);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool attached = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
		{
			kiero::bind(42, (void**)& oEndScene, hkEndScene);
			do
				window = GetProcessWindow();
			while (window == NULL);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
			attached = true;
		}
	} while (!attached);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}
