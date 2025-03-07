#define WIN32_LEAN_AND_MEAN
#include<windows.h>

#include<wrl.h>

#include<atlacc.h>
#include <atlconv.h>
#include <atlcoll.h> 

#include<process.h>

#include<tchar.h>

#include<string>
#include<fstream>
#include<utility>
#include<vector>
#include<memory>

#include<dxgi1_6.h>
#ifdef _DEBUG
#include<dxgidebug.h>
#endif // _DEBUG

#include<d3d12.h>
#include<d3dcompiler.h>
#include "../d3dx12.h"

#include<DirectXMath.h>

#include<wincodec.h>

#include "../DDSTextureLoader12.h"

//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

using std::wstring;
using std::pair;
using std::vector;
using Microsoft::WRL::ComPtr;
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

#define GRS_FLOOR_DIV(A,B)					((UINT)(((A)+((B)-1))/(B)))
#define GRS_ALIGNMENT_UPPER(A,B)			((UINT)(((A)+((B)-1))&~(B - 1)))

//Help
UINT64 Alignment_Upper(_In_ UINT64 n64Size, _In_ UINT64 nAligment) {
	return (n64Size + nAligment - 1) & ~(nAligment - 1);
}

UINT64 Floor_DIV(_In_ UINT64 n64Size, _In_ UINT64 div) {
	return (n64Size + div - 1) / div;
}

_Check_return_ HRESULT Get_CatallogPath(_Out_ wstring* wstrCatallogPath) {

	TCHAR path[MAX_PATH] = {};
	if (0 == GetModuleFileName(nullptr, path, MAX_PATH))
		return HRESULT_FROM_WIN32(GetLastError());

	*wstrCatallogPath = path;

	//Get Catallog
	{
		//Erase "fimename.exe"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L'\\'));
		//Erase "x64"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L"\\"));
		//Erase "Debug/Release"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L"\\") + 1);
	}

	return S_OK;
}

