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

using namespace DirectX;


class COMException {
public:
	COMException(HRESULT hr, INT Line) : hrError(hr), Line(Line) {}
	HRESULT Error() const { return hrError; }
	HRESULT hrError = S_OK;
	INT Line = -1;
};
#define THROW_IF_FAILED(hr)					{HRESULT _hr=(hr);if(FAILED(_hr))throw COMException(_hr,__LINE__);}
#define RETURN_IF_FAILED(hr)				{HRESULT _hr=(hr);if(FAILED(_hr)){::OutputDebugString((std::to_wstring(__LINE__) + _T('\n')).c_str());return _hr;}}

#define CLASS_NAME							_T("Window Class name")
#define WINDOW_NAME							_T("WIndow name")

//Graphics
LONG										LClientWidth = 800;
LONG										LClientHeight = 600;
D3D12_VIEWPORT								stViewPort = { 0.0f, 0.0f, static_cast<float>(LClientWidth), static_cast<float>(LClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
D3D12_RECT									stScissorRect = { 0, 0, static_cast<LONG>(LClientWidth), static_cast<LONG>(LClientHeight) };

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

//DSV Heap
ComPtr<ID3D12DescriptorHeap>				pID3D12DSVHeap;
D3D12_DESCRIPTOR_HEAP_DESC					stD3D12DSVHeapDesc = {};
UINT										nDSVHandleIncrementSize = 0;

//CBV Heap
ComPtr<ID3D12DescriptorHeap>				pID3D12CBVHeap;
D3D12_DESCRIPTOR_HEAP_DESC					stD3D12CBVHeapDesc = {};
UINT										nCBVHandleIncrementSize = 0;

//RTVs
ComPtr<ID3D12Resource>						pID3D12RenderTargetsResource[nBackBufferCount];
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPURTVHandle = {};

//DSV
ComPtr<ID3D12Resource>						pID3D12DepthStencilResource;
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPUDSVHandle = {};
D3D12_RESOURCE_DESC							stD3D12DepthStencilResourceDesc = {};
D3D12_HEAP_PROPERTIES						stD3D12DepthStencilHeapProperties = {};
D3D12_CLEAR_VALUE							stD3D12DepthStencilClearValue = {};
D3D12_DEPTH_STENCIL_VIEW_DESC				stD3D12DepthStencilViewDesc = {};

//CBV
ComPtr<ID3D12Resource>						pID3D12ConstantBufferResource = {};
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPUCBVHandle = {};
D3D12_RESOURCE_DESC							stD3D12ConstantBufferResourceDesc = {};
D3D12_HEAP_PROPERTIES						stD3D12ConstantBufferHeapProperties = {};
D3D12_CONSTANT_BUFFER_VIEW_DESC				stD3D12ConstantBufferViewDesc = {};//in Func

//Command Allocator
ComPtr<ID3D12CommandAllocator>				pID3D12CommandAllocator;

//Root Signature
ComPtr<ID3D12RootSignature>					pID3D12RootSignature;
D3D12_VERSIONED_ROOT_SIGNATURE_DESC			stD3D12VersionedRootSignatureDesc = {};
D3D12_ROOT_SIGNATURE_DESC1					stRootSignatureDesc1 = {};

//Compile
//Compile Shadaer
//Shander
#ifdef _DEBUG
UINT										nD3DCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
UINT										nD3DCompileFlags = 0;
#endif // _DEBUG
TSTRING										strShaderFileName = _T("Shader_Cube.hlsl");
ComPtr<ID3DBlob>							pID3DVertexShaderBlob;//In CPU Memory
ComPtr<ID3DBlob>							pID3DPixelShaderBlob;
D3D12_SHADER_BYTECODE						stD3D12VertexShader = {};
D3D12_SHADER_BYTECODE						stD3D12PixelShader = {};

//Input layout
D3D12_INPUT_LAYOUT_DESC						stD3D12InputLayoutDesc = {};

//Graphics Pipelein State
ComPtr<ID3D12PipelineState>					pID3D12PipelineState;
D3D12_GRAPHICS_PIPELINE_STATE_DESC			stD3D12GraphicsPipelineStateDesc = {};
D3D12_RENDER_TARGET_BLEND_DESC				stD3D12RenderTargetBlendDesc = {};
D3D12_BLEND_DESC							stD3D12BlendDesc = {};
D3D12_RASTERIZER_DESC						stD3D12RasterizerDesc = {};
D3D12_DEPTH_STENCIL_DESC					stD3D12DepthStencilDesc = {};

//Graphics Command List
ComPtr<ID3D12GraphicsCommandList>			pID3D12GraphicsCommandList;

//Vertex Bufffer
ComPtr<ID3D12Resource>						pID3D12VertexBufferResource;
D3D12_RESOURCE_DESC							stD3D12VertexBufferResourceDesc = {};
D3D12_HEAP_PROPERTIES						stD3D12VertexBufferHeapProperties = {};
D3D12_VERTEX_BUFFER_VIEW					stD3D12VertexBufferView = {};

//Index Buffer
ComPtr<ID3D12Resource>						pID3D12IndexBufferResource;
D3D12_RESOURCE_DESC							stD3D12IndexBufferResourceDesc = {};
D3D12_HEAP_PROPERTIES						stD3D12IndexBufferHeapProperties = {};
D3D12_INDEX_BUFFER_VIEW						stD3D12IndexBufferView = {};

//Resource Barrier
D3D12_RESOURCE_BARRIER						stD3D12BeginResourceBarrier = {};
D3D12_RESOURCE_BARRIER						stD3D12EndResourceBarrier = {};

//Fence
ComPtr<ID3D12Fence>							pID3D12Fence;
UINT64										n64D3D12FenceCPUValue = 0Ui64;

//Fence Event
HANDLE										hFenceEvent = nullptr;

//Vertex And Index
struct VERTEX {
	DirectX::XMFLOAT3 Position;		//Position
	DirectX::XMFLOAT4 Color;		//Texcoord
};
VERTEX TriangleVertices[] = {
			{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },//Front Left Bottom
			{ { -1.0f, +1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },//      Left Top
			{ { +1.0f, +1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },//      Right Top
			{ { +1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },//      Right Bootm
			{ { -1.0f, -1.0f, +1.0f }, { 0.0f, 0.5f, 0.5f, 1.0f } },//Back
			{ { -1.0f, +1.0f, +1.0f }, { 0.5f, 0.0f, 0.0f, 1.0f } },
			{ { +1.0f, +1.0f, +1.0f }, { 0.5f, 0.5f, 0.0f, 1.0f } },
			{ { +1.0f, -1.0f, +1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f } }
};
DWORD TriangleIndexs[]={
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
};
UINT										nVertexBufferByteSize = sizeof(TriangleVertices);
UINT										nIndexBufferByteSize = sizeof(TriangleIndexs);

//Constant Value
struct SceneConstantBuffer {
	DirectX::XMFLOAT4X4 MVP;
};
SceneConstantBuffer							stConstantBufferData;
template <typename T>
constexpr UINT CalcConstantBufferByteSize() {
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	UINT byteSize = sizeof(T);
	return (byteSize + 255) & ~255;
}
UINT										nConstantBufferByteSize = 0;
VOID* pCBVData = nullptr;

//Clear
float color[4];
bool isRAdd = true;
bool isGAdd = true;
bool isBAdd = true;

void OnUpdate() {
	if (color[0] <= 1.0f && isRAdd) {
		color[0] += 0.001f;
		isRAdd = true;
	}
	else {
		color[0] -= 0.002f;
		color[0] <= 0 ? isRAdd = true : isRAdd = false;

	}
	if (color[1] <= 1.0f && isGAdd) {
		color[1] += 0.002f;
		isGAdd = true;
	}
	else {
		color[1] -= 0.001f;
		color[1] <= 0 ? isGAdd = true : isGAdd = false;

	}
	if (color[2] <= 1.0f && isBAdd) {
		color[2] += 0.001f;
		isBAdd = true;
	}
	else {
		color[2] -= 0.001f;
		color[2] <= 0 ? isBAdd = true : isBAdd = false;
	}
	color[3] = 1.0f;

	XMVECTOR pos = XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f);
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	XMMATRIX m = XMMatrixIdentity();
	XMMATRIX p = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(LClientWidth) / LClientHeight, 1.0f, 1000.0f);
	XMMATRIX MVP = m * v * p;

	SceneConstantBuffer objConstants;
	XMStoreFloat4x4(&objConstants.MVP, XMMatrixTranspose(MVP));
	memcpy(pCBVData, &objConstants, sizeof(objConstants));
}

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

HRESULT Create_D3D12_DSV_Heap() {
	//DSV HEap Desc
	{
		stD3D12DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		stD3D12DSVHeapDesc.NumDescriptors = 1;
		stD3D12DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		stD3D12DSVHeapDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12DSVHeapDesc, IID_PPV_ARGS(&pID3D12DSVHeap)));
	nDSVHandleIncrementSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//just one DepthStencil
	return S_OK;
}

HRESULT	Create_D3D12_CBV_Heap() {
	//Desc
	{
		stD3D12CBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		stD3D12CBVHeapDesc.NumDescriptors = 1;
		stD3D12CBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		stD3D12CBVHeapDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12CBVHeapDesc, IID_PPV_ARGS(&pID3D12CBVHeap)));
	nDSVHandleIncrementSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

HRESULT Create_D3D12_DSV() {
	//Heap Properties
	{
		stD3D12DepthStencilHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		stD3D12DepthStencilHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12DepthStencilHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12DepthStencilHeapProperties.CreationNodeMask = 0;
		stD3D12DepthStencilHeapProperties.VisibleNodeMask = 0;
	}
	//Respurce Desc
	{
		stD3D12DepthStencilResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stD3D12DepthStencilResourceDesc.Alignment = 0;
		stD3D12DepthStencilResourceDesc.Width = LClientWidth;
		stD3D12DepthStencilResourceDesc.Height = LClientHeight;
		stD3D12DepthStencilResourceDesc.DepthOrArraySize = 1;
		stD3D12DepthStencilResourceDesc.MipLevels = 0;
		stD3D12DepthStencilResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
		stD3D12DepthStencilResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12DepthStencilResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		stD3D12DepthStencilResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	//Clear Value
	{
		stD3D12DepthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		stD3D12DepthStencilClearValue.DepthStencil.Depth = 1.0f;
		stD3D12DepthStencilClearValue.DepthStencil.Stencil = 0;
	}
	//Create Resource
	{
		RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12DepthStencilHeapProperties
			, D3D12_HEAP_FLAG_NONE
			, &stD3D12DepthStencilResourceDesc
			, D3D12_RESOURCE_STATE_DEPTH_WRITE
			, &stD3D12DepthStencilClearValue
			, IID_PPV_ARGS(&pID3D12DepthStencilResource)
		));
	}
	//View Desc
	{
		stD3D12DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		stD3D12DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		stD3D12DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		//tExture2D

	}
	//View
	stD3D12CPUDSVHandle = pID3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
	pID3D12Device4->CreateDepthStencilView(pID3D12DepthStencilResource.Get(), &stD3D12DepthStencilViewDesc, stD3D12CPUDSVHandle);
	return S_OK;
}

HRESULT Create_D3D12_CBV() {
	//Heap Properties
	{
		stD3D12ConstantBufferHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		stD3D12ConstantBufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12ConstantBufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12ConstantBufferHeapProperties.CreationNodeMask = 0;
		stD3D12ConstantBufferHeapProperties.VisibleNodeMask = 0;
	}
	//Resource Desc
	{
		stD3D12ConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stD3D12ConstantBufferResourceDesc.Alignment = 0;
		stD3D12ConstantBufferResourceDesc.Width = nConstantBufferByteSize = CalcConstantBufferByteSize<SceneConstantBuffer>();
		stD3D12ConstantBufferResourceDesc.Height = 1;
		stD3D12ConstantBufferResourceDesc.DepthOrArraySize = 1;
		stD3D12ConstantBufferResourceDesc.MipLevels = 1;
		stD3D12ConstantBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		stD3D12ConstantBufferResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12ConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stD3D12ConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}
	//Create Resource
	{
		RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12ConstantBufferHeapProperties
			, D3D12_HEAP_FLAG_NONE
			, &stD3D12ConstantBufferResourceDesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&pID3D12ConstantBufferResource)
		));
	}
	//Init
	{
		//avoid overhead with mapping/unmapping each frame.
		D3D12_RANGE stReadRange = { 0,0 };
		THROW_IF_FAILED(pID3D12ConstantBufferResource->Map(0, &stReadRange, &pCBVData));
		memcpy(pCBVData, &stConstantBufferData, sizeof(stConstantBufferData));
	}
	//View Desc
	{
		stD3D12ConstantBufferViewDesc.BufferLocation = pID3D12ConstantBufferResource->GetGPUVirtualAddress();
		stD3D12ConstantBufferViewDesc.SizeInBytes = nConstantBufferByteSize;
	}
	//Create View
	stD3D12CPUCBVHandle = pID3D12CBVHeap->GetCPUDescriptorHandleForHeapStart();
	pID3D12Device4->CreateConstantBufferView(&stD3D12ConstantBufferViewDesc, stD3D12CPUCBVHandle);
	return S_OK;
}

