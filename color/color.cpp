#define WIN32_LEAN_AND_MEAN
#include<windows.h>

#include<wrl.h>
using Microsoft::WRL::ComPtr;

#include<tchar.h>
#include<string>
using std::string;
using std::wstring;
#ifdef UNICODE
typedef wstring TSTRING;
#else 
typedef  string TSTRING;
#endif // UNICODE

#include<dxgi1_6.h>
#ifdef _DEBUG
#include<dxgidebug.h>
#endif // _DEBUG

#include<d3d12.h>
#include<d3dcompiler.h>
#include "../d3dx12.h"

#include<DirectXMath.h>

#include<wincodec.h>
//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

//using namespace DirectX;


class COMException {
public:
	COMException(HRESULT hr, INT Line) : hrError(hr), Line(Line) {}
	HRESULT Error() const { return hrError; }
	HRESULT hrError = S_OK;
	INT Line = -1;
};
#define THROW_IF_FAILED(hr)					{HRESULT _hr=(hr);if(FAILED(_hr))throw COMException(_hr,__LINE__);}
#define RETURN_IF_FAILED(hr)				{HRESULT _hr=(hr);if(FAILED(_hr))return _hr;}

#define CLASS_NAME							_T("Window Class name")
#define WINDOW_NAME							_T("WIndow name")

//Graphics
LONG										LClientWidth = 800;
LONG										LClientHeight = 600;

//Path
TSTRING										strCatallogPath = {};

//Window
WNDCLASSEX									stWndClassEx = {};
HWND										hWnd = nullptr;
DWORD										dwStyle = WS_OVERLAPPEDWINDOW;
DWORD										dwExstyle = WS_EX_LEFT;

//DXGI Factory
ComPtr<IDXGIFactory6>						pIDXGIFactory6;
UINT										nDXGIFactoryFlags = 0;

//DXGI Adapter
ComPtr<IDXGIAdapter1>						pIDXGIAdapter1;

//D3D12 Device
ComPtr<ID3D12Device4>						pID3D12Device4;
D3D_FEATURE_LEVEL							enD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;

//D3D12 Command Queue
ComPtr<ID3D12CommandQueue>					pID3D12CommandQueue;
D3D12_COMMAND_QUEUE_DESC					stD3D12CommandQueueDesc = {};

//DXGI Swap Chain
ComPtr<IDXGISwapChain3>						pIDXGISwapChain3;
DXGI_SWAP_CHAIN_DESC1						stDXGISwapChainDesc1 = {};
DXGI_FORMAT									enDXGISwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_SAMPLE_DESC							stDXGISwapChainSampleDesc = {};
const UINT									nBackBufferCount = 3;

//RTV Heap
ComPtr<ID3D12DescriptorHeap>				pID3D12RTVHeap;
D3D12_DESCRIPTOR_HEAP_DESC					stD3D12RTVHeapDesc = {};
UINT										nRTVHandleIncrementSize = 0;

//Create RTVs
ComPtr<ID3D12Resource>						pID3D12RenderTargetsResource[nBackBufferCount];
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPURTVHandle = {};

//Command Allocator
ComPtr<ID3D12CommandAllocator>				pID3D12CommandAllocator;

//Graphics Pipelein State
ComPtr<ID3D12PipelineState>					pID3D12PipelineState;

//Graphics Command List
ComPtr<ID3D12GraphicsCommandList>			pID3D12GraphicsCommandList;

//Resource Barrier
D3D12_RESOURCE_BARRIER						stD3D12BeginResourceBarrier = {};
D3D12_RESOURCE_BARRIER						stD3D12EndResourceBarrier = {};

//Fence
ComPtr<ID3D12Fence>						pID3D12Fence;
UINT64									n64D3D12FenceCPUValue = 0Ui64;

//Fence Event
HANDLE									hFenceEvent = nullptr;

//Clear
float color[4];
bool isRAdd = true;
bool isGAdd = true;
bool isBAdd = true;


