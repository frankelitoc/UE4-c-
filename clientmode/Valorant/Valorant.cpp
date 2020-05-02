#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <io.h>
#include <cstdint>
#include <cstdlib>
#include <dwmapi.h>
#include <comdef.h> 
#include <winternl.h>

#include <dwmapi.h>
#include <comdef.h> 
#include <d3d9.h>
#include <d3dx9.h>

#include <inttypes.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#include "vector3.h"
#include "driver.h"
#include "process.h"
#include "colors.h"
#include "overlay.h"

#include <random>
#include <vector>
#include "xor.h"

#include "defs.h"

using namespace std;

int Width = GetSystemMetrics(SM_CXSCREEN);
int Height = GetSystemMetrics(SM_CYSCREEN);
const MARGINS Margin = { -1 };
DWORD ScreenCenterX;
DWORD ScreenCenterY;

RECT GameRect = { NULL };
HWND GameWnd = NULL;
void create_console();

HRESULT DirectXInit(HWND hWnd);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
void SetupWindow();
WPARAM MainLoop();
void CleanuoD3D();

int isTopwin();
void SetWindowToTarget();

#define TopWindowGame 11
#define TopWindowMvoe 22

IDirect3D9Ex* p_Object = NULL;
IDirect3DDevice9Ex* p_Device = NULL;
D3DPRESENT_PARAMETERS p_Params = { NULL };

#define M_Name xorstr_(L" ")
HWND MyWnd = NULL;
MSG Message = { NULL };

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define M_PI	3.1415926535
D3DXMATRIX ToMatrix(Vector3 Rotation, Vector3 origin = Vector3(0, 0, 0));
D3DXMATRIX ToMatrix(Vector3 Rotation, Vector3 origin)
{
	float Pitch = (Rotation.x * float(M_PI) / 180.f);
	float Yaw = (Rotation.y * float(M_PI) / 180.f);
	float Roll = (Rotation.z * float(M_PI) / 180.f);

	float SP = sinf(Pitch);
	float CP = cosf(Pitch);
	float SY = sinf(Yaw);
	float CY = cosf(Yaw);
	float SR = sinf(Roll);
	float CR = cosf(Roll);

	D3DXMATRIX Matrix;
	Matrix._11 = CP * CY;
	Matrix._12 = CP * SY;
	Matrix._13 = SP;
	Matrix._14 = 0.f;

	Matrix._21 = SR * SP * CY - CR * SY;
	Matrix._22 = SR * SP * SY + CR * CY;
	Matrix._23 = -SR * CP;
	Matrix._24 = 0.f;

	Matrix._31 = -(CR * SP * CY + SR * SY);
	Matrix._32 = CY * SR - CR * SP * SY;
	Matrix._33 = CR * CP;
	Matrix._34 = 0.f;

	Matrix._41 = origin.x;
	Matrix._42 = origin.y;
	Matrix._43 = origin.z;
	Matrix._44 = 1.f;

	return Matrix;
}
Vector3 WorldToScreen(Vector3 world_location, Vector3 position, Vector3 rotation, float fov)
{
	Vector3 screen_location = Vector3(0, 0, 0);

	D3DMATRIX tempMatrix = ToMatrix(rotation);

	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	Vector3 vDelta = world_location - position;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float FovAngle = fov;
	float ScreenCenterX = Width / 2.0f;
	float ScreenCenterY = Height / 2.0f;

	screen_location.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	screen_location.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;

	return screen_location;
}

D3DMATRIX MatrixMultiplication(D3DMATRIX pM1, D3DMATRIX pM2)
{
	D3DMATRIX pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}

#define ReadInteger(addr)  read<int>(addr)
#define RPMFT(addr)   read<FTransform>(addr)
#define RPMDPTR(addr)   read<DWORD_PTR>(addr)

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = RPMDPTR(mesh + 0x878, 0x8);
	return RPMFT(bonearray + (index * 0x30), sizeof(FTransform));
}
Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = RPMFT(mesh + 0x1D0, sizeof(FTransform));
	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());
	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

typedef struct _TslEntity
{
	uint64_t pObjPointer;
	int ID;

	uint64_t PlayerController;
	uint64_t mesh;

}TslEntity;
vector<TslEntity> entityList;

char* drawBuff = (char*)malloc(1024);
D3DXVECTOR4 Rect;