HRESULT Create_D3D12_Command_Allocator() {
	RETURN_IF_FAILED(pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pID3D12CommandAllocator)));
	return S_OK;
}

HRESULT Create_D3D12_Root_Signature() {

	stD3D12VersionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	//RootSignature Desc1
	{
		//SRV Range Struct
		D3D12_DESCRIPTOR_RANGE1 stD3D12CBVRange1 = {};
		{
			stD3D12CBVRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			stD3D12CBVRange1.NumDescriptors = 1;
			stD3D12CBVRange1.BaseShaderRegister = 0;
			stD3D12CBVRange1.RegisterSpace = 0;
			stD3D12CBVRange1.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
			stD3D12CBVRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}
		//Root Descriptor Table
		D3D12_ROOT_DESCRIPTOR_TABLE1 stD3D12RootDescriptorTable1 = {};
		{
			stD3D12RootDescriptorTable1.NumDescriptorRanges = 1;
			stD3D12RootDescriptorTable1.pDescriptorRanges = &stD3D12CBVRange1;
		}
		//Root Parameter Struct
		D3D12_ROOT_PARAMETER1 stD3D12RootParameter1 = {};
		{
			stD3D12RootParameter1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stD3D12RootParameter1.DescriptorTable = stD3D12RootDescriptorTable1;
			stD3D12RootParameter1.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}
		stRootSignatureDesc1.NumParameters = 1;
		stRootSignatureDesc1.pParameters = &stD3D12RootParameter1;
		stRootSignatureDesc1.NumStaticSamplers = 0;
		stRootSignatureDesc1.pStaticSamplers = nullptr;
		stRootSignatureDesc1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	}
	stD3D12VersionedRootSignatureDesc.Desc_1_1 = stRootSignatureDesc1;
	ComPtr<ID3DBlob> pID3DSignatureBlob;
	ComPtr<ID3DBlob> pID3DErrorBlob;
	RETURN_IF_FAILED(D3D12SerializeVersionedRootSignature(&stD3D12VersionedRootSignatureDesc, &pID3DSignatureBlob, &pID3DErrorBlob));
	RETURN_IF_FAILED(pID3D12Device4->CreateRootSignature(0,
		pID3DSignatureBlob->GetBufferPointer(),
		pID3DSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&pID3D12RootSignature)
	));
	return S_OK;
}