LRESULT	CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT Get_Path() {
	TCHAR path[MAX_PATH] = {};
	if (!GetModuleFileName(nullptr, path, MAX_PATH))
		RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	strCatallogPath = path;
	//Get Catallog
	{
		//erase "fimename.exe"
		strCatallogPath.erase(strCatallogPath.find_last_of(_T('\\')));
		//erase "x64"
		strCatallogPath.erase(strCatallogPath.find_last_of(_T("\\")));
		//erase "Debug/Release"
		strCatallogPath.erase(strCatallogPath.find_last_of(_T("\\")) + 1);
	}
	return S_OK;
}

HRESULT Window_Initialize(_In_ HINSTANCE hInstance, _In_ int nCmdShow) {
	//register window class
	{
		stWndClassEx.cbSize = sizeof(WNDCLASSEX);
		stWndClassEx.style = CS_GLOBALCLASS;
		stWndClassEx.lpfnWndProc = WndProc;
		stWndClassEx.cbClsExtra = 0;
		stWndClassEx.cbWndExtra = 0;
		stWndClassEx.hInstance = hInstance;
		stWndClassEx.hIcon = nullptr;
		stWndClassEx.hCursor = static_cast<HCURSOR> (LoadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
		stWndClassEx.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
		stWndClassEx.lpszMenuName = nullptr;
		stWndClassEx.lpszClassName = CLASS_NAME;
		stWndClassEx.hIconSm = nullptr;
		if (!RegisterClassEx(&stWndClassEx))
			RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	}
	//Window Rectangle Middle Position
	RECT rectWnd = { 0,0,LClientWidth,LClientHeight };
	{
		if (AdjustWindowRectEx(&rectWnd, dwStyle, TRUE, dwExstyle) == TRUE)
			RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	}
	INT iScreenLeftTopX = (GetSystemMetrics(SM_CXSCREEN) - (rectWnd.right - rectWnd.left)) / 2;
	INT iScreenLeftTopY = (GetSystemMetrics(SM_CYSCREEN) - (rectWnd.bottom - rectWnd.top)) / 2;
	//Create Window
	{
		hWnd = CreateWindowEx(dwExstyle
			, CLASS_NAME
			, WINDOW_NAME
			, dwStyle
			, iScreenLeftTopX
			, iScreenLeftTopY
			, rectWnd.right - rectWnd.left
			, rectWnd.bottom - rectWnd.top
			, nullptr
			, nullptr
			, hInstance
			, nullptr
		);
	}
	if (!hWnd)
		return E_HANDLE;
	return S_OK;
}

HRESULT Set_D3D12_Debug() {
#ifdef _DEBUG
	ComPtr<ID3D12Debug> pID3D12Debug;
	RETURN_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12Debug)));
	pID3D12Debug->EnableDebugLayer();
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG
	return S_OK;
}

HRESULT Create_DXGI_Factory() {
	RETURN_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory6)));
	RETURN_IF_FAILED(pIDXGIFactory6->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
	return S_OK;
}

HRESULT Create_DXGI_Adapter() {
	RETURN_IF_FAILED(pIDXGIFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pIDXGIAdapter1)));
	return S_OK;
}

HRESULT Create_D3D12_Device() {
	RETURN_IF_FAILED(D3D12CreateDevice(pIDXGIAdapter1.Get(), enD3DFeatureLevel, IID_PPV_ARGS(&pID3D12Device4)));
	return S_OK;
}

HRESULT Create_D3D12_Command_Queue() {
	//Command Queue Struct
	{
		stD3D12CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		stD3D12CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		stD3D12CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		stD3D12CommandQueueDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateCommandQueue(&stD3D12CommandQueueDesc, IID_PPV_ARGS(&pID3D12CommandQueue)));
	return S_OK;
}