uint64_t world;
uint64_t persistent_level;
uint64_t game_instance;
uint64_t local_player_array;
uint64_t local_player;
uint64_t local_player_controller;
uint64_t local_player_pawn;
uint64_t damage_controller;
float health;
uint64_t camera_manager;
Vector3 camera_position;
Vector3 camera_rotation;
float camera_fov;
uint64_t actors;
int actor_count;
Vector3 LocalPos;
uint64_t LocalRoot;


void cache()
{
	while (true)
	{
		vector<TslEntity> tmpList;

		world = read<std::uint64_t>(m_base + 0x5DD8EF8);
		persistent_level = read<std::uint64_t>(world + 0x38);
		game_instance = read<std::uint64_t>(world + 0x180);

		local_player_array = read<std::uint64_t>(game_instance + 0x40);
		local_player = read<std::uint64_t>(local_player_array);
		local_player_controller = read<std::uint64_t>(local_player + 0x38);
		local_player_pawn = read<std::uint64_t>(local_player_controller + 0x518);

		camera_manager = read<std::uint64_t>(local_player_controller + 0x530);

		actors = read<std::uint64_t>(persistent_level + 0xB0);
		actor_count = read<int>(persistent_level + 0xB8);

		LocalRoot = read<std::uint64_t>(local_player_pawn + 0x238);
		LocalPos = read<Vector3>(LocalRoot + 0x1E0);

		damage_controller = read<std::uint64_t>(local_player_pawn + 0xAF8);

		for (unsigned long i = 0; i < actor_count; ++i)
		{
			std::uint64_t actor = read<std::uint64_t>(actors + i * 0x8);

			if (actor == 0x00)
			{
				continue;
			}

			

			int ID = read<int>(actor + 0x18);

			TslEntity tslEntity{ };
			tslEntity.pObjPointer = actor;
			tslEntity.ID = ID;
			tslEntity.PlayerController = local_player_controller;

			uint64_t mesh = read<uint64_t>(actor + 0x4F0);

			int UniqueID = read<int>(actor + 0x3C);
			if (UniqueID != 16777502)
				continue;

			if (mesh != 0x00 && UniqueID != 0x00)
			{
				tslEntity.mesh = mesh;

				tmpList.push_back(tslEntity);
			}
		}
		entityList = tmpList;
		Sleep(1);
	}
}

void ESP() {

	int revise = 0;
	auto entityListCopy = entityList;
	float distance = 0.f;

	if (entityListCopy.empty())
	{
		return;
	}

	for (unsigned long i = 0; i < entityListCopy.size(); ++i)
	{
		TslEntity entity = entityListCopy[i];
		bool is_alive = read<bool>(damage_controller + 0x425);
		health = read<float>(damage_controller + 0x1B0);

		if (entity.pObjPointer == local_player_pawn)
		{
			continue;
		}

		if (entity.pObjPointer == is_alive)
		{
			continue;
		}

		

		camera_position = read<Vector3>(camera_manager + 0x1260);

		camera_rotation = read<Vector3>(camera_manager + 0x126C);
		camera_fov = read<float>(camera_manager + 0x4B0) + 33.0f;

		auto root_component = read<std::uint64_t>(entity.pObjPointer + 0x238);
		auto RootPos = read<Vector3>(root_component + 0x1E0);

		wchar_t pPlayerName[64] = { NULL };

		auto ActorPlayerState = read<uint64_t>(entity.pObjPointer + 0x4B0);

		readwtf(ActorPlayerState + 0x490, &pPlayerName, sizeof(pPlayerName));

		uint64_t mesh = entity.mesh;

		distance = LocalPos.Distance(RootPos) / 100.f;

		if (mesh != 0x00)
		{
			Vector3 vHeadBone = GetBoneWithRotation(mesh, 8);
			Vector3 RootPos2d = WorldToScreen(RootPos, camera_position, camera_rotation, camera_fov);

			sprintf(drawBuff, u8" %s %0.fm ", pPlayerName, distance);

			if (vHeadBone.z <= RootPos.z)
				continue;

			revise = strlen(drawBuff) * 7 + 28;
			DrawFilledRect(RootPos2d.x - (revise / 2) + (Rect.w / 2) - 1, RootPos2d.y + Rect.z - 25, revise + 1, 3, &Col.blue);
			DrawFilledRect(RootPos2d.x - (revise / 2) + (Rect.w / 2) - 1, RootPos2d.y + Rect.z - 28, revise + 1, 16, &Col.blue);
			DrawNewText(RootPos2d.x - (revise / 2) + (Rect.w / 2) + 21, RootPos2d.y + Rect.z - 27, &Col.white_, drawBuff);

			if (health >= 100)
				health = 100;
			int length = (int)(health * revise / 100);

			if (health >= 80)
				DrawFilledRect(RootPos2d.x - (revise / 2) + (Rect.w / 2), RootPos2d.y + Rect.z - 13, length, 1, &Col.greens_);
			else if (health < 80 && health > 50)
				DrawFilledRect(RootPos2d.x - (revise / 2) + (Rect.w / 2), RootPos2d.y + Rect.z - 13, length, 1, &Col.red_);
			else if (health <= 50)
				DrawFilledRect(RootPos2d.x - (revise / 2) + (Rect.w / 2), RootPos2d.y + Rect.z - 13, length, 1, &Col.gray_);
		}
	}
}