HRESULT Compile_D3D_Shaders() {
	//Compile
	{
		TSTRING strFilePath = /*strCatallogPath +*/ strShaderFileName;
		RETURN_IF_FAILED(D3DCompileFromFile(strFilePath.c_str()
			, nullptr
			, nullptr
			, "VSMain"
			, "vs_5_0"
			, nD3DCompileFlags
			, 0
			, &pID3DVertexShaderBlob
			, nullptr
		));
		RETURN_IF_FAILED(D3DCompileFromFile(strFilePath.c_str()
			, nullptr
			, nullptr
			, "PSMain"
			, "ps_5_0"
			, nD3DCompileFlags
			, 0
			, &pID3DPixelShaderBlob
			, nullptr
		));
	}
	//Shander Desc
	{
		stD3D12VertexShader.pShaderBytecode = pID3DVertexShaderBlob->GetBufferPointer();
		stD3D12VertexShader.BytecodeLength = pID3DVertexShaderBlob->GetBufferSize();
		stD3D12PixelShader.pShaderBytecode = pID3DPixelShaderBlob->GetBufferPointer();
		stD3D12PixelShader.BytecodeLength = pID3DPixelShaderBlob->GetBufferSize();
	}
	return S_OK;
}

HRESULT Set_D3D12_Input_Layout() {
	D3D12_INPUT_ELEMENT_DESC stD3D12InputElementDescs[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
	stD3D12InputLayoutDesc = { stD3D12InputElementDescs,_countof(stD3D12InputElementDescs) };
	return S_OK;
}

HRESULT Create_D3D12_Pipeline_State() {
	//Render Target Blender Desc
	{
		stD3D12RenderTargetBlendDesc.BlendEnable = FALSE;
		stD3D12RenderTargetBlendDesc.LogicOpEnable = FALSE;
		stD3D12RenderTargetBlendDesc.SrcBlend = D3D12_BLEND_ONE;
		stD3D12RenderTargetBlendDesc.DestBlend = D3D12_BLEND_ZERO;
		stD3D12RenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		stD3D12RenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		stD3D12RenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		stD3D12RenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		stD3D12RenderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		stD3D12RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	//Blend Desc
	{
		stD3D12BlendDesc.AlphaToCoverageEnable = FALSE;
		stD3D12BlendDesc.IndependentBlendEnable = FALSE;
		for (auto&& t : stD3D12BlendDesc.RenderTarget)
			t = stD3D12RenderTargetBlendDesc;

	}
	//Rasterizer Desc
	{
		stD3D12RasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		stD3D12RasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		stD3D12RasterizerDesc.FrontCounterClockwise = FALSE;//Left-handed Coordinates
		stD3D12RasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		stD3D12RasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		stD3D12RasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		stD3D12RasterizerDesc.DepthClipEnable = TRUE;
		stD3D12RasterizerDesc.MultisampleEnable = FALSE;
		stD3D12RasterizerDesc.AntialiasedLineEnable = FALSE;
		stD3D12RasterizerDesc.ForcedSampleCount = 0;
		stD3D12RasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	//Depth Stencil Desc
	{
		stD3D12DepthStencilDesc.DepthEnable = TRUE;
		stD3D12DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		stD3D12DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		stD3D12DepthStencilDesc.StencilEnable = FALSE;
		stD3D12DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		stD3D12DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		stD3D12DepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		stD3D12DepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12DepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
	//Graphics Pipelie State Desc
	{
		stD3D12GraphicsPipelineStateDesc.pRootSignature = pID3D12RootSignature.Get();
		stD3D12GraphicsPipelineStateDesc.VS = stD3D12VertexShader;
		stD3D12GraphicsPipelineStateDesc.PS = stD3D12PixelShader;
		//DS
		//HS
		//GS
		//Default StreamOutput
		stD3D12GraphicsPipelineStateDesc.BlendState = stD3D12BlendDesc;
		stD3D12GraphicsPipelineStateDesc.SampleMask = UINT_MAX;
		stD3D12GraphicsPipelineStateDesc.RasterizerState = stD3D12RasterizerDesc;
		stD3D12GraphicsPipelineStateDesc.DepthStencilState = stD3D12DepthStencilDesc;
		stD3D12GraphicsPipelineStateDesc.InputLayout = stD3D12InputLayoutDesc;
		stD3D12GraphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		stD3D12GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		stD3D12GraphicsPipelineStateDesc.NumRenderTargets = 1;
		stD3D12GraphicsPipelineStateDesc.RTVFormats[0] = enDXGISwapChainFormat;
		stD3D12GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		stD3D12GraphicsPipelineStateDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12GraphicsPipelineStateDesc.NodeMask = 0;
		stD3D12GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stD3D12GraphicsPipelineStateDesc, IID_PPV_ARGS(&pID3D12PipelineState)));
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

HRESULT Load_D3D12_Vertex_Buffer() {
	//Vertex BufferHeap Properties
	{
		stD3D12VertexBufferHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		stD3D12VertexBufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12VertexBufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12VertexBufferHeapProperties.CreationNodeMask = 0;
		stD3D12VertexBufferHeapProperties.VisibleNodeMask = 0;
	}
	//Vertex Buffer Resource Desc
	{
		stD3D12VertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stD3D12VertexBufferResourceDesc.Alignment = 0;
		stD3D12VertexBufferResourceDesc.Width = nVertexBufferByteSize;
		stD3D12VertexBufferResourceDesc.Height = 1;
		stD3D12VertexBufferResourceDesc.DepthOrArraySize = 1;
		stD3D12VertexBufferResourceDesc.MipLevels = 1;
		stD3D12VertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		stD3D12VertexBufferResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12VertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stD3D12VertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}
	//Create Buffer
	{
		RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12VertexBufferHeapProperties
			, D3D12_HEAP_FLAG_NONE
			, &stD3D12VertexBufferResourceDesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&pID3D12VertexBufferResource)
		));
	}
	//Copy to Buffer
	{
		void* pVertexData = nullptr;
		D3D12_RANGE stReadRange = { 0,0 };
		THROW_IF_FAILED(pID3D12VertexBufferResource->Map(0, &stReadRange, &pVertexData));
		memcpy(pVertexData, TriangleVertices, sizeof(TriangleVertices));
		pID3D12VertexBufferResource->Unmap(0, nullptr);
	}
	//Vertex Buffer View
	{
		stD3D12VertexBufferView.BufferLocation = pID3D12VertexBufferResource->GetGPUVirtualAddress();
		stD3D12VertexBufferView.StrideInBytes = sizeof(VERTEX);
		stD3D12VertexBufferView.SizeInBytes = nVertexBufferByteSize;
	}
	return S_OK;
}

HRESULT Load_D3D12_Index_Buffer() {
	//Index BufferHeap Properties
	{
		stD3D12IndexBufferHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		stD3D12IndexBufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12IndexBufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12IndexBufferHeapProperties.CreationNodeMask = 0;
		stD3D12IndexBufferHeapProperties.VisibleNodeMask = 0;
	}
	//Index Buffer Resource Desc
	{
		stD3D12IndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stD3D12IndexBufferResourceDesc.Alignment = 0;
		stD3D12IndexBufferResourceDesc.Width = nIndexBufferByteSize;
		stD3D12IndexBufferResourceDesc.Height = 1;
		stD3D12IndexBufferResourceDesc.DepthOrArraySize = 1;
		stD3D12IndexBufferResourceDesc.MipLevels = 1;
		stD3D12IndexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		stD3D12IndexBufferResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12IndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stD3D12IndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}
	//Create Buffer
	{
		RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12IndexBufferHeapProperties
			, D3D12_HEAP_FLAG_NONE
			, &stD3D12IndexBufferResourceDesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&pID3D12IndexBufferResource)
		));
	}
	//Copy to Buffer
	{
		void* pIndexData = nullptr;
		D3D12_RANGE stReadRange = { 0,0 };
		THROW_IF_FAILED(pID3D12IndexBufferResource->Map(0, &stReadRange, &pIndexData));
		memcpy(pIndexData, TriangleIndexs, sizeof(TriangleIndexs));
		pID3D12IndexBufferResource->Unmap(0, nullptr);
	}
	//Index Buffer View
	{
		stD3D12IndexBufferView.BufferLocation = pID3D12IndexBufferResource->GetGPUVirtualAddress();
		stD3D12IndexBufferView.SizeInBytes = nIndexBufferByteSize;
		stD3D12IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}
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