HRESULT Create_DXGI_SwapChain() {
	//Sample Desc
	{
		stDXGISwapChainSampleDesc.Count = 1;
		stDXGISwapChainSampleDesc.Quality = 0;
	}
	//Swap Chain Desc
	{
		stDXGISwapChainDesc1.Width = LClientWidth;
		stDXGISwapChainDesc1.Height = LClientHeight;
		stDXGISwapChainDesc1.Format = enDXGISwapChainFormat;
		stDXGISwapChainDesc1.Stereo = FALSE;
		stDXGISwapChainDesc1.SampleDesc = stDXGISwapChainSampleDesc;
		stDXGISwapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		stDXGISwapChainDesc1.BufferCount = nBackBufferCount;
		stDXGISwapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		stDXGISwapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		stDXGISwapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		stDXGISwapChainDesc1.Flags = 0;//No Flag
	}
	//Create
	{
		ComPtr<IDXGISwapChain1> pIDXGISwapChain1;
		RETURN_IF_FAILED(pIDXGIFactory6->CreateSwapChainForHwnd(pID3D12CommandQueue.Get()
			, hWnd
			, &stDXGISwapChainDesc1
			, nullptr
			, nullptr
			, &pIDXGISwapChain1
		));
		pIDXGISwapChain1.As(&pIDXGISwapChain3);
	}
	return S_OK;
}

HRESULT Create_D3D12_RTV_Heap() {
	//RTV Struct
	{
		stD3D12RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		stD3D12RTVHeapDesc.NumDescriptors = nBackBufferCount;
		stD3D12RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		stD3D12RTVHeapDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12RTVHeapDesc, IID_PPV_ARGS(&pID3D12RTVHeap)));
	nRTVHandleIncrementSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return S_OK;
}

HRESULT Create_D3D12_RTVs() {
	stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT Idx = 0; Idx < nBackBufferCount; ++Idx) {
		RETURN_IF_FAILED(pIDXGISwapChain3->GetBuffer(Idx, IID_PPV_ARGS(&pID3D12RenderTargetsResource[Idx])));
		pID3D12Device4->CreateRenderTargetView(pID3D12RenderTargetsResource[Idx].Get(), nullptr, stD3D12CPURTVHandle);
		stD3D12CPURTVHandle.ptr += nRTVHandleIncrementSize;
	}
	return S_OK;
}

HRESULT Create_D3D12_Command_Allocator() {
	RETURN_IF_FAILED(pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pID3D12CommandAllocator)));
	return S_OK;
}

HRESULT Create_D3D12_Graphics_Command_List() {
	RETURN_IF_FAILED(pID3D12Device4->CreateCommandList(0
		, D3D12_COMMAND_LIST_TYPE_DIRECT
		, pID3D12CommandAllocator.Get()
		, nullptr
		, IID_PPV_ARGS(&pID3D12GraphicsCommandList)
	));
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Close());
	return S_OK;
}

HRESULT Create_D3D12_Fence() {
	RETURN_IF_FAILED(pID3D12Device4->CreateFence(n64D3D12FenceCPUValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pID3D12Fence)));
	++n64D3D12FenceCPUValue;
	return S_OK;
}

HRESULT Create_D3D12_Fence_Event() {
	hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hFenceEvent == nullptr)
		THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	SetEvent(hFenceEvent);
	return S_OK;
}

HRESULT Create_D3D12_Resource_Barrier() {
	//begin
	{
		stD3D12BeginResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stD3D12BeginResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		stD3D12BeginResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		stD3D12BeginResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		stD3D12BeginResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	}
	//end
	{
		stD3D12EndResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stD3D12EndResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		stD3D12EndResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		stD3D12EndResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		stD3D12EndResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	}
	return S_OK;
}