//Window
_Check_return_ HRESULT Register_WndClassEx(_In_ const HINSTANCE hInstance, _In_ const wstring* wstrClassName, _In_ WNDPROC lpfnWndProc) {

	WNDCLASSEX stWndClassEx = {};
	{
		stWndClassEx.cbSize = sizeof(WNDCLASSEX);
		stWndClassEx.style = CS_GLOBALCLASS;
		stWndClassEx.lpfnWndProc = lpfnWndProc;
		stWndClassEx.cbClsExtra = 0;
		stWndClassEx.cbWndExtra = 0;
		stWndClassEx.hInstance = hInstance;
		stWndClassEx.hIcon = nullptr;
		stWndClassEx.hCursor = static_cast<HCURSOR> (LoadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
		stWndClassEx.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
		stWndClassEx.lpszMenuName = nullptr;
		stWndClassEx.lpszClassName = wstrClassName->c_str();
		stWndClassEx.hIconSm = nullptr;
	}

	if (0 == RegisterClassEx(&stWndClassEx))
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

_Check_return_ HRESULT Initialize_WindowEx(_In_opt_ const HINSTANCE hInstance, _In_ const wstring* wstrClassName, _In_ const wstring* wstrWindowExName, _In_ int nClientWidth, _In_ int nClientHeight, _Outptr_ HWND* hWnd) {

	RECT rectWnd = { 0,0,nClientWidth,nClientHeight };

	if (0 == AdjustWindowRectEx(&rectWnd, WS_OVERLAPPEDWINDOW, TRUE, WS_EX_LEFT))
		return  HRESULT_FROM_WIN32(GetLastError());

	INT iScreenLeftTopX = (GetSystemMetrics(SM_CXSCREEN) - (rectWnd.right - rectWnd.left)) / 2;
	INT iScreenLeftTopY = (GetSystemMetrics(SM_CYSCREEN) - (rectWnd.bottom - rectWnd.top)) / 2;

	//Create Window
	{
		*hWnd = CreateWindowEx(WS_EX_LEFT
			, wstrClassName->c_str()
			, wstrWindowExName->c_str()
			, WS_OVERLAPPEDWINDOW
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
	if (0 == *hWnd)
		return  HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

// D3D12 Fill
D3D12_DESCRIPTOR_HEAP_DESC Fill_D3D12_Descriptor_Heap_Desc(_In_ D3D12_DESCRIPTOR_HEAP_TYPE enD3D12DescriptorHeapType, _In_ UINT nDescriptorCount) {

	D3D12_DESCRIPTOR_HEAP_DESC stD3D12DescriptorHeapDesc = {};
	{
		stD3D12DescriptorHeapDesc.Type = enD3D12DescriptorHeapType;
		stD3D12DescriptorHeapDesc.NumDescriptors = nDescriptorCount;
		stD3D12DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		stD3D12DescriptorHeapDesc.NodeMask = 0;
	}

	return stD3D12DescriptorHeapDesc;
}

D3D12_DESCRIPTOR_RANGE1 Fill_D3D12_Descriptor_Range1(_In_ D3D12_DESCRIPTOR_RANGE_TYPE enD3D12DescriptorRangeType, _In_ UINT DescriptorNum, _In_ UINT BaseShaderRegister, _In_ D3D12_DESCRIPTOR_RANGE_FLAGS enD3D12DescriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE) {

	D3D12_DESCRIPTOR_RANGE1 stD3D12DescriptorRange1 = {};
	{
		stD3D12DescriptorRange1.RangeType = enD3D12DescriptorRangeType;
		stD3D12DescriptorRange1.NumDescriptors = DescriptorNum;
		stD3D12DescriptorRange1.BaseShaderRegister = BaseShaderRegister;
		stD3D12DescriptorRange1.RegisterSpace = 0;
		stD3D12DescriptorRange1.Flags = enD3D12DescriptorRangeFlags;
		stD3D12DescriptorRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	return stD3D12DescriptorRange1;
}

D3D12_ROOT_PARAMETER1 Fill_D3D12_Root_Parameter1_Descriptor_Table(_In_ UINT nDescriptorRangeNum, _In_ const D3D12_DESCRIPTOR_RANGE1* pD3D12DescriptorRange1s, _In_ D3D12_SHADER_VISIBILITY enD3D12ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL) {

	D3D12_ROOT_PARAMETER1 stD3D12RootParameter1 = {};
	{
		stD3D12RootParameter1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stD3D12RootParameter1.DescriptorTable.NumDescriptorRanges = nDescriptorRangeNum;
		stD3D12RootParameter1.DescriptorTable.pDescriptorRanges = pD3D12DescriptorRange1s;
		stD3D12RootParameter1.ShaderVisibility = enD3D12ShaderVisibility;
	}

	return stD3D12RootParameter1;
}

D3D12_STATIC_SAMPLER_DESC Fill_D3D12_Static_Sampler_Desc(_In_ UINT nShaderRegister) {

	D3D12_STATIC_SAMPLER_DESC	stD3D12StaticSamplerDesc = {};
	{
		stD3D12StaticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		stD3D12StaticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12StaticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12StaticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12StaticSamplerDesc.MipLODBias = 0;
		stD3D12StaticSamplerDesc.MaxAnisotropy = 0;
		stD3D12StaticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		stD3D12StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		stD3D12StaticSamplerDesc.MinLOD = 0.0f;
		stD3D12StaticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		stD3D12StaticSamplerDesc.ShaderRegister = nShaderRegister;
		stD3D12StaticSamplerDesc.RegisterSpace = 0;
		stD3D12StaticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	return stD3D12StaticSamplerDesc;
}

D3D12_ROOT_SIGNATURE_DESC1 Fill_D3D12_Root_Signature_Desc1(_In_opt_ UINT nParameterNum, _In_opt_ const D3D12_ROOT_PARAMETER1* pstD3D12Parameters, _In_opt_ UINT nStaticSamplerNum, _In_opt_ const D3D12_STATIC_SAMPLER_DESC* pstD3D12StaticSamplers) {

	D3D12_ROOT_SIGNATURE_DESC1	stRootSignatureDesc1 = {};
	{
		stRootSignatureDesc1.NumParameters = nParameterNum;
		stRootSignatureDesc1.pParameters = pstD3D12Parameters;
		stRootSignatureDesc1.NumStaticSamplers = nStaticSamplerNum;
		stRootSignatureDesc1.pStaticSamplers = pstD3D12StaticSamplers;
		stRootSignatureDesc1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	return stRootSignatureDesc1;
}

D3D12_INPUT_LAYOUT_DESC Fill_D3D12_Input_Layout() {

	D3D12_INPUT_ELEMENT_DESC stD3D12InputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	return D3D12_INPUT_LAYOUT_DESC{ stD3D12InputElementDescs,_countof(stD3D12InputElementDescs) };
}

D3D12_BLEND_DESC Fill_D3D12_Blend_Desc() {

	D3D12_RENDER_TARGET_BLEND_DESC	stD3D12RenderTargetBlendDesc = {};
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

	D3D12_BLEND_DESC stD3D12BlendDesc = {};
	{
		stD3D12BlendDesc.AlphaToCoverageEnable = FALSE;
		stD3D12BlendDesc.IndependentBlendEnable = FALSE;

		for (auto&& t : stD3D12BlendDesc.RenderTarget)
			t = stD3D12RenderTargetBlendDesc;
	}

	return stD3D12BlendDesc;
}

D3D12_RASTERIZER_DESC Fill_D3D12_Rasterizer_Desc() {

	D3D12_RASTERIZER_DESC stD3D12RasterizerDesc = {};
	{
		stD3D12RasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		stD3D12RasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

		stD3D12RasterizerDesc.FrontCounterClockwise = FALSE;//Left-handed Coordinates
		stD3D12RasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		stD3D12RasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		stD3D12RasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		stD3D12RasterizerDesc.DepthClipEnable = FALSE;
		stD3D12RasterizerDesc.MultisampleEnable = FALSE;
		stD3D12RasterizerDesc.AntialiasedLineEnable = FALSE;
		stD3D12RasterizerDesc.ForcedSampleCount = 0;
		stD3D12RasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	
	return stD3D12RasterizerDesc;
}

D3D12_DEPTH_STENCIL_DESC Fill_D3D12_DepthStencil_Desc() {

	D3D12_DEPTH_STENCIL_DESC stD3D12DepthStencilDesc = {};
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

	return stD3D12DepthStencilDesc;
}

D3D12_HEAP_PROPERTIES Fill_D3D12_Heap_Properties(_In_ D3D12_HEAP_TYPE enD3D12HeapType) {

	D3D12_HEAP_PROPERTIES stD3D12HeapProperties = {};
	{
		stD3D12HeapProperties.Type = enD3D12HeapType;
		stD3D12HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12HeapProperties.CreationNodeMask = 0;
		stD3D12HeapProperties.VisibleNodeMask = 0;
	}

	return stD3D12HeapProperties;
}

D3D12_RESOURCE_DESC Fill_D3D12_Resource_Desc_Tex2D(_In_ DXGI_FORMAT enDXGIFormat, _In_ UINT64 nWidth, _In_ UINT nHeight, _In_ D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE, _In_opt_ UINT64 Alignment = 0) {

	D3D12_RESOURCE_DESC	stD3D12ResourceDesc = {};
	{
		stD3D12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stD3D12ResourceDesc.Alignment = Alignment;//fill init to system Optimize
		stD3D12ResourceDesc.Width = nWidth;
		stD3D12ResourceDesc.Height = nHeight;
		stD3D12ResourceDesc.DepthOrArraySize = 1;
		stD3D12ResourceDesc.MipLevels = 1;
		stD3D12ResourceDesc.Format = enDXGIFormat;
		stD3D12ResourceDesc.SampleDesc.Count = 1;
		stD3D12ResourceDesc.SampleDesc.Quality = 0;
		stD3D12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		stD3D12ResourceDesc.Flags = Flags;
	}

	return stD3D12ResourceDesc;
}

D3D12_RESOURCE_DESC Fill_D3D12_Resource_Desc_Buffer(_In_ UINT64 nTotailByteSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) {

	D3D12_RESOURCE_DESC	stD3D12ResourceDesc = {};
	{
		stD3D12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stD3D12ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stD3D12ResourceDesc.Width = nTotailByteSize;
		stD3D12ResourceDesc.Height = 1;
		stD3D12ResourceDesc.DepthOrArraySize = 1;
		stD3D12ResourceDesc.MipLevels = 1;
		stD3D12ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		stD3D12ResourceDesc.SampleDesc.Count = 1;
		stD3D12ResourceDesc.SampleDesc.Quality = 0;
		stD3D12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stD3D12ResourceDesc.Flags = Flags;
	}

	return stD3D12ResourceDesc;
}

D3D12_SAMPLER_DESC Fill_D3D12_Sampler_Desc() {

	D3D12_SAMPLER_DESC stD3D12SamplerDescs = {};
	{
		stD3D12SamplerDescs.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		stD3D12SamplerDescs.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stD3D12SamplerDescs.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stD3D12SamplerDescs.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

		stD3D12SamplerDescs.MipLODBias = 0.0f;
		stD3D12SamplerDescs.MaxAnisotropy = 1;
		stD3D12SamplerDescs.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		stD3D12SamplerDescs.BorderColor[0] = 0.0f;
		stD3D12SamplerDescs.BorderColor[1] = 0.0f;
		stD3D12SamplerDescs.BorderColor[2] = 0.0f;
		stD3D12SamplerDescs.BorderColor[3] = 0.0f;

		stD3D12SamplerDescs.MinLOD = 0.0f;
		stD3D12SamplerDescs.MaxLOD = D3D12_FLOAT32_MAX;
	}

	return stD3D12SamplerDescs;
}

D3D12_VERTEX_BUFFER_VIEW Fill_D3D12_VertexBufferView(_In_ UINT nSignlByteSize, _In_ ID3D12Resource* pID3D12VertexBufferResource) {

	D3D12_VERTEX_BUFFER_VIEW stD3D12VertexBufferView = {};
	{
		stD3D12VertexBufferView.BufferLocation = pID3D12VertexBufferResource->GetGPUVirtualAddress();
		stD3D12VertexBufferView.StrideInBytes = nSignlByteSize;
		stD3D12VertexBufferView.SizeInBytes = static_cast<UINT>(pID3D12VertexBufferResource->GetDesc().Width);
	}

	return stD3D12VertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Fill_D3D12_IndexBufferView(_In_ UINT nTotalByteSize, _In_ ID3D12Resource* pID3D12IndexBufferResource) {

	D3D12_INDEX_BUFFER_VIEW	stD3D12IndexBufferView = {};
	{
		stD3D12IndexBufferView.BufferLocation = pID3D12IndexBufferResource->GetGPUVirtualAddress();
		stD3D12IndexBufferView.SizeInBytes = nTotalByteSize;
		stD3D12IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}

	return stD3D12IndexBufferView;
}

D3D12_RESOURCE_BARRIER Fill_D3D12_Barrier_Transition(_In_ ID3D12Resource* pID3D12Resource, _In_ D3D12_RESOURCE_STATES enD3D12ResourceStateBefore, _In_ D3D12_RESOURCE_STATES enD3D12ResourceStateAfter) {

	D3D12_RESOURCE_BARRIER stResBar = {};
	{
		stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stResBar.Transition.pResource = pID3D12Resource;
		stResBar.Transition.StateBefore = enD3D12ResourceStateBefore;
		stResBar.Transition.StateAfter = enD3D12ResourceStateAfter;
		stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}

	return stResBar;
}

// D3D12 Init
_Check_return_ HRESULT Set_D3D12_Debug(_Inout_ UINT* nDXGIFactoryFlags) {

#ifdef _DEBUG
	ID3D12Debug* pID3D12Debug = nullptr;
	RETURN_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12Debug)));
	pID3D12Debug->EnableDebugLayer();
	*nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	return S_OK;
#endif // _DEBUG

	return E_FAIL;
}

_Check_return_ HRESULT Create_DXGI_Factory(_In_opt_ UINT nDXGIFactoryFlags, _COM_Outptr_ IDXGIFactory6** ppIDXGIFactory6) {
	RETURN_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(ppIDXGIFactory6)));
	return S_OK;
}

_Check_return_ HRESULT Create_DXGI_Adapter(_In_ IDXGIFactory6* pIDXGIFactory6, _COM_Outptr_ IDXGIAdapter1** ppIDXGIAdapter1) {
	RETURN_IF_FAILED(pIDXGIFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppIDXGIAdapter1)));
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Device(_In_ IDXGIAdapter1* pIDXGIAdapter1, _In_ D3D_FEATURE_LEVEL enD3DFeatureLevel, _COM_Outptr_ ID3D12Device4** ppID3D12Device4) {
	RETURN_IF_FAILED(D3D12CreateDevice(pIDXGIAdapter1, enD3DFeatureLevel, IID_PPV_ARGS(ppID3D12Device4)));
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Direct_Command_Queue(_In_ ID3D12Device4* pID3D12Device4, _COM_Outptr_ ID3D12CommandQueue** ppID3D12CommandQueue) {

	D3D12_COMMAND_QUEUE_DESC stD3D12CommandQueueDesc = {};
	{
		stD3D12CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		stD3D12CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		stD3D12CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		stD3D12CommandQueueDesc.NodeMask = 0;
	}

	RETURN_IF_FAILED(pID3D12Device4->CreateCommandQueue(&stD3D12CommandQueueDesc, IID_PPV_ARGS(ppID3D12CommandQueue)));

	return S_OK;
}

_Check_return_ HRESULT Create_DXGI_SwapChain(_In_ IDXGIFactory6* pIDXGIFactory6,_In_ DXGI_FORMAT enDXGIFormat, _In_ UINT nWidth, _In_ UINT nHeight, _In_ UINT nBufferCount, _In_ ID3D12CommandQueue* pID3D12CommandQueue, _In_ HWND hWnd, _Outref_ ComPtr<IDXGISwapChain3>& refcomIDXGISwapChain3) {

	DXGI_SWAP_CHAIN_DESC1 stDXGISwapChainDesc1 = {};
	{
		stDXGISwapChainDesc1.Width = nWidth;
		stDXGISwapChainDesc1.Height = nHeight;
		stDXGISwapChainDesc1.Format = enDXGIFormat;
		stDXGISwapChainDesc1.Stereo = FALSE;
		stDXGISwapChainDesc1.SampleDesc.Count = 1;
		stDXGISwapChainDesc1.SampleDesc.Quality = 0;
		stDXGISwapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		stDXGISwapChainDesc1.BufferCount = nBufferCount;
		stDXGISwapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		stDXGISwapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		stDXGISwapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		stDXGISwapChainDesc1.Flags = 0;//No Flag
	}

	//Create
	{
		ComPtr<IDXGISwapChain1> pIDXGISwapChain1;
		RETURN_IF_FAILED(pIDXGIFactory6->CreateSwapChainForHwnd(pID3D12CommandQueue
			, hWnd
			, &stDXGISwapChainDesc1
			, nullptr
			, nullptr
			, &pIDXGISwapChain1
		));
		pIDXGISwapChain1.As(&refcomIDXGISwapChain3);
	}

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Descriptor_Heap(_In_ ID3D12Device4* pID3D12Device4, _In_ D3D12_DESCRIPTOR_HEAP_TYPE enD3D12DescriptorHeapType, _In_ UINT nDescriptorNum, _COM_Outptr_ ID3D12DescriptorHeap** ppID3D12DescriptorHeap) {

	D3D12_DESCRIPTOR_HEAP_DESC stD3D12DescriptorHeapDesc = Fill_D3D12_Descriptor_Heap_Desc(enD3D12DescriptorHeapType, nDescriptorNum);
	if (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == enD3D12DescriptorHeapType || D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == enD3D12DescriptorHeapType)
		stD3D12DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12DescriptorHeapDesc, IID_PPV_ARGS(ppID3D12DescriptorHeap)));

	return S_OK;
}

//Command List
_Check_return_ HRESULT Create_D3D12_Command_Allocator(_In_ ID3D12Device4* pID3D12Device4, _In_ D3D12_COMMAND_LIST_TYPE enD3D12CommandListType, _COM_Outptr_ ID3D12CommandAllocator** ppID3D12CommandAllocator) {
	RETURN_IF_FAILED(pID3D12Device4->CreateCommandAllocator(enD3D12CommandListType, IID_PPV_ARGS(ppID3D12CommandAllocator)));
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Graphics_Command_List(_In_ ID3D12CommandAllocator* pID3D12CommandAllocator, _In_ D3D12_COMMAND_LIST_TYPE enD3D12CommandListType, _COM_Outptr_ ID3D12GraphicsCommandList** ppID3D12GraphicsCommandList){

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12CommandAllocator->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	RETURN_IF_FAILED(pID3D12Device4->CreateCommandList(0
		, enD3D12CommandListType
		, pID3D12CommandAllocator
		, nullptr
		, IID_PPV_ARGS(ppID3D12GraphicsCommandList)
	));

	pID3D12Device4->Release();

	return S_OK;
}

//D3D12 Assert
_Check_return_ HRESULT Create_D3D12_Root_Signature(_In_ ID3D12Device4* pID3D12Device4, _COM_Outptr_ ID3D12RootSignature** ppID3D12RootSignature) {

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC	stD3D12VersionedRootSignatureDesc = {};
	stD3D12VersionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	//Root Signature Desc1
	{
		D3D12_DESCRIPTOR_RANGE1 stD3D12DescriptorRange1s[3] = {};
		stD3D12DescriptorRange1s[0] = Fill_D3D12_Descriptor_Range1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		stD3D12DescriptorRange1s[1] = Fill_D3D12_Descriptor_Range1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		stD3D12DescriptorRange1s[2] = Fill_D3D12_Descriptor_Range1(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		D3D12_ROOT_PARAMETER1 stD3D12RootParameter1s[3] = {};
		stD3D12RootParameter1s[0] = Fill_D3D12_Root_Parameter1_Descriptor_Table(1, &stD3D12DescriptorRange1s[0], D3D12_SHADER_VISIBILITY_ALL);
		stD3D12RootParameter1s[1] = Fill_D3D12_Root_Parameter1_Descriptor_Table(1, &stD3D12DescriptorRange1s[1], D3D12_SHADER_VISIBILITY_PIXEL);
		stD3D12RootParameter1s[2] = Fill_D3D12_Root_Parameter1_Descriptor_Table(1, &stD3D12DescriptorRange1s[2], D3D12_SHADER_VISIBILITY_PIXEL);

		stD3D12VersionedRootSignatureDesc.Desc_1_1 = Fill_D3D12_Root_Signature_Desc1(_countof(stD3D12RootParameter1s), &stD3D12RootParameter1s[0], 0, nullptr);
	}

	ID3DBlob* pID3DSignatureBlob = nullptr;
	RETURN_IF_FAILED(D3D12SerializeVersionedRootSignature(&stD3D12VersionedRootSignatureDesc, &pID3DSignatureBlob, nullptr));

	RETURN_IF_FAILED(pID3D12Device4->CreateRootSignature(0,
		pID3DSignatureBlob->GetBufferPointer(),
		pID3DSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(ppID3D12RootSignature)
	));

	return S_OK;
}

_Check_return_ HRESULT Compile_D3D_VertexShader(_In_ const wstring* wstrShaderFileName, _COM_Outptr_ ID3DBlob** ppID3DVertexShaderBlob, _In_opt_ UINT nD3DCompileFlags = 0) {

#ifdef _DEBUG
	nD3DCompileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#endif // _DEBUG

	RETURN_IF_FAILED(D3DCompileFromFile(wstrShaderFileName->c_str()
		, nullptr
		, nullptr
		, "VSMain"
		, "vs_5_0"
		, nD3DCompileFlags
		, 0
		, ppID3DVertexShaderBlob
		, nullptr
	));

	return S_OK;
}

_Check_return_ HRESULT Compile_D3D_PixelShader(_In_ const wstring* wstrShaderFileName, _COM_Outptr_ ID3DBlob** ppID3DPixelShaderBlob, _In_opt_ UINT nD3DCompileFlags = 0) {

#ifdef _DEBUG
	nD3DCompileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#endif // _DEBUG

	RETURN_IF_FAILED(D3DCompileFromFile(wstrShaderFileName->c_str()
		, nullptr
		, nullptr
		, "PSMain"
		, "ps_5_0"
		, nD3DCompileFlags
		, 0
		, ppID3DPixelShaderBlob
		, nullptr
	));

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Pipeline_State(_In_ ID3D12RootSignature* pID3D12RootSignature, _In_ const wstring* wstrShaderFileName, _In_ D3D12_BLEND_DESC* pstD3D12BlendDesc, _In_ D3D12_RASTERIZER_DESC* pstD3D12RasterizerDesc, _In_ D3D12_DEPTH_STENCIL_DESC* pstD3D12DepthStencilDesc, _In_ D3D12_INPUT_LAYOUT_DESC* pstD3D12InputLayoutDesc, _In_ DXGI_FORMAT enDXGIRenderTargetFormat, _In_ DXGI_FORMAT enDXGIDEpthStencilFormat, _COM_Outptr_ ID3D12PipelineState** ppID3D12PipelineState) {

	ID3DBlob* pID3DVertexBlob = nullptr;
	ID3DBlob* pID3DPixelBlob = nullptr;

	RETURN_IF_FAILED(Compile_D3D_VertexShader(wstrShaderFileName, &pID3DVertexBlob));
	RETURN_IF_FAILED(Compile_D3D_PixelShader(wstrShaderFileName, &pID3DPixelBlob));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stD3D12GraphicsPipelineStateDesc = {};
	{
		stD3D12GraphicsPipelineStateDesc.pRootSignature = pID3D12RootSignature;

		stD3D12GraphicsPipelineStateDesc.VS.pShaderBytecode = pID3DVertexBlob->GetBufferPointer();
		stD3D12GraphicsPipelineStateDesc.VS.BytecodeLength = pID3DVertexBlob->GetBufferSize();

		stD3D12GraphicsPipelineStateDesc.PS.pShaderBytecode = pID3DPixelBlob->GetBufferPointer();
		stD3D12GraphicsPipelineStateDesc.PS.BytecodeLength = pID3DPixelBlob->GetBufferSize();

		//DS
		//HS
		//GS
		
		//Default StreamOutput

		stD3D12GraphicsPipelineStateDesc.BlendState = *pstD3D12BlendDesc;
		stD3D12GraphicsPipelineStateDesc.SampleMask = UINT_MAX;
		stD3D12GraphicsPipelineStateDesc.RasterizerState = *pstD3D12RasterizerDesc;
		stD3D12GraphicsPipelineStateDesc.DepthStencilState = *pstD3D12DepthStencilDesc;
		stD3D12GraphicsPipelineStateDesc.InputLayout = *pstD3D12InputLayoutDesc;
		stD3D12GraphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		stD3D12GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		stD3D12GraphicsPipelineStateDesc.NumRenderTargets = 1;
		stD3D12GraphicsPipelineStateDesc.RTVFormats[0] = enDXGIRenderTargetFormat;
		stD3D12GraphicsPipelineStateDesc.DSVFormat = enDXGIDEpthStencilFormat;
		stD3D12GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		stD3D12GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		stD3D12GraphicsPipelineStateDesc.NodeMask = 0;
		stD3D12GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12RootSignature->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	RETURN_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stD3D12GraphicsPipelineStateDesc, IID_PPV_ARGS(ppID3D12PipelineState)));

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Fence(_In_ ID3D12Device4* pID3D12Device4, _In_ UINT64 nD3D12FenceCPUValue, _COM_Outptr_ ID3D12Fence** ppID3D12Fence) {
	RETURN_IF_FAILED(pID3D12Device4->CreateFence(nD3D12FenceCPUValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(ppID3D12Fence)));
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_RTVs(_In_ ID3D12DescriptorHeap* pID3D12RTVHeap, _In_ IDXGISwapChain3* pIDXGISwapChain3, _Out_ ComPtr<ID3D12Resource> comID3D12RenderTargetResources[]) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12RTVHeap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT Idx = 0; Idx < pID3D12RTVHeap->GetDesc().NumDescriptors; ++Idx) {

		RETURN_IF_FAILED(pIDXGISwapChain3->GetBuffer(Idx, IID_PPV_ARGS(&comID3D12RenderTargetResources[Idx])));
		pID3D12Device4->CreateRenderTargetView(comID3D12RenderTargetResources[Idx].Get(), nullptr, stD3D12CPURTVHandle);

		stD3D12CPURTVHandle.ptr += pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_DSVs(_In_ ID3D12DescriptorHeap* pID3D12DSVHeap, _In_ DXGI_FORMAT enDXGIFOrmat, _In_ UINT64 nWidth, _In_ UINT nHeight, _Out_ ComPtr<ID3D12Resource> comID3D12DepthStencilResources[]) {

	D3D12_HEAP_PROPERTIES stD3D12HeapProperties = Fill_D3D12_Heap_Properties(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC	stD3D12DepthStencilResourceDesc = Fill_D3D12_Resource_Desc_Tex2D(enDXGIFOrmat, nWidth, nHeight, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	stD3D12DepthStencilResourceDesc.MipLevels = 0;

	D3D12_CLEAR_VALUE stD3D12DepthStencilClearValue = {};
	{
		stD3D12DepthStencilClearValue.Format = enDXGIFOrmat;
		stD3D12DepthStencilClearValue.DepthStencil.Depth = 1.0f;
		stD3D12DepthStencilClearValue.DepthStencil.Stencil = 0;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC stD3D12DepthStencilViewDesc = {};
	{
		stD3D12DepthStencilViewDesc.Format = enDXGIFOrmat;
		stD3D12DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		stD3D12DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	}

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12DSVHeap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUDSVHandle = pID3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT Idx = 0; Idx < pID3D12DSVHeap->GetDesc().NumDescriptors; ++Idx) {

		//Create Resource In Auto Heap
		{
			RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12HeapProperties
				, D3D12_HEAP_FLAG_NONE
				, &stD3D12DepthStencilResourceDesc
				, D3D12_RESOURCE_STATE_DEPTH_WRITE
				, &stD3D12DepthStencilClearValue
				, IID_PPV_ARGS(&comID3D12DepthStencilResources[Idx])
			));
		}

		//View
		pID3D12Device4->CreateDepthStencilView(comID3D12DepthStencilResources[Idx].Get(), &stD3D12DepthStencilViewDesc, stD3D12CPUDSVHandle);
		stD3D12CPUDSVHandle.ptr += pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Heap(_In_ ID3D12Device4* pID3D12Device4, _In_ D3D12_HEAP_TYPE enD3D12HeapType, _In_ UINT64 n64TotalByteSize, _COM_Outptr_ ID3D12Heap** ppID3D12Heap, _In_opt_ D3D12_HEAP_FLAGS enD3D12HeapFlags = D3D12_HEAP_FLAG_NONE, _In_opt_ UINT64 Alignment = 0) {

	D3D12_HEAP_DESC	stD3D12HeapDesc = {};
	{
		stD3D12HeapDesc.SizeInBytes = n64TotalByteSize;
		stD3D12HeapDesc.Alignment = Alignment;
		stD3D12HeapDesc.Properties = Fill_D3D12_Heap_Properties(enD3D12HeapType);
		stD3D12HeapDesc.Flags = enD3D12HeapFlags;
	}

	RETURN_IF_FAILED(pID3D12Device4->CreateHeap(&stD3D12HeapDesc, IID_PPV_ARGS(ppID3D12Heap)));

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Placed_Resource(_In_ const D3D12_RESOURCE_DESC* stD3D12ResourceDesc, _In_ ID3D12Heap* pID3D12Heap, _In_ UINT64 nHeapOffset, _In_ D3D12_RESOURCE_STATES enD3D12ResourceState, _COM_Outptr_ ID3D12Resource** ppID3D12Resource) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12Heap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	RETURN_IF_FAILED(pID3D12Device4->CreatePlacedResource(pID3D12Heap, nHeapOffset, stD3D12ResourceDesc, enD3D12ResourceState, nullptr, IID_PPV_ARGS(ppID3D12Resource)));

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Placed_Resource_Buffer(_In_ UINT64 nTotailByteSize, _In_ ID3D12Heap* pID3D12Heap, _In_ UINT64 nHeapOffset, _In_ D3D12_RESOURCE_STATES enD3D12ResourceState, _COM_Outptr_ ID3D12Resource** ppID3D12BufferResource) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12Heap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_RESOURCE_DESC stD3D12BufferResourceDesc = Fill_D3D12_Resource_Desc_Buffer(nTotailByteSize);
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource(&stD3D12BufferResourceDesc, pID3D12Heap, nHeapOffset, enD3D12ResourceState, ppID3D12BufferResource));

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Placed_Resource_Tex2D(_In_ DXGI_FORMAT enDXGITex2DFormat, _In_ UINT64 nWidth, _In_ UINT nHeight, _In_ ID3D12Heap* pID3D12Heap, _In_ UINT64 nHeapOffset, _In_ D3D12_RESOURCE_STATES enD3D12ResourceState, _COM_Outptr_ ID3D12Resource** ppID3D12Tex2DResource) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12Heap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_RESOURCE_DESC stD3D12Tex2DResourceDesc = Fill_D3D12_Resource_Desc_Tex2D(enDXGITex2DFormat, nWidth, nHeight);

	RETURN_IF_FAILED(Create_D3D12_Placed_Resource(&stD3D12Tex2DResourceDesc, pID3D12Heap, nHeapOffset, enD3D12ResourceState, ppID3D12Tex2DResource));

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Set_D3D12_CBV(_In_ D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUCSRUVHandle, _In_ ID3D12Resource* pID3D12ConstantBufferResource) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12ConstantBufferResource->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC	stD3D12ConstantBufferViewDesc = {};
	{
		stD3D12ConstantBufferViewDesc.BufferLocation = pID3D12ConstantBufferResource->GetGPUVirtualAddress();
		stD3D12ConstantBufferViewDesc.SizeInBytes = static_cast<UINT>(pID3D12ConstantBufferResource->GetDesc().Width);
	}

	pID3D12Device4->CreateConstantBufferView(&stD3D12ConstantBufferViewDesc, stD3D12CPUCSRUVHandle);

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Set_D3D12_SRV(_In_ D3D12_CPU_DESCRIPTOR_HANDLE stD3D12DescriptorHandle, _In_ ID3D12Resource* pID3D12ShaderResource, _In_ D3D12_SRV_DIMENSION enD3D12SRVDimension) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12ShaderResource->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_SHADER_RESOURCE_VIEW_DESC	stD3D12SRVDesc = {};
	{
		D3D12_RESOURCE_DESC stD3D12ResourceDesc = pID3D12ShaderResource->GetDesc();

		stD3D12SRVDesc.Format = stD3D12ResourceDesc.Format;
		stD3D12SRVDesc.ViewDimension = enD3D12SRVDimension;
		stD3D12SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		switch (enD3D12SRVDimension) {
		case D3D12_SRV_DIMENSION_TEXTURE2D: {
			stD3D12SRVDesc.Texture2D.MipLevels = stD3D12ResourceDesc.MipLevels;
		}
										  break;
		case D3D12_SRV_DIMENSION_TEXTURECUBE: {
			stD3D12SRVDesc.TextureCube.MipLevels = stD3D12ResourceDesc.MipLevels;
		}
											break;
		}
	}
	pID3D12Device4->CreateShaderResourceView(pID3D12ShaderResource, &stD3D12SRVDesc, stD3D12DescriptorHandle);

	pID3D12Device4->Release();

	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_Samplers(_In_ D3D12_SAMPLER_DESC* stD3D12SamplerDescs, _In_ ID3D12DescriptorHeap* pID3D12SamplerHeap) {

	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12SamplerHeap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));

	D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUSamplerHandle = pID3D12SamplerHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT Idx = 0; Idx < pID3D12SamplerHeap->GetDesc().NumDescriptors; ++Idx) {

		pID3D12Device4->CreateSampler(&stD3D12SamplerDescs[Idx], stD3D12CPUSamplerHandle);
		stD3D12CPUSamplerHandle.ptr += pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	pID3D12Device4->Release();

	return S_OK;
}


//DDSTexLoader
_Check_return_ HRESULT Create_DDSTex_Placed_Resource(_In_ ID3D12Device4* pID3D12Device4, _In_ const wstring* pwstrDDSFileName, _In_ ID3D12Heap* pID3D12Heap, _Inout_ UINT64 nHeapOffset, _COM_Outptr_ ID3D12Resource** ppID3D12Resource, _In_ std::unique_ptr<uint8_t[]>& ddsData, _Outref_ vector<D3D12_SUBRESOURCE_DATA>& refD3D12SubResourcDatas) {

	ID3D12Resource* pID3D12CommitedResource = nullptr;
	RETURN_IF_FAILED(LoadDDSTextureFromFile(pID3D12Device4, pwstrDDSFileName->c_str(), &pID3D12CommitedResource, ddsData, refD3D12SubResourcDatas, SIZE_MAX));

	D3D12_RESOURCE_DESC stD3D12ResourceDesc = pID3D12CommitedResource->GetDesc();
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource(&stD3D12ResourceDesc, pID3D12Heap, nHeapOffset, D3D12_RESOURCE_STATE_COPY_DEST, ppID3D12Resource));

	return S_OK;
}


//Thread Global-----------------------------------------------------------------------------------------------

ComPtr<ID3D12Device4>						g_pID3D12Device4;

struct ST_GRS_THREAD_PARAMS{
	UINT									nIndex;				//ThreadNmae

	DWORD									dwThisThreadID;
	DWORD									dwMainThreadID;

	HANDLE									hThisThread;
	HANDLE									hMainThread;

	HANDLE									hRunEvent;
	HANDLE									hEventRenderOver;

	XMFLOAT4								v4ModelPos;

	wstring									wstrDDSFilePath;
	wstring									wstrMeshFilePath;

	D3D12_VIEWPORT							stViewPort;
	D3D12_RECT								stScissorRect;

	D3D12_CPU_DESCRIPTOR_HANDLE				D3D12CPURTVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE				D3D12CPUDSVHanlde;

	ID3D12CommandAllocator* pID3D12DirectCommandAlloctor;
	ID3D12GraphicsCommandList* pID3D12DirectCommandList;

	ID3D12RootSignature* pID3D12RootSignature;
	ID3D12PipelineState* pID3D12PipelineState;
};
const UINT									nMaxThreadNum = 3;
enum EN_GRS_THREAD { g_enThreadSphere, g_enThreadCube, g_enThreadPlane };
ST_GRS_THREAD_PARAMS						g_stThreadParams[nMaxThreadNum];

//CBV
struct MVPBuffer {
	DirectX::XMFLOAT4X4 m_MVP;
};

//Vertex
struct VERTEX {
	XMFLOAT4 m_vPos;		//Position
	XMFLOAT2 m_vTex;		//Texcoord
	XMFLOAT3 m_vNor;		//Normal
};

//MVP
XMFLOAT4X4									g_mat4Model2 = {}; 
XMFLOAT4X4									g_mat4VP = {};  

//time
UINT64										g_n64FrameStartTime = 0;
UINT64										g_n64FrameCurrentTime = 0;
//------------------------------------------------------------------------------------------------------------

//Camera------------------------------------------------------------------------------------------------------
XMFLOAT3									g_f3EyePos = XMFLOAT3(0.0f, 5.0f, -10.0f); 
XMFLOAT3									g_f3LockAt = XMFLOAT3(0.0f, 0.0f, 0.0f);   
XMFLOAT3									g_f3HeapUp = XMFLOAT3(0.0f, 1.0f, 0.0f);   

double										g_fPalstance = 10.0f * XM_PI / 180.0f;	
//------------------------------------------------------------------------------------------------------------

void Load_Mesh_Data(_In_ const wstring* wstrMeshFilePath, _Outref_ vector<VERTEX>& refvecVertexData, _Outref_ vector<UINT>& refvecIndexData) {

	std::ifstream fin;
	char input;
	fin.open(*wstrMeshFilePath);

	UINT count = 0;
	fin.get(input);
	while (input != ':')
		fin.get(input);
	fin >> count;

	fin.get(input);
	while (input != ':')
		fin.get(input);
	fin.get(input);
	fin.get(input);

	refvecVertexData.resize(count);
	refvecIndexData.resize(count);

	for (UINT i = 0; i < count; ++i) {

		fin >> refvecVertexData[i].m_vPos.x >> refvecVertexData[i].m_vPos.y >> refvecVertexData[i].m_vPos.z;
		refvecVertexData[i].m_vPos.w = 1.0f;
		fin >> refvecVertexData[i].m_vTex.x >> refvecVertexData[i].m_vTex.y;
		fin >> refvecVertexData[i].m_vNor.x >> refvecVertexData[i].m_vNor.y >> refvecVertexData[i].m_vNor.z;

		refvecIndexData[i] = i;
	}
}

LRESULT CALLBACK Wnd_Proc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {

	switch (message) {
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

UINT WINAPI Render_Thread(void* Param) {

	ST_GRS_THREAD_PARAMS* pstThreadParam = reinterpret_cast<ST_GRS_THREAD_PARAMS*>(Param);

	//CSU Discriptor Heap-------------------------------------------------------------------------------------

	ComPtr<ID3D12DescriptorHeap>				pID3D12CSUVHeap;
	const UINT									nCSUVNum = 2;
	UINT										nCSUVHandleSize = 0;

	//CBV
	ComPtr<ID3D12Resource>						pID3D12ConstantBufferResource;
	const UINT64								nConstantBufferByteSize = GRS_ALIGNMENT_UPPER(sizeof(MVPBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	LPVOID										pConstBufferData = nullptr;

	ComPtr<ID3D12Heap>							pID3D12ConstBufferHeap;
	const UINT64								nConstBufferHeapSize = Alignment_Upper(nConstantBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	UINT64										nConstantBufferHeapOffset = 0;

	//SRV
	ComPtr<ID3D12Resource>						pID3D12ShaderResource;
	UINT64										nShaderResourceByteSize = 0;

	ComPtr<ID3D12Heap>							pID3D12ShaderResourceHeap;
	const UINT64								nShaderResourceHeapSize = static_cast<UINT64>(70 * 1024) * 1024;
	UINT64										nShaderResourceHeapOffset = 0;

	//Upload Heap
	ComPtr<ID3D12Resource>						pID3D12UploadShaderResource;

	ComPtr<ID3D12Heap>							pID3D12UploadHeap;
	UINT64										nUpLoadHeapSize = 0;
	UINT64										nUploadHeapOffset = 0;

	//Sampler Descriptor Heap
	ComPtr<ID3D12DescriptorHeap>				pID3D12SamplerHeap;
	const UINT									nSamplerNum = 1;
	//------------------------------------------------------------------------------------------------------------

	//Vertex------------------------------------------------------------------------------------------------------

	ComPtr<ID3D12Resource>						pID3D12VertexBufferResource;
	UINT										nVertexBufferByteSize = 0;
	D3D12_VERTEX_BUFFER_VIEW					stD3D12VertexBufferView = {};

	ComPtr<ID3D12Heap>							pID3D12VertexHeap;
	UINT64										nVertexHeapSize = 0;
	UINT64										nVertexHeapOffset = 0;
	//------------------------------------------------------------------------------------------------------------


	//Index-------------------------------------------------------------------------------------------------------
	ComPtr<ID3D12Resource>						pID3D12IndexBufferResource;
	UINT										nIndexBufferByteSize = 0;
	D3D12_INDEX_BUFFER_VIEW						stD3D12IndexBufferView = {};

	ComPtr<ID3D12Heap>							pID3D12IndexHeap;
	UINT64										nIndexHeapSize = 0;
	UINT64										nIndexHeapOffset = 0;
	//------------------------------------------------------------------------------------------------------------

	//DDSTex Data
	std::unique_ptr<uint8_t[]>					ddsData;

	//Vertex Data
	vector<VERTEX>								vecVertexData;
	UINT										nVertexCount = 0;

	//Index Data
	vector<UINT>								vecIndexData;
	UINT										nIndexCount = 0;

	const D3D12_RANGE							stReadRange = { 0,0 };

	try {
		//Load Thread Assert-----------------------------------------------------------------------------------
		{
			//CSU Diescriptor Heap-----------------------------------------------------------------------------
			{
				THROW_IF_FAILED(Create_D3D12_Descriptor_Heap(g_pID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nCSUVNum, &pID3D12CSUVHeap));
				nCSUVHandleSize = g_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUCSUVHandle = pID3D12CSUVHeap->GetCPUDescriptorHandleForHeapStart();

				//CBV
				{
					//Constant Resource Heap
					THROW_IF_FAILED(Create_D3D12_Heap(g_pID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, nConstBufferHeapSize, &pID3D12ConstBufferHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

					//Heap Placeed Const Buffer
					THROW_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(nConstantBufferByteSize, pID3D12ConstBufferHeap.Get(), nConstantBufferHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &pID3D12ConstantBufferResource));
					nConstantBufferHeapOffset = Alignment_Upper(nConstantBufferHeapOffset + nConstantBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

					//Const Buffer Data
					THROW_IF_FAILED(pID3D12ConstantBufferResource->Map(0, &stReadRange, &pConstBufferData));

					//CBV
					THROW_IF_FAILED(Set_D3D12_CBV(stD3D12CPUCSUVHandle, pID3D12ConstantBufferResource.Get()));
					stD3D12CPUCSUVHandle.ptr += nCSUVHandleSize;
				}

				//SRV
				{
					//Shader Resource Heap
					THROW_IF_FAILED(Create_D3D12_Heap(g_pID3D12Device4.Get(), D3D12_HEAP_TYPE_DEFAULT, nShaderResourceHeapSize, &pID3D12ShaderResourceHeap, D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

					//Heap Resource Data
					vector<D3D12_SUBRESOURCE_DATA> vecD3D12SubResourceData;

					//Shader Resource
					THROW_IF_FAILED(Create_DDSTex_Placed_Resource(g_pID3D12Device4.Get(), &pstThreadParam->wstrDDSFilePath, pID3D12ShaderResourceHeap.Get(), nShaderResourceHeapOffset, &pID3D12ShaderResource, ddsData, vecD3D12SubResourceData));
					nShaderResourceByteSize = GetRequiredIntermediateSize(pID3D12ShaderResource.Get(), 0, static_cast<UINT>(vecD3D12SubResourceData.size()));
					nShaderResourceHeapOffset = Alignment_Upper(nShaderResourceHeapOffset + nShaderResourceByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

					//Upload Shader Heap
					nUpLoadHeapSize = Alignment_Upper(nShaderResourceByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
					THROW_IF_FAILED(Create_D3D12_Heap(g_pID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, nUpLoadHeapSize, &pID3D12UploadHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

					//Uplaod Shader Resource
					{
						THROW_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(nShaderResourceByteSize, pID3D12UploadHeap.Get(), nUploadHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &pID3D12UploadShaderResource));
						nUploadHeapOffset = Alignment_Upper(nUploadHeapOffset + nShaderResourceByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

						UpdateSubresources(pstThreadParam->pID3D12DirectCommandList, pID3D12ShaderResource.Get(), pID3D12UploadShaderResource.Get(), 0, 0, static_cast<UINT>(vecD3D12SubResourceData.size()), vecD3D12SubResourceData.data());
						D3D12_RESOURCE_BARRIER stD3D12ShaderResourceBarrier = Fill_D3D12_Barrier_Transition(pID3D12ShaderResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
						pstThreadParam->pID3D12DirectCommandList->ResourceBarrier(1, &stD3D12ShaderResourceBarrier);
					}

					//SRV
					THROW_IF_FAILED(Set_D3D12_SRV(stD3D12CPUCSUVHandle, pID3D12ShaderResource.Get(), D3D12_SRV_DIMENSION_TEXTURE2D));
				}
			}

			//Sampler------------------------------------------------------------------------------------------
			{
				THROW_IF_FAILED(Create_D3D12_Descriptor_Heap(g_pID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, nSamplerNum, &pID3D12SamplerHeap));

				D3D12_SAMPLER_DESC	stD3D12SamplerDescs = Fill_D3D12_Sampler_Desc();
				THROW_IF_FAILED(Create_D3D12_Samplers(&stD3D12SamplerDescs, pID3D12SamplerHeap.Get()));
			}

			//Vertex And Index Buffer--------------------------------------------------------------------------
			{
				//Load
				{
					Load_Mesh_Data(&pstThreadParam->wstrMeshFilePath, vecVertexData, vecIndexData);

					nVertexCount = static_cast<UINT>(vecVertexData.size());
					nIndexCount = static_cast<UINT>(vecIndexData.size());

					nVertexBufferByteSize = nVertexCount * sizeof(VERTEX);
					nIndexBufferByteSize = nIndexCount * sizeof(UINT);

					nVertexHeapSize = Alignment_Upper(nVertexBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
					nIndexHeapSize = Alignment_Upper(nIndexBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
				}

				//Heap
				THROW_IF_FAILED(Create_D3D12_Heap(g_pID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, nVertexHeapSize, &pID3D12VertexHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
				THROW_IF_FAILED(Create_D3D12_Heap(g_pID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, nIndexHeapSize, &pID3D12IndexHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

				//Vertex View
				{
					//Vertex Buffer Resource
					THROW_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(nVertexBufferByteSize, pID3D12VertexHeap.Get(), nVertexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &pID3D12VertexBufferResource));
					nVertexHeapOffset = Alignment_Upper(nVertexHeapOffset + nVertexBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

					//Vertex Buffer View
					stD3D12VertexBufferView = Fill_D3D12_VertexBufferView(sizeof(VERTEX), pID3D12VertexBufferResource.Get());

					//Vertex Data
					LPVOID	pVertexData = nullptr;
					THROW_IF_FAILED(pID3D12VertexBufferResource->Map(0, &stReadRange, &pVertexData));
					memcpy(pVertexData, vecVertexData.data(), nVertexBufferByteSize);
					pID3D12VertexBufferResource->Unmap(0, nullptr);
				}

				//Index View
				{
					//Index Buffer Resource
					THROW_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(nIndexBufferByteSize, pID3D12IndexHeap.Get(), nIndexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &pID3D12IndexBufferResource));
					nIndexHeapOffset = Alignment_Upper(nIndexHeapOffset + nIndexBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

					//Index Buffer View
					stD3D12IndexBufferView = Fill_D3D12_IndexBufferView(nIndexBufferByteSize, pID3D12IndexBufferResource.Get());

					//Index Data
					LPVOID pIndexData = nullptr;
					THROW_IF_FAILED(pID3D12IndexBufferResource->Map(0, &stReadRange, &pIndexData));
					memcpy(pIndexData, vecIndexData.data(), nIndexBufferByteSize);
					pID3D12IndexBufferResource->Unmap(0, nullptr);
				}
			}

			THROW_IF_FAILED(pstThreadParam->pID3D12DirectCommandList->Close());
		}

		//Updata Sync Data
		::SetEvent(pstThreadParam->hEventRenderOver);

		//Loop-------------------------------------------------------------------------------------------------

		// Only Used In Sphere
		float										fJmpSpeed = 0.003f;
		float										fUp = 1.0f;
		float										fRawYPos = pstThreadParam->v4ModelPos.y;

		XMMATRIX									XmatPosModuel1 = XMMatrixTranslationFromVector(XMLoadFloat4(&pstThreadParam->v4ModelPos));

		MSG												msg = {};
		BOOL											bQuit = FALSE;
		//-----------------------------------------------------------------------------------------------------

		while (!bQuit) {
			DWORD dwMsgWaitRet = ::MsgWaitForMultipleObjects(1, &pstThreadParam->hRunEvent, FALSE, INFINITE, QS_ALLPOSTMESSAGE);
			switch (dwMsgWaitRet) {
			case WAIT_OBJECT_0: {
				// Updata
				{
					//Updata Model1
					if (g_enThreadSphere == pstThreadParam->nIndex) {

						if (pstThreadParam->v4ModelPos.y >= 2.0f * fRawYPos) {
							fUp = -1.0f;
							pstThreadParam->v4ModelPos.y = 2.0f * fRawYPos;
						}
						if (pstThreadParam->v4ModelPos.y <= fRawYPos) {
							fUp = 1.0f;
							pstThreadParam->v4ModelPos.y = fRawYPos;
						}

						pstThreadParam->v4ModelPos.y += fUp * fJmpSpeed * static_cast<float>(g_n64FrameCurrentTime - g_n64FrameStartTime);

						XmatPosModuel1 = XMMatrixTranslationFromVector(XMLoadFloat4(&pstThreadParam->v4ModelPos));
					}

					//Module 
					XMMATRIX XmatMVP = XMMatrixMultiply(XmatPosModuel1, XMLoadFloat4x4(&g_mat4Model2));

					// MVP
					XmatMVP = XMMatrixMultiply(XmatMVP, XMLoadFloat4x4(&g_mat4VP));

					XMStoreFloat4x4(&reinterpret_cast<MVPBuffer*>(pConstBufferData)->m_MVP, XMMatrixTranspose(XmatMVP));	
				}

				//Reset
				THROW_IF_FAILED(pstThreadParam->pID3D12DirectCommandAlloctor->Reset());
				THROW_IF_FAILED(pstThreadParam->pID3D12DirectCommandList->Reset(pstThreadParam->pID3D12DirectCommandAlloctor, nullptr));

				//Set View
				pstThreadParam->pID3D12DirectCommandList->RSSetViewports(1, &pstThreadParam->stViewPort);
				pstThreadParam->pID3D12DirectCommandList->RSSetScissorRects(1, &pstThreadParam->stScissorRect);

				//Render Target
				pstThreadParam->pID3D12DirectCommandList->OMSetRenderTargets(1, &pstThreadParam->D3D12CPURTVHandle, FALSE, &pstThreadParam->D3D12CPUDSVHanlde);

				//Render Command
				{
					pstThreadParam->pID3D12DirectCommandList->SetGraphicsRootSignature(pstThreadParam->pID3D12RootSignature);
					pstThreadParam->pID3D12DirectCommandList->SetPipelineState(pstThreadParam->pID3D12PipelineState);

					ID3D12DescriptorHeap* const pID3D12DescriptorHeap[] = { pID3D12CSUVHeap.Get(),pID3D12SamplerHeap.Get() };
					pstThreadParam->pID3D12DirectCommandList->SetDescriptorHeaps(_countof(pID3D12DescriptorHeap), pID3D12DescriptorHeap);

					D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUCSUVHandle = pID3D12CSUVHeap->GetGPUDescriptorHandleForHeapStart();
					pstThreadParam->pID3D12DirectCommandList->SetGraphicsRootDescriptorTable(0, stD3D12GPUCSUVHandle);

					stD3D12GPUCSUVHandle.ptr += nCSUVHandleSize;
					pstThreadParam->pID3D12DirectCommandList->SetGraphicsRootDescriptorTable(1, stD3D12GPUCSUVHandle);

					D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUSamplerHandle = pID3D12SamplerHeap->GetGPUDescriptorHandleForHeapStart();
					pstThreadParam->pID3D12DirectCommandList->SetGraphicsRootDescriptorTable(2, stD3D12GPUSamplerHandle);

					pstThreadParam->pID3D12DirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					pstThreadParam->pID3D12DirectCommandList->IASetVertexBuffers(0, 1, &stD3D12VertexBufferView);
					pstThreadParam->pID3D12DirectCommandList->IASetIndexBuffer(&stD3D12IndexBufferView);

					pstThreadParam->pID3D12DirectCommandList->DrawIndexedInstanced(nIndexCount, 1, 0, 0, 0);

					THROW_IF_FAILED(pstThreadParam->pID3D12DirectCommandList->Close());
				}

				//Up Sync Data
				::SetEvent(pstThreadParam->hEventRenderOver);
			}
							  break;
			case WAIT_OBJECT_0 + 1: {
				while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					if (WM_QUIT != msg.message) {
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					else
						bQuit = TRUE;
				}
			}
								  break;
			case WAIT_FAILED:
				throw HRESULT_FROM_WIN32(GetLastError());
			}
		}

		//UnInit Data
		ddsData.reset(nullptr);

	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		return e.Error();
	}

	_endthreadex(0);
	return 0;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

	//Initialize--------------------------------------------------------------------------------------------------

	::CoInitialize(nullptr);
	/*if (S_OK != Windows::Foundation::Initialize(RO_INIT_MULTITHREADED))
		return  -1;*/
	//------------------------------------------------------------------------------------------------------------
	
	//Const Value-------------------------------------------------------------------------------------------------

	//Window
	const wstring								wstrWndClassName = L"Class Name";
	const wstring								wstrWindowExName = L"Window Name";

	//Swap Chain
	const DXGI_FORMAT							enDXGIRenderTargetormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const UINT									nBackBufferCount = 3;

	const UINT									nRTVNum = nBackBufferCount;

	const DXGI_FORMAT							enDXGIDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const UINT									nDSVNum = 1;

	const wstring								wstrShaderFileName = L"Texture_Cube.hlsl";
	//------------------------------------------------------------------------------------------------------------
	
	//Local Value-------------------------------------------------------------------------------------------------

	//Path
	wstring										wstrCatallogPath = {};

	//Window
	const UINT									nClientWidth = 800;
	const UINT									nClientHeight = 600;
	HWND										hWnd = nullptr;

	//D3D12 Pipeline----------------------------------------------------------------------------------------------

	ComPtr<IDXGIFactory6>						pIDXGIFactory6;

	ComPtr<ID3D12CommandQueue>					pID3D12DirectCommandQueue;

	ComPtr<IDXGISwapChain3>						pIDXGISwapChain3;

	ComPtr<ID3D12DescriptorHeap>				pID3D12RTVHeap;
	UINT										nRTVHandleSize = 0;

	ComPtr<ID3D12DescriptorHeap>				pID3D12DSVHeap;
	UINT										nDSVHandleSize = 0;

	//Direct Command List
	ComPtr<ID3D12CommandAllocator>				pID3D12PostDirectCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>			pID3D12PostDirectCommandList;

	ComPtr<ID3D12CommandAllocator>				pID3D12PreDirectCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>			pID3D12PreDirectCommandList;

	ComPtr<ID3D12RootSignature>					pID3D12RootSignature;

	ComPtr<ID3D12PipelineState>					pID3D12PipelineState;

	ComPtr<ID3D12Resource>						pID3D12RenderTargetResources[nRTVNum];

	ComPtr<ID3D12Resource>						pID3D12DepthStencilResources[nDSVNum];

	//Fence
	ComPtr<ID3D12Fence>							pID3D12Fence;
	UINT64										nFenceCPUValue = 0;
	HANDLE										hFenceEvent = nullptr;
	//SubThread------------------------------------------------------------------------------------------------------

	vector<HANDLE>								vechSubThread;

	const wstring								wstrDDSFileName[nMaxThreadNum] = { L"Assets\\sphere.dds",L"Assets\\Cube.dds",L"Assets\\Plane.dds" };
	const wstring								wstrMeshFileName[nMaxThreadNum] = { L"Assets\\sphere.txt",L"Assets\\Cube.txt",L"Assets\\Plane.txt" };
	//------------------------------------------------------------------------------------------------------------
	
	try {

		//Path----------------------------------------------------------------------------------------------------
		THROW_IF_FAILED(Get_CatallogPath(&wstrCatallogPath));

		//Window--------------------------------------------------------------------------------------------------
		THROW_IF_FAILED(Register_WndClassEx(hInstance, &wstrWndClassName, Wnd_Proc));
		THROW_IF_FAILED(Initialize_WindowEx(hInstance, &wstrWndClassName, &wstrWindowExName, nClientWidth, nClientHeight, &hWnd));

		//Load Pipeline--------------------------------------------------------------------------------------------
		{
			//Debug InterFace
			UINT nDXGIFactoryFlags = 0;
			THROW_IF_FAILED(Set_D3D12_Debug(&nDXGIFactoryFlags));

			//DXGI Factory
			THROW_IF_FAILED(Create_DXGI_Factory(nDXGIFactoryFlags, &pIDXGIFactory6));
			THROW_IF_FAILED(pIDXGIFactory6->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

			//DXGIAdapter
			IDXGIAdapter1* pIDXGIAdapter1 = nullptr;
			THROW_IF_FAILED(Create_DXGI_Adapter(pIDXGIFactory6.Get(), &pIDXGIAdapter1));
			THROW_IF_FAILED(Create_D3D12_Device(pIDXGIAdapter1, D3D_FEATURE_LEVEL_12_1, &g_pID3D12Device4));

			THROW_IF_FAILED(Create_D3D12_Direct_Command_Queue(g_pID3D12Device4.Get(), &pID3D12DirectCommandQueue));

			THROW_IF_FAILED(Create_DXGI_SwapChain(pIDXGIFactory6.Get(), enDXGIRenderTargetormat, nClientWidth, nClientHeight, nBackBufferCount, pID3D12DirectCommandQueue.Get(), hWnd, pIDXGISwapChain3));

			//RTV Heap
			THROW_IF_FAILED(Create_D3D12_Descriptor_Heap(g_pID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, nRTVNum, &pID3D12RTVHeap));
			nRTVHandleSize = g_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			//DSV Heap
			THROW_IF_FAILED(Create_D3D12_Descriptor_Heap(g_pID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, nDSVNum, &pID3D12DSVHeap));
			//nDSVHandleSize = g_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			//Direct Command List
			THROW_IF_FAILED(Create_D3D12_Command_Allocator(g_pID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &pID3D12PreDirectCommandAllocator));
			THROW_IF_FAILED(Create_D3D12_Graphics_Command_List(pID3D12PreDirectCommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &pID3D12PreDirectCommandList));
			THROW_IF_FAILED(pID3D12PreDirectCommandList->Close());

			THROW_IF_FAILED(Create_D3D12_Command_Allocator(g_pID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &pID3D12PostDirectCommandAllocator));
			THROW_IF_FAILED(Create_D3D12_Graphics_Command_List(pID3D12PostDirectCommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &pID3D12PostDirectCommandList));
			THROW_IF_FAILED(pID3D12PostDirectCommandList->Close());
		}
		
		//Load Assert----------------------------------------------------------------------------------------------
		{
			THROW_IF_FAILED(Create_D3D12_Root_Signature(g_pID3D12Device4.Get(), &pID3D12RootSignature));

			//Pipeline State
			{
				D3D12_BLEND_DESC stD3D12BlendDesc = Fill_D3D12_Blend_Desc();
				D3D12_RASTERIZER_DESC stD3D12RasterizerDesc = Fill_D3D12_Rasterizer_Desc();
				D3D12_DEPTH_STENCIL_DESC stD3D12DepthStencilDesc = Fill_D3D12_DepthStencil_Desc();
				D3D12_INPUT_LAYOUT_DESC stD3D12InputDesc = Fill_D3D12_Input_Layout();

				THROW_IF_FAILED(Create_D3D12_Pipeline_State(pID3D12RootSignature.Get(), &wstrShaderFileName, &stD3D12BlendDesc, &stD3D12RasterizerDesc, &stD3D12DepthStencilDesc, &stD3D12InputDesc, enDXGIRenderTargetormat, enDXGIDepthStencilFormat, &pID3D12PipelineState));
			}

			//Fence
			THROW_IF_FAILED(Create_D3D12_Fence(g_pID3D12Device4.Get(), nFenceCPUValue++, &pID3D12Fence));
			if (nullptr == (hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr)))
				throw HRESULT_FROM_WIN32(GetLastError());

			THROW_IF_FAILED(Create_D3D12_RTVs(pID3D12RTVHeap.Get(), pIDXGISwapChain3.Get(), pID3D12RenderTargetResources));

			THROW_IF_FAILED(Create_D3D12_DSVs(pID3D12DSVHeap.Get(), enDXGIDepthStencilFormat, nClientWidth, nClientHeight, pID3D12DepthStencilResources));

		}

		//SubThread------------------------------------------------------------------------------------------------
		{
			//Sphere
			g_stThreadParams[g_enThreadSphere].wstrDDSFilePath = wstrCatallogPath + wstrDDSFileName[g_enThreadSphere];
			g_stThreadParams[g_enThreadSphere].wstrMeshFilePath = wstrCatallogPath + wstrMeshFileName[g_enThreadSphere];
			g_stThreadParams[g_enThreadSphere].v4ModelPos = XMFLOAT4(2.0f, 2.0f, 0.0f, 1.0f);

			//Cube
			g_stThreadParams[g_enThreadCube].wstrDDSFilePath = wstrCatallogPath + wstrDDSFileName[g_enThreadCube];
			g_stThreadParams[g_enThreadCube].wstrMeshFilePath = wstrCatallogPath + wstrMeshFileName[g_enThreadCube];
			g_stThreadParams[g_enThreadCube].v4ModelPos = XMFLOAT4(-2.0f, 2.0f, 0.0f, 1.0f);

			//Plane
			g_stThreadParams[g_enThreadPlane].wstrDDSFilePath = wstrCatallogPath + wstrDDSFileName[g_enThreadPlane];
			g_stThreadParams[g_enThreadPlane].wstrMeshFilePath = wstrCatallogPath + wstrMeshFileName[g_enThreadPlane];
			g_stThreadParams[g_enThreadPlane].v4ModelPos = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

			for (UINT Index = 0; Index < nMaxThreadNum; ++Index) {

				g_stThreadParams[Index].nIndex = Index;

				THROW_IF_FAILED(Create_D3D12_Command_Allocator(g_pID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &g_stThreadParams[Index].pID3D12DirectCommandAlloctor));

				THROW_IF_FAILED(Create_D3D12_Graphics_Command_List(g_stThreadParams[Index].pID3D12DirectCommandAlloctor, D3D12_COMMAND_LIST_TYPE_DIRECT, &g_stThreadParams[Index].pID3D12DirectCommandList));

				g_stThreadParams[Index].dwMainThreadID = ::GetCurrentThreadId();
				g_stThreadParams[Index].hMainThread = ::GetCurrentThread();

				if (nullptr == (g_stThreadParams[Index].hRunEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr)))
					throw HRESULT_FROM_WIN32(GetLastError());
				if (nullptr == (g_stThreadParams[Index].hEventRenderOver = ::CreateEvent(nullptr, FALSE, FALSE, nullptr)))
					throw HRESULT_FROM_WIN32(GetLastError());

				g_stThreadParams[Index].pID3D12RootSignature = pID3D12RootSignature.Get();
				g_stThreadParams[Index].pID3D12PipelineState = pID3D12PipelineState.Get();

				if (nullptr == (g_stThreadParams[Index].hThisThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, Render_Thread, reinterpret_cast<LPVOID>(&g_stThreadParams[Index]), CREATE_SUSPENDED, reinterpret_cast<UINT*>(&g_stThreadParams[Index].dwThisThreadID)))))
					throw  HRESULT_FROM_WIN32(GetLastError());
					
				vechSubThread.emplace_back(g_stThreadParams[Index].hThisThread);
			}

			for (UINT Index = 0; Index < nMaxThreadNum; ++Index)
				if (static_cast<DWORD> (-1) == ::ResumeThread(g_stThreadParams[Index].hThisThread))
					throw  HRESULT_FROM_WIN32(GetLastError());
		}

		//Loop parameters--------------------------------------------------------------------------------------------

		UINT										nCurrentFrameIndex = 0;

		D3D12_RESOURCE_BARRIER						stD3D12ResourceBarrierBegin = Fill_D3D12_Barrier_Transition(nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		D3D12_RESOURCE_BARRIER						stD3D12REsourceBarrierEnd   = Fill_D3D12_Barrier_Transition(nullptr, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

		D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPURTVHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPUDSVHandle = {};

		CAtlArray<ID3D12CommandList*>				atlpID3D12SGraphicsCommandList;

		double										dModelRotationYAngle = 0;
		
		MSG											msg = {};
		UINT										nThreadState = 0;
		vector<HANDLE>								vechWaitThread;

		BOOL										bExit = FALSE;
		//Initialize--------------------------------------------------------------------------------------------------

		//WIndow
		ShowWindow(hWnd, nCmdShow);

		::SetTimer(hWnd, WM_USER + 100, 1, nullptr);

		for (UINT Index = 0; Index < nMaxThreadNum; ++Index)
			vechWaitThread.emplace_back(g_stThreadParams[Index].hEventRenderOver);

		//start
		g_n64FrameStartTime = ::GetTickCount64();


		//Static Value-----------------------------------------------------------------------------------------------
		const float									ClearColor[] = { 0.2f, 0.5f, 1.0f, 1.0f };

		D3D12_VIEWPORT								stViewPort = { 0.0f, 0.0f, static_cast<float>(nClientWidth), static_cast<float>(nClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		D3D12_RECT									stScissorRect = { 0, 0, static_cast<LONG>(nClientWidth), static_cast<LONG>(nClientHeight) };
		//------------------------------------------------------------------------------------------------------------

		while (!bExit) {
			DWORD dwMsgWaitRet = ::MsgWaitForMultipleObjects(static_cast<DWORD>(vechWaitThread.size()), vechWaitThread.data(), TRUE, INFINITE, QS_ALLINPUT);

			if (WAIT_OBJECT_0 == dwMsgWaitRet) {
				switch (nThreadState) {
				//State0£¬Resouece Copy Commmand---------------------------------------------------------------------
				case 0: {

					//Upload Command
					CAtlArray<ID3D12CommandList*> atlpID3D12UploadCommandList;
					atlpID3D12UploadCommandList.Add(g_stThreadParams[g_enThreadSphere].pID3D12DirectCommandList);
					atlpID3D12UploadCommandList.Add(g_stThreadParams[g_enThreadCube].pID3D12DirectCommandList);
					atlpID3D12UploadCommandList.Add(g_stThreadParams[g_enThreadPlane].pID3D12DirectCommandList);

					pID3D12DirectCommandQueue->ExecuteCommandLists(static_cast<UINT>(atlpID3D12UploadCommandList.GetCount()), atlpID3D12UploadCommandList.GetData());

					//Up Sync Data
					{
						THROW_IF_FAILED(pID3D12DirectCommandQueue->Signal(pID3D12Fence.Get(), nFenceCPUValue));
						THROW_IF_FAILED(pID3D12Fence->SetEventOnCompletion(nFenceCPUValue++, hFenceEvent));

						nThreadState = 1;

						vechWaitThread.clear();
						vechWaitThread.emplace_back(hFenceEvent);
					}
				}
					  break;
				//State1 Get Pre Render Command And Init-------------------------------------------------------------
				case 1: {

					THROW_IF_FAILED(pID3D12PreDirectCommandAllocator->Reset());
					THROW_IF_FAILED(pID3D12PreDirectCommandList->Reset(pID3D12PreDirectCommandAllocator.Get(), nullptr));

					//Initialize
					nCurrentFrameIndex = pIDXGISwapChain3->GetCurrentBackBufferIndex();

					//Pre Render Command
					{
						//Begin
						stD3D12ResourceBarrierBegin.Transition.pResource = pID3D12RenderTargetResources[nCurrentFrameIndex].Get();
						pID3D12PreDirectCommandList->ResourceBarrier(1, &stD3D12ResourceBarrierBegin);

						//Render Target And Depen Stencil  Clear 
						{
							//Render Target
							stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
							stD3D12CPURTVHandle.ptr += static_cast<unsigned long long>(nCurrentFrameIndex) * g_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
							stD3D12CPUDSVHandle = pID3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();

							//Clear
							pID3D12PreDirectCommandList->ClearRenderTargetView(stD3D12CPURTVHandle, ClearColor, 0, nullptr);
							pID3D12PreDirectCommandList->ClearDepthStencilView(stD3D12CPUDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
						}

						THROW_IF_FAILED(pID3D12PreDirectCommandList->Close());
					}

					//Init Frame
					{
						g_n64FrameCurrentTime = ::GetTickCount64();

						//Model2
						{
							dModelRotationYAngle += ((g_n64FrameCurrentTime - g_n64FrameStartTime) / 1000.0f) * g_fPalstance;

							if (dModelRotationYAngle > XM_2PI)
								dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);

							//Model2
							XMStoreFloat4x4(&g_mat4Model2, XMMatrixRotationY(static_cast<float>(dModelRotationYAngle)));
						}

						//VP
						{
							XMStoreFloat4x4(&g_mat4VP
								, XMMatrixMultiply(XMMatrixLookAtLH(XMLoadFloat3(&g_f3EyePos)
									, XMLoadFloat3(&g_f3LockAt)
									, XMLoadFloat3(&g_f3HeapUp))
									, XMMatrixPerspectiveFovLH(XM_PIDIV4
										, static_cast<FLOAT>(nClientWidth) / static_cast<FLOAT>(nClientHeight), 0.1f, 1000.0f)));
						}

						for (int Index = 0; Index < nMaxThreadNum; Index++) {

							g_stThreadParams[Index].D3D12CPURTVHandle = stD3D12CPURTVHandle;
							g_stThreadParams[Index].D3D12CPUDSVHanlde = stD3D12CPUDSVHandle;

							g_stThreadParams[Index].stViewPort = stViewPort;
							g_stThreadParams[Index].stScissorRect = stScissorRect;
						}
					}

					//Updata Sync Data
					{
						nThreadState = 2;

						vechWaitThread.clear();

						for (int Index = 0; Index < nMaxThreadNum; ++Index) {

							SetEvent(g_stThreadParams[Index].hRunEvent);

							vechWaitThread.emplace_back(g_stThreadParams[Index].hEventRenderOver);
						}
					}

				}
					  break;
				//Execute Command------------------------------------------------------------------------------------
				case 2: {

					THROW_IF_FAILED(pID3D12PostDirectCommandAllocator->Reset());
					THROW_IF_FAILED(pID3D12PostDirectCommandList->Reset(pID3D12PostDirectCommandAllocator.Get(), nullptr));

					//End
					stD3D12REsourceBarrierEnd.Transition.pResource=pID3D12RenderTargetResources[nCurrentFrameIndex].Get();
					pID3D12PostDirectCommandList->ResourceBarrier(1, &stD3D12REsourceBarrierEnd);

					THROW_IF_FAILED(pID3D12PostDirectCommandList->Close());

					//Blend Command
					atlpID3D12SGraphicsCommandList.RemoveAll();
					atlpID3D12SGraphicsCommandList.Add(pID3D12PreDirectCommandList.Get());
					atlpID3D12SGraphicsCommandList.Add(g_stThreadParams[g_enThreadCube].pID3D12DirectCommandList);
					atlpID3D12SGraphicsCommandList.Add(g_stThreadParams[g_enThreadPlane].pID3D12DirectCommandList);
					atlpID3D12SGraphicsCommandList.Add(g_stThreadParams[g_enThreadSphere].pID3D12DirectCommandList);
					atlpID3D12SGraphicsCommandList.Add(pID3D12PostDirectCommandList.Get());

					pID3D12DirectCommandQueue->ExecuteCommandLists(static_cast<UINT>(atlpID3D12SGraphicsCommandList.GetCount()), atlpID3D12SGraphicsCommandList.GetData());

					//Present
					THROW_IF_FAILED(pIDXGISwapChain3->Present(1, 0));

					//Init Frame
					g_n64FrameStartTime = g_n64FrameCurrentTime;

					//Updata Sync Data
					{
						nThreadState = 1;

						THROW_IF_FAILED(pID3D12DirectCommandQueue->Signal(pID3D12Fence.Get(), nFenceCPUValue));
						THROW_IF_FAILED(pID3D12Fence->SetEventOnCompletion(nFenceCPUValue++, hFenceEvent));

						vechWaitThread.clear();
						vechWaitThread.emplace_back(hFenceEvent);
					}
				}
					  break;
				}

				while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD)) {
					if (WM_QUIT != msg.message) {
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					else
						bExit = TRUE;
				}
			}

			DWORD dwWaitRet = WaitForMultipleObjects(static_cast<DWORD>(nMaxThreadNum), vechSubThread.data(), FALSE, 0);
			if (dwWaitRet >= WAIT_OBJECT_0 && dwWaitRet < nMaxThreadNum)
				bExit = TRUE;
		}

		//UnInitialize------------------------------------------------------------------------------------------------

		KillTimer(hWnd, WM_USER + 100);
		//------------------------------------------------------------------------------------------------------------

		//Kill Thread-------------------------------------------------------------------------------------------------
		{
			//Post Kill Thread
			for (int Index = 0; Index < nMaxThreadNum; ++Index)
				if (WAIT_OBJECT_0 != ::WaitForSingleObject(vechSubThread[Index], 0))
					::PostThreadMessage(g_stThreadParams[Index].dwThisThreadID, WM_QUIT, 0, 0);

			//Wait for Kill
			if (WAIT_FAILED == WaitForMultipleObjects(static_cast<DWORD>(vechSubThread.size()), vechSubThread.data(), TRUE, INFINITE))
				throw ::HRESULT_FROM_WIN32(::GetLastError());

			//UnInitialize
			for (int Index = 0; Index < nMaxThreadNum; ++Index) {
				::CloseHandle(g_stThreadParams[Index].hThisThread);
				::CloseHandle(g_stThreadParams[Index].hEventRenderOver);

				g_stThreadParams[Index].pID3D12DirectCommandList->Release();
				g_stThreadParams[Index].pID3D12DirectCommandAlloctor->Release();
			}
		}
		//------------------------------------------------------------------------------------------------------------

		//Un Load Pipeline--------------------------------------------------------------------------------------------

		CloseHandle(hFenceEvent);
		//------------------------------------------------------------------------------------------------------------
	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		return e.Error();
	}

	::CoUninitialize();

	return 0;
}