HRESULT Load_Pipeline() {
	RETURN_IF_FAILED(Set_D3D12_Debug());
	RETURN_IF_FAILED(Create_DXGI_Factory());
	RETURN_IF_FAILED(Create_DXGI_Adapter());
	RETURN_IF_FAILED(Create_D3D12_Device());
	RETURN_IF_FAILED(Create_D3D12_Command_Queue());
	RETURN_IF_FAILED(Create_DXGI_SwapChain());
	RETURN_IF_FAILED(Create_D3D12_RTV_Heap());
	RETURN_IF_FAILED(Create_D3D12_DSV_Heap());
	RETURN_IF_FAILED(Create_D3D12_CBV_Heap());
	RETURN_IF_FAILED(Create_D3D12_RTVs());
	RETURN_IF_FAILED(Create_D3D12_DSV());
	RETURN_IF_FAILED(Create_D3D12_CBV());
	RETURN_IF_FAILED(Create_D3D12_Command_Allocator());
	return S_OK;
}

HRESULT Load_Asset() {
	RETURN_IF_FAILED(Create_D3D12_Root_Signature());
	RETURN_IF_FAILED(Compile_D3D_Shaders());
	RETURN_IF_FAILED(Set_D3D12_Input_Layout());
	RETURN_IF_FAILED(Create_D3D12_Pipeline_State());
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List());
	RETURN_IF_FAILED(Load_D3D12_Vertex_Buffer());
	RETURN_IF_FAILED(Load_D3D12_Index_Buffer());
	RETURN_IF_FAILED(Create_D3D12_Fence());
	RETURN_IF_FAILED(Create_D3D12_Fence_Event());
	RETURN_IF_FAILED(Create_D3D12_Resource_Barrier());
	return S_OK;
}