bool verify_game()
{
	m_pid = PIDManager::GetAowProcId();

	if (!m_pid)
		return false;

	Controller = new DriverController(m_pid);

	usermode_pid = GetCurrentProcessId();

	if (!usermode_pid)
		return false;

	printf(("> usermode_pid: %d\n"), usermode_pid);

	m_base = GetBaseAddress();

	if (!m_base)
		return false;

	printf(("> m_pid: %d base: %llx\n"), m_pid, m_base);

	return true;
}
HRESULT DirectXInit(HWND hWnd)
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object)))
		exit(3);

	D3DPRESENT_PARAMETERS p_Params = { 0 };
	p_Params.Windowed = TRUE;
	p_Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	p_Params.hDeviceWindow = hWnd;
	p_Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	p_Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	p_Params.BackBufferWidth = Width;
	p_Params.BackBufferHeight = Height;
	p_Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	p_Params.EnableAutoDepthStencil = TRUE;
	p_Params.AutoDepthStencilFormat = D3DFMT_D16;
	p_Params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (FAILED(p_Object->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p_Params, 0, &p_Device)))
	{
		p_Object->Release();
		exit(4);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(p_Device);

	ImGui::StyleColorsClassic();
	ImGuiStyle* style = &ImGui::GetStyle();
	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

	style->WindowTitleAlign.x = 0.50f;
	style->FrameRounding = 2.0f;

	p_Object->Release();
	return S_OK;
}
void SetupWindow()
{
	//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SetWindowToTarget, 0, 0, 0);

	WNDCLASSEX wClass =
	{
		sizeof(WNDCLASSEX),
		0,
		WinProc,
		0,
		0,
		nullptr,
		LoadIcon(nullptr, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		TEXT("Test1"),
		LoadIcon(nullptr, IDI_APPLICATION)
	};

	if (!RegisterClassEx(&wClass))
		exit(1);

	GameWnd = FindWindowW(NULL, TEXT("VALORANT  "));

	printf("GameWnd Found! : %p", GameWnd);

	if (GameWnd)
	{
		GetClientRect(GameWnd, &GameRect);
		POINT xy;
		ClientToScreen(GameWnd, &xy);
		GameRect.left = xy.x;
		GameRect.top = xy.y;

		Width = GameRect.right;
		Height = GameRect.bottom;
	}
	else exit(2);

	MyWnd = CreateWindowExA(NULL, "Test1", "Test1", WS_POPUP | WS_VISIBLE, GameRect.left, GameRect.top, Width, Height, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(MyWnd, &Margin);
	SetWindowLong(MyWnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(MyWnd, SW_SHOW);
	UpdateWindow(MyWnd);

	printf("Hwnd created : %p", MyWnd);

}
void render() {


	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	char fpsinfo[64];
	char lol[64];
	char ent[64];
	char ents[64];
	sprintf(fpsinfo, xorstr_("Overlay FPS: %0.f"), ImGui::GetIO().Framerate);
	DrawStrokeText(30, 44, &Col.red, fpsinfo);

	ESP();

	ImGui::EndFrame();
	p_Device->SetRenderState(D3DRS_ZENABLE, false);
	p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	p_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	p_Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (p_Device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		p_Device->EndScene();
	}
	HRESULT result = p_Device->Present(NULL, NULL, NULL, NULL);

	if (result == D3DERR_DEVICELOST && p_Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		p_Device->Reset(&p_Params);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}
WPARAM MainLoop()
{
	static RECT old_rc;
	ZeroMemory(&Message, sizeof(MSG));

	while (Message.message != WM_QUIT)
	{
		if (PeekMessage(&Message, MyWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		HWND hwnd_active = GetForegroundWindow();
		if (GetAsyncKeyState(0x23) & 1)
			exit(8);

		if (hwnd_active == GameWnd) {
			HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
			SetWindowPos(MyWnd, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		RECT rc;
		POINT xy;

		ZeroMemory(&rc, sizeof(RECT));
		ZeroMemory(&xy, sizeof(POINT));
		GetClientRect(GameWnd, &rc);
		ClientToScreen(GameWnd, &xy);
		rc.left = xy.x;
		rc.top = xy.y;

		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = GameWnd;
		io.DeltaTime = 1.0f / 60.0f;

		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x - xy.x;
		io.MousePos.y = p.y - xy.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else
			io.MouseDown[0] = false;
		if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom)
		{

			old_rc = rc;

			Width = rc.right;
			Height = rc.bottom;

			p_Params.BackBufferWidth = Width;
			p_Params.BackBufferHeight = Height;
			SetWindowPos(MyWnd, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
			p_Device->Reset(&p_Params);
		}



		render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanuoD3D();
	DestroyWindow(MyWnd);

	return Message.wParam;
}
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message)
	{
	case WM_DESTROY:
		CleanuoD3D();
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (p_Device != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
			p_Params.BackBufferWidth = LOWORD(lParam);
			p_Params.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = p_Device->Reset(&p_Params);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}
void CleanuoD3D()
{
	if (p_Device != NULL)
	{
		p_Device->EndScene();
		p_Device->Release();
	}
	if (p_Object != NULL)
	{
		p_Object->Release();
	}
}
int isTopwin()
{
	HWND hWnd = GetForegroundWindow();
	if (hWnd == GameWnd)
		return TopWindowGame;
	if (hWnd == MyWnd)
		return TopWindowMvoe;

	return 0;
}
void SetWindowToTarget()
{
	while (true)
	{
		GameWnd = FindWindowW(TEXT("UnrealWindow"), NULL);
		if (GameWnd)
		{
			ZeroMemory(&GameRect, sizeof(GameRect));
			GetWindowRect(GameWnd, &GameRect);
			Width = GameRect.right - GameRect.left;
			Height = GameRect.bottom - GameRect.top;
			DWORD dwStyle = GetWindowLong(GameWnd, GWL_STYLE);
			if (dwStyle & WS_BORDER)
			{
				GameRect.top += 32;
				Height -= 39;
			}
			ScreenCenterX = Width / 2;
			ScreenCenterY = Height / 2;
			MoveWindow(MyWnd, GameRect.left, GameRect.top, Width, Height, true);
		}
		Sleep(1);
	}
}
static const char consoleNameAlphanum[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
int consoleNameLength = sizeof(consoleNameAlphanum) - 1;
char genRandomConsoleName()
{
	return consoleNameAlphanum[rand() % consoleNameLength];
}
void create_console()
{
	if (!AllocConsole())
	{
		char buffer[1024] = { 0 };
		return;
	}

	auto lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	auto hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	auto fp = _fdopen(hConHandle, xorstr_("w"));

	freopen_s(&fp, xorstr_("CONOUT$"), xorstr_("w"), stdout);

	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	create_console();

	HWND ConsWind = GetConsoleWindow();
	srand(time(0));
	std::wstring ConsoleNameStr;

	for (unsigned int i = 0; i < 20; ++i)
	{
		ConsoleNameStr += genRandomConsoleName();
	}

	SetConsoleTitle(ConsoleNameStr.c_str());

	printf("[>] seting up window\n");

	SetupWindow();

	printf("[>] Initializing DX (overlay %d)\n", MyWnd);

	DirectXInit(MyWnd);

	printf("[>] obtaining game data\n");

	verify_game();

	printf("[>] Done\n");

	HANDLE hdl = CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(cache), nullptr, NULL, nullptr);

	printf("[>] Cache thread started\n");

	CloseHandle(hdl);

	printf("[>] entering main loop for rendering\n");

	while (TRUE)
	{

		MainLoop();
	}

	printf("main loop exited");

	return 0;
}