void OnUpdate()
{

	if (color[0] <= 1.0f && isRAdd)
	{
		color[0] += 0.001f;
		isRAdd = true;
	}
	else
	{
		color[0] -= 0.002f;
		color[0] <= 0 ? isRAdd = true : isRAdd = false;

	}

	if (color[1] <= 1.0f && isGAdd)
	{
		color[1] += 0.002f;
		isGAdd = true;
	}
	else
	{
		color[1] -= 0.001f;
		color[1] <= 0 ? isGAdd = true : isGAdd = false;

	}

	if (color[2] <= 1.0f && isBAdd)
	{
		color[2] += 0.001f;
		isBAdd = true;
	}
	else
	{
		color[2] -= 0.001f;
		color[2] <= 0 ? isBAdd = true : isBAdd = false;

	}

	color[3] = 1.0f;

}

HRESULT Load_Pipeline() {
	RETURN_IF_FAILED(Set_D3D12_Debug());
	RETURN_IF_FAILED(Create_DXGI_Factory());
	RETURN_IF_FAILED(Create_DXGI_Adapter());
	RETURN_IF_FAILED(Create_D3D12_Device());
	RETURN_IF_FAILED(Create_D3D12_Command_Queue());
	RETURN_IF_FAILED(Create_DXGI_SwapChain());
	RETURN_IF_FAILED(Create_D3D12_RTV_Heap());
	RETURN_IF_FAILED(Create_D3D12_RTVs());
	RETURN_IF_FAILED(Create_D3D12_Command_Allocator());
	return S_OK;
}

HRESULT Load_Asset() {
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List());
	RETURN_IF_FAILED(Create_D3D12_Fence());
	RETURN_IF_FAILED(Create_D3D12_Fence_Event());
	RETURN_IF_FAILED(Create_D3D12_Resource_Barrier());
	return S_OK;
}

HRESULT Populate_CommandList() {
	RETURN_IF_FAILED(pID3D12CommandAllocator->Reset());
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Reset(pID3D12CommandAllocator.Get(), pID3D12PipelineState.Get()));

	UINT nFrameIndex = pIDXGISwapChain3->GetCurrentBackBufferIndex();
	//Begin
	stD3D12BeginResourceBarrier.Transition.pResource = pID3D12RenderTargetsResource[nFrameIndex].Get();
	pID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12BeginResourceBarrier);

	//Render Target
	stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
	stD3D12CPURTVHandle.ptr += nFrameIndex * nRTVHandleIncrementSize;

	const float ClearColor[] = { color[0], color[1], color[2], color[3] };
	pID3D12GraphicsCommandList->ClearRenderTargetView(stD3D12CPURTVHandle, ClearColor, 0, nullptr);

	//End
	stD3D12EndResourceBarrier.Transition.pResource = pID3D12RenderTargetsResource[nFrameIndex].Get();
	pID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12EndResourceBarrier);

	//Close
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Close());
	return S_OK;
}

HRESULT WaitForPreviousFrame() {
	RETURN_IF_FAILED(pID3D12CommandQueue->Signal(pID3D12Fence.Get(), n64D3D12FenceCPUValue));
	RETURN_IF_FAILED(pID3D12Fence->SetEventOnCompletion(n64D3D12FenceCPUValue++, hFenceEvent));
	WaitForSingleObject(hFenceEvent, INFINITE);
	return S_OK;
}

void OnRender() {
	try {
		THROW_IF_FAILED(Populate_CommandList());
		ID3D12CommandList* ppCommandLists[] = { pID3D12GraphicsCommandList.Get() };
		pID3D12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		THROW_IF_FAILED(pIDXGISwapChain3->Present(1, 0));
		THROW_IF_FAILED(WaitForPreviousFrame());
	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		exit(-1);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_PAINT:
		OnRender();
		OnUpdate();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	try {
		THROW_IF_FAILED(Window_Initialize(hInstance, nCmdShow));
		THROW_IF_FAILED(Load_Pipeline());
		THROW_IF_FAILED(Load_Asset());
		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);
		MSG msg = {};
		while (msg.message != WM_QUIT)
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		return e.Error();
	}

	CloseHandle(hFenceEvent);
	return 0;
}