HRESULT Populate_CommandList() {
	RETURN_IF_FAILED(pID3D12CommandAllocator->Reset());
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Reset(pID3D12CommandAllocator.Get(), pID3D12PipelineState.Get()));

	//set State
	pID3D12GraphicsCommandList->SetGraphicsRootSignature(pID3D12RootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { pID3D12CBVHeap.Get() };
	pID3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	pID3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(0, pID3D12CBVHeap->GetGPUDescriptorHandleForHeapStart());

	pID3D12GraphicsCommandList->SetPipelineState(pID3D12PipelineState.Get());

	pID3D12GraphicsCommandList->RSSetViewports(1, &stViewPort);
	pID3D12GraphicsCommandList->RSSetScissorRects(1, &stScissorRect);

	UINT nFrameIndex = pIDXGISwapChain3->GetCurrentBackBufferIndex();

	//Begin
	stD3D12BeginResourceBarrier.Transition.pResource = pID3D12RenderTargetsResource[nFrameIndex].Get();
	pID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12BeginResourceBarrier);

	//Render Target
	stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
	stD3D12CPURTVHandle.ptr += nFrameIndex * nRTVHandleIncrementSize;
	stD3D12CPUDSVHandle = pID3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
	//set
	pID3D12GraphicsCommandList->OMSetRenderTargets(1, &stD3D12CPURTVHandle, FALSE, &stD3D12CPUDSVHandle);

	//Cear
	const float ClearColor[] = { color[0], color[1], color[2], color[3] };
	pID3D12GraphicsCommandList->ClearRenderTargetView(stD3D12CPURTVHandle, ClearColor, 0, nullptr);
	pID3D12GraphicsCommandList->ClearDepthStencilView(stD3D12CPUDSVHandle
		, D3D12_CLEAR_FLAG_DEPTH
		, stD3D12DepthStencilClearValue.DepthStencil.Depth
		, stD3D12DepthStencilClearValue.DepthStencil.Stencil
		, 0
		, nullptr
	);

	//Render Resource
	pID3D12GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pID3D12GraphicsCommandList->IASetVertexBuffers(0, 1, &stD3D12VertexBufferView);
	pID3D12GraphicsCommandList->IASetIndexBuffer(&stD3D12IndexBufferView);

	//Render Command
	pID3D12GraphicsCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

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
		OnUpdate();
		OnRender();
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