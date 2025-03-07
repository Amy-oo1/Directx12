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

#define GRS_FLOOR_DIV(A,B)					((UINT)(((A)+((B)-1))/(B)))
#define GRS_UPPER(A,B)						((UINT)(((A)+((B)-1))&~(B - 1)))


//Path
TSTRING										strCatallogPath = {};
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

//WIC Image Factory
ComPtr<IWICImagingFactory>					pIWICImageFactory;
HRESULT Create_WIC_Image_Factory() {
	::CoInitialize(nullptr);  //for WIC & COM
	RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICImageFactory)));
	return S_OK;
}

//WIC Bitmap Decoder
ComPtr<IWICBitmapFrameDecode>				pIWICBitmapFrameDecode;
TSTRING										strImapgeFileName = _T("anbi.jpg");

HRESULT Create_WIC_Bitmap_Frame_Decode() {
	//create Decoder
	TSTRING strImagePath = /*strCatallogPath +*/ strImapgeFileName;
	ComPtr<IWICBitmapDecoder> pIWICBitmapDecoder;
	{
		RETURN_IF_FAILED(pIWICImageFactory->CreateDecoderFromFilename(strImagePath.c_str()
			, nullptr
			, GENERIC_READ
			, WICDecodeMetadataCacheOnDemand
			, &pIWICBitmapDecoder
		));
	}
	RETURN_IF_FAILED(pIWICBitmapDecoder->GetFrame(0, &pIWICBitmapFrameDecode));
	return S_OK;
}

//Bitmap Source
ComPtr<IWICBitmapSource>					pIWICBitmapSource;
WICPixelFormatGUID							WICTargetFormatguid = {};

WICPixelFormatGUID Get_ConvertTo_WICFormat(const WICPixelFormatGUID& WICSourceFormatguid) {
	if (WICSourceFormatguid == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (WICSourceFormatguid == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (WICSourceFormatguid == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (WICSourceFormatguid == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (WICSourceFormatguid == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WICSourceFormatguid == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (WICSourceFormatguid == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (WICSourceFormatguid == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif
	else return GUID_WICPixelFormatDontCare;
}
HRESULT Get_WIC_Bitmap_Source() {
	WICPixelFormatGUID WICSourceFormatguid = {};
	RETURN_IF_FAILED(pIWICBitmapFrameDecode->GetPixelFormat(&WICSourceFormatguid));
	WICTargetFormatguid = Get_ConvertTo_WICFormat(WICSourceFormatguid);
	if (WICTargetFormatguid == GUID_WICPixelFormatDontCare)
		return E_FAIL;
	//Source To Target
	{
		if (!InlineIsEqualGUID(WICSourceFormatguid, WICTargetFormatguid)) {
			ComPtr<IWICFormatConverter> pIWICFormatConverter;
			RETURN_IF_FAILED(pIWICImageFactory->CreateFormatConverter(&pIWICFormatConverter));
			RETURN_IF_FAILED(pIWICFormatConverter->Initialize(pIWICBitmapFrameDecode.Get()
				, WICTargetFormatguid
				, WICBitmapDitherTypeNone
				, nullptr
				, 0.0f
				, WICBitmapPaletteTypeCustom
			));
			RETURN_IF_FAILED(pIWICFormatConverter.As(&pIWICBitmapSource));
		}
		else
			RETURN_IF_FAILED(pIWICBitmapFrameDecode.As(&pIWICBitmapSource));
	}
	return S_OK;
}

//Image
UINT										nImageWidth = 0;
UINT										nImageHeight = 0;
DXGI_FORMAT									enDXGIImageFormat = {};
UINT										nImageBPP = 0;
UINT										nImageRowByteSize = 0;
UINT										nImageByteSize = 0;

DXGI_FORMAT Get_DXGIFormat_From_WICFormat(const WICPixelFormatGUID& WICTargetFormatguid) {
	if (WICTargetFormatguid == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (WICTargetFormatguid == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (WICTargetFormatguid == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (WICTargetFormatguid == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (WICTargetFormatguid == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (WICTargetFormatguid == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}
INT Get_DXGIFormat_BitsPerPixel(DXGI_FORMAT& dxgiFormat) {
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return 0;
}
HRESULT	Get_Image_Info() {
	RETURN_IF_FAILED(pIWICBitmapSource->GetSize(&nImageWidth, &nImageHeight));
	enDXGIImageFormat = Get_DXGIFormat_From_WICFormat(WICTargetFormatguid);
	if (enDXGIImageFormat == DXGI_FORMAT_UNKNOWN)
		return E_FAIL;
	//BPP or Function
	{
		ComPtr<IWICComponentInfo> pIWICCOMInfo;
		RETURN_IF_FAILED(pIWICImageFactory->CreateComponentInfo(WICTargetFormatguid, &pIWICCOMInfo));
		WICComponentType enType;
		RETURN_IF_FAILED(pIWICCOMInfo->GetComponentType(&enType));
		if (enType != WICPixelFormat)
			return E_FAIL;
		ComPtr<IWICPixelFormatInfo> pIWICPixelFormatInfo;
		RETURN_IF_FAILED(pIWICCOMInfo.As(&pIWICPixelFormatInfo));
		RETURN_IF_FAILED(pIWICPixelFormatInfo->GetBitsPerPixel(&nImageBPP));
	}
	nImageRowByteSize = UINT(floor((UINT64(nImageWidth) * UINT64(nImageBPP)) / 8.0));//l(UINT64(nImageWidth) * UINT64(nImageBPP) + 7u) / 8u;//floor mul
	nImageByteSize = nImageRowByteSize * nImageHeight;
	return S_OK;
}

HRESULT Load_Image() {
	RETURN_IF_FAILED(Create_WIC_Image_Factory());
	RETURN_IF_FAILED(Create_WIC_Bitmap_Frame_Decode());
	RETURN_IF_FAILED(Get_WIC_Bitmap_Source());
	RETURN_IF_FAILED(Get_Image_Info());
	return S_OK;
}

//D3D12

//Graphics
LONG										LClientWidth = 800;
LONG										LClientHeight = 600;
D3D12_VIEWPORT								stViewPort = { 0.0f, 0.0f, static_cast<float>(LClientWidth), static_cast<float>(LClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
D3D12_RECT									stScissorRect = { 0, 0, static_cast<LONG>(LClientWidth), static_cast<LONG>(LClientHeight) };

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

//CBVSRV Heap
ComPtr<ID3D12DescriptorHeap>				pID3D12CBVSRVHeap;
D3D12_DESCRIPTOR_HEAP_DESC					stD3D12CBVSRVHeapDesc = {};
UINT										nCBVSRVHandleIncrementSize = 0;
const UINT									nCBVSRVNum = 2;

//Sample Heap
ComPtr<ID3D12DescriptorHeap>				pID3D12SamplerHeap;
D3D12_DESCRIPTOR_HEAP_DESC					stD3D12SamplerHeapDesc = {};
UINT										nSamplerHandleIncrementSize = 0;
const UINT									nSamplerNum = 5;

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

//SRV
ComPtr<ID3D12Resource>						pID3D12TextureResource = {};
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPUSRVHandle = {};
D3D12_RESOURCE_DESC							stD3D12TextureResourceDesc = {};
ComPtr<ID3D12Heap>							pID3D12TextureHeap;
D3D12_HEAP_DESC								stD3D12TextureHeapDesc = {};
D3D12_HEAP_PROPERTIES						stD3D12TextureHeapProperties = {};
D3D12_SHADER_RESOURCE_VIEW_DESC				stD3D12ShaderResourceView = {};

//Sample
D3D12_SAMPLER_DESC							stD3D12SamplerDescs[nSamplerNum] = {};
D3D12_CPU_DESCRIPTOR_HANDLE					stD3D12CPUSamplerHandle = {};
UINT										nCurrentSamplerNO = 0;

//Command Allocator
ComPtr<ID3D12CommandAllocator>				pID3D12CommandAllocator;

//Root Signature
ComPtr<ID3D12RootSignature>					pID3D12RootSignature;
D3D12_VERSIONED_ROOT_SIGNATURE_DESC			stD3D12VersionedRootSignatureDesc = {};
D3D12_ROOT_SIGNATURE_DESC1					stRootSignatureDesc1 = {};


//Compile Shadaer
TSTRING										strShaderFileName = _T("Shaders_Sample.hlsl");
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

//UpLoad Texture
ComPtr<ID3D12Resource>						pID3D12UploadeTextureResource;
D3D12_RESOURCE_DESC							stD3D12UploadTextureResourceDesc = {};
ComPtr<ID3D12Heap>							pID3D12UploadTextureHeap;
D3D12_HEAP_DESC								stD3D12UploadTextureHeap = {};
D3D12_HEAP_PROPERTIES						stD3D12UploadTextureHeapProperties = {};

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
const float fBoxSize = 3.0f;
const float fTCMax = 3.0f;
struct VERTEX {
	XMFLOAT3 m_vPos;		//Position
	XMFLOAT2 m_vTex;		//Texcoord
	XMFLOAT3 m_vNor;		//Normal
};
VERTEX TriangleVertices[] = {
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f, -1.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, 0.0f, -1.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f, -1.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f,  1.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f }},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
};
DWORD TriangleIndexs[] = {
			0,1,2,
			3,4,5,

			6,7,8,
			9,10,11,

			12,13,14,
			15,16,17,

			18,19,20,
			21,22,23,

			24,25,26,
			27,28,29,

			30,31,32,
			33,34,35,
};
UINT										nVertexBufferByteSize = sizeof(TriangleVertices);
UINT										nIndexBufferByteSize = sizeof(TriangleIndexs);

//Const Value
struct MVPBuffer {
	DirectX::XMFLOAT4X4 m_MVP;
};
UINT64										nMVPBufferByteSize = GRS_UPPER(sizeof(MVPBuffer), 256);
VOID*										pCBVData = nullptr;

//Clear
float color[4];
bool isRAdd = true;
bool isGAdd = true;
bool isBAdd = true;

//time
UINT64										n64FrameStatrTime = 0;
UINT64										n64FrameCurrentTime = 0;
double										fPalstance = 10.0f * XM_PI / 180.0f;	//W£ºrod/s
double										dModelRotationYAngle = 0;

LRESULT	CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

HRESULT	Create_D3D12_CBVSRV_Heap() {
	//Desc
	{
		stD3D12CBVSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		stD3D12CBVSRVHeapDesc.NumDescriptors = nCBVSRVNum;
		stD3D12CBVSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		stD3D12CBVSRVHeapDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12CBVSRVHeapDesc, IID_PPV_ARGS(&pID3D12CBVSRVHeap)));
	nCBVSRVHandleIncrementSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pID3D12CBVSRVHeap->SetName(_T("CBVSRV"));
	return S_OK;
}

HRESULT Create_D3D12_Sampler_Heap() {
	//Heap Desc
	{
		stD3D12SamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		stD3D12SamplerHeapDesc.NumDescriptors = nSamplerNum;
		stD3D12SamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		stD3D12SamplerHeapDesc.NodeMask = 0;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stD3D12SamplerHeapDesc, IID_PPV_ARGS(&pID3D12SamplerHeap)));
	nSamplerHandleIncrementSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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
		stD3D12ConstantBufferResourceDesc.Width = nMVPBufferByteSize;
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
		D3D12_RANGE stReadRange = { 0,0 };
		THROW_IF_FAILED(pID3D12ConstantBufferResource->Map(0, &stReadRange, &pCBVData));
	}
	//View Desc
	{
		stD3D12ConstantBufferViewDesc.BufferLocation = pID3D12ConstantBufferResource->GetGPUVirtualAddress();
		stD3D12ConstantBufferViewDesc.SizeInBytes = static_cast<UINT>(nMVPBufferByteSize);
	}
	//Create View
	stD3D12CPUCBVHandle = pID3D12CBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
	pID3D12Device4->CreateConstantBufferView(&stD3D12ConstantBufferViewDesc, stD3D12CPUCBVHandle);
	return S_OK;
}

HRESULT Create_D3D12_SRV() {
	//Properties
	{
		stD3D12TextureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		stD3D12TextureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stD3D12TextureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stD3D12TextureHeapProperties.CreationNodeMask = 0;
		stD3D12TextureHeapProperties.VisibleNodeMask = 0;
	}
	//Default Heap
	{
		stD3D12TextureHeapDesc.SizeInBytes = GRS_UPPER(2 * nImageRowByteSize * nImageHeight, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		stD3D12TextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stD3D12TextureHeapDesc.Properties = stD3D12TextureHeapProperties;
		stD3D12TextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;
	}
	RETURN_IF_FAILED(pID3D12Device4->CreateHeap(&stD3D12TextureHeapDesc, IID_PPV_ARGS(&pID3D12TextureHeap)));
	//Resource
	{
		stD3D12TextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stD3D12TextureResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stD3D12TextureResourceDesc.Width = nImageWidth;
		stD3D12TextureResourceDesc.Height = nImageHeight;
		stD3D12TextureResourceDesc.DepthOrArraySize = 1;
		stD3D12TextureResourceDesc.MipLevels = 1;
		stD3D12TextureResourceDesc.Format = enDXGIImageFormat;
		stD3D12TextureResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
		stD3D12TextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		stD3D12TextureResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}
	//Creat Resource In Heap
	{
		/*RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12TextureHeapProperties
			, D3D12_HEAP_FLAG_NONE
			, &stD3D12TextureResourceDesc
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&pID3D12TextureResource)
		));*/

		RETURN_IF_FAILED(pID3D12Device4->CreatePlacedResource(pID3D12TextureHeap.Get()
			, 0
			, &stD3D12TextureResourceDesc
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&pID3D12TextureResource)
		));
	}
	//view
	{
		stD3D12ShaderResourceView.Format = enDXGIImageFormat;
		stD3D12ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		stD3D12ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		stD3D12ShaderResourceView.Texture2D.MipLevels = 1;
		//D3D12_TEXTURE_SRV
	}
	stD3D12CPUSRVHandle = pID3D12CBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
	stD3D12CPUSRVHandle.ptr += nCBVSRVHandleIncrementSize;
	pID3D12Device4->CreateShaderResourceView(pID3D12TextureResource.Get(), &stD3D12ShaderResourceView, stD3D12CPUSRVHandle);
	return S_OK;
}

HRESULT Create_D3D12_Samplers() {
	//Samplers Desc
	{
		stD3D12SamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stD3D12SamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12SamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12SamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stD3D12SamplerDescs[0].MipLODBias = 0.0f;
		stD3D12SamplerDescs[0].MaxAnisotropy = 1;
		stD3D12SamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12SamplerDescs[0].BorderColor[0] = 1.0f;
		stD3D12SamplerDescs[0].BorderColor[1] = 0.0f;
		stD3D12SamplerDescs[0].BorderColor[2] = 1.0f;
		stD3D12SamplerDescs[0].BorderColor[3] = 1.0f;
		stD3D12SamplerDescs[0].MinLOD = 0.0f;
		stD3D12SamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		stD3D12SamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stD3D12SamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stD3D12SamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stD3D12SamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stD3D12SamplerDescs[1].MipLODBias = 0.0f;
		stD3D12SamplerDescs[1].MaxAnisotropy = 1;
		stD3D12SamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12SamplerDescs[1].BorderColor[0] = 1.0f;
		stD3D12SamplerDescs[1].BorderColor[1] = 0.0f;
		stD3D12SamplerDescs[1].BorderColor[2] = 1.0f;
		stD3D12SamplerDescs[1].BorderColor[3] = 1.0f;
		stD3D12SamplerDescs[1].MinLOD = 0.0f;
		stD3D12SamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		stD3D12SamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stD3D12SamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		stD3D12SamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		stD3D12SamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		stD3D12SamplerDescs[2].MipLODBias = 0.0f;
		stD3D12SamplerDescs[2].MaxAnisotropy = 1;
		stD3D12SamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12SamplerDescs[2].BorderColor[0] = 1.0f;
		stD3D12SamplerDescs[2].BorderColor[1] = 0.0f;
		stD3D12SamplerDescs[2].BorderColor[2] = 1.0f;
		stD3D12SamplerDescs[2].BorderColor[3] = 1.0f;
		stD3D12SamplerDescs[2].MinLOD = 0.0f;
		stD3D12SamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		stD3D12SamplerDescs[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stD3D12SamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		stD3D12SamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		stD3D12SamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		stD3D12SamplerDescs[3].MipLODBias = 0.0f;
		stD3D12SamplerDescs[3].MaxAnisotropy = 1;
		stD3D12SamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12SamplerDescs[3].BorderColor[0] = 1.0f;
		stD3D12SamplerDescs[3].BorderColor[1] = 0.0f;
		stD3D12SamplerDescs[3].BorderColor[2] = 1.0f;
		stD3D12SamplerDescs[3].BorderColor[3] = 1.0f;
		stD3D12SamplerDescs[3].MinLOD = 0.0f;
		stD3D12SamplerDescs[3].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		stD3D12SamplerDescs[4].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stD3D12SamplerDescs[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		stD3D12SamplerDescs[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		stD3D12SamplerDescs[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		stD3D12SamplerDescs[4].MipLODBias = 0.0f;
		stD3D12SamplerDescs[4].MaxAnisotropy = 1;
		stD3D12SamplerDescs[4].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stD3D12SamplerDescs[4].BorderColor[0] = 1.0f;
		stD3D12SamplerDescs[4].BorderColor[1] = 0.0f;
		stD3D12SamplerDescs[4].BorderColor[2] = 1.0f;
		stD3D12SamplerDescs[4].BorderColor[3] = 1.0f;
		stD3D12SamplerDescs[4].MinLOD = 0.0f;
		stD3D12SamplerDescs[4].MaxLOD = D3D12_FLOAT32_MAX;
	}
	stD3D12CPUSamplerHandle = pID3D12SamplerHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT Idx = 0; Idx < nSamplerNum; ++Idx) {
		pID3D12Device4->CreateSampler(&stD3D12SamplerDescs[Idx], stD3D12CPUSamplerHandle);
		stD3D12CPUSamplerHandle.ptr += nSamplerHandleIncrementSize;
	}
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
		//Root Parameter Struct
		D3D12_ROOT_PARAMETER1 stD3D12RootParameter1s[3] = {};
		{
			//CBV
			{
				D3D12_DESCRIPTOR_RANGE1 stD3D12CBVRange1s[1] = {};
				{
					stD3D12CBVRange1s[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					stD3D12CBVRange1s[0].NumDescriptors = 1;
					stD3D12CBVRange1s[0].BaseShaderRegister = 0;
					stD3D12CBVRange1s[0].RegisterSpace = 0;
					stD3D12CBVRange1s[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
					stD3D12CBVRange1s[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				}
				stD3D12RootParameter1s[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				stD3D12RootParameter1s[0].DescriptorTable.NumDescriptorRanges = _countof(stD3D12CBVRange1s);
				stD3D12RootParameter1s[0].DescriptorTable.pDescriptorRanges = &stD3D12CBVRange1s[0];
				stD3D12RootParameter1s[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			//SRV
			{
				D3D12_DESCRIPTOR_RANGE1 stD3D12SRVRange1s[1] = {};
				{
					stD3D12SRVRange1s[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					stD3D12SRVRange1s[0].NumDescriptors = 1;
					stD3D12SRVRange1s[0].BaseShaderRegister = 0;
					stD3D12SRVRange1s[0].RegisterSpace = 0;
					stD3D12SRVRange1s[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
					stD3D12SRVRange1s[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				}
				stD3D12RootParameter1s[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				stD3D12RootParameter1s[1].DescriptorTable.NumDescriptorRanges = _countof(stD3D12SRVRange1s);
				stD3D12RootParameter1s[1].DescriptorTable.pDescriptorRanges = &stD3D12SRVRange1s[0];
				stD3D12RootParameter1s[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			}
			//Sampler
			{
				D3D12_DESCRIPTOR_RANGE1	stD3D12SamplerRangels[1] = {};
				{
					stD3D12SamplerRangels[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					stD3D12SamplerRangels[0].NumDescriptors = 1;
					stD3D12SamplerRangels[0].BaseShaderRegister = 0;
					stD3D12SamplerRangels[0].RegisterSpace = 0;
					stD3D12SamplerRangels[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
					stD3D12SamplerRangels[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				}
				stD3D12RootParameter1s[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				stD3D12RootParameter1s[2].DescriptorTable.NumDescriptorRanges = _countof(stD3D12SamplerRangels);
				stD3D12RootParameter1s[2].DescriptorTable.pDescriptorRanges = &stD3D12SamplerRangels[0];
				stD3D12RootParameter1s[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			}
		}
		//Static Sampler Desc
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
			stD3D12StaticSamplerDesc.ShaderRegister = 0;
			stD3D12StaticSamplerDesc.RegisterSpace = 0;
			stD3D12StaticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}
		stRootSignatureDesc1.NumParameters = _countof(stD3D12RootParameter1s);
		stRootSignatureDesc1.pParameters = &stD3D12RootParameter1s[0];
		stRootSignatureDesc1.NumStaticSamplers = 0;
		stRootSignatureDesc1.pStaticSamplers = nullptr;// &stD3D12StaticSamplerDesc;
		stRootSignatureDesc1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
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
#ifdef _DEBUG
	UINT										nD3DCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT										nD3DCompileFlags = 0;
#endif // _DEBUG
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	//RETURN_IF_FAILED(pID3D12GraphicsCommandList->Close());
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

HRESULT Wait_For_Previous_Frame() {
	RETURN_IF_FAILED(pID3D12CommandQueue->Signal(pID3D12Fence.Get(), n64D3D12FenceCPUValue));
	RETURN_IF_FAILED(pID3D12Fence->SetEventOnCompletion(n64D3D12FenceCPUValue++, hFenceEvent));
	WaitForSingleObject(hFenceEvent, INFINITE);
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
	RETURN_IF_FAILED(Create_D3D12_CBVSRV_Heap());
	RETURN_IF_FAILED(Create_D3D12_Sampler_Heap());
	RETURN_IF_FAILED(Create_D3D12_RTVs());
	RETURN_IF_FAILED(Create_D3D12_DSV());
	RETURN_IF_FAILED(Create_D3D12_CBV());
	RETURN_IF_FAILED(Create_D3D12_SRV());
	RETURN_IF_FAILED(Create_D3D12_Samplers());
	RETURN_IF_FAILED(Create_D3D12_Command_Allocator());
	return S_OK;
}

HRESULT Load_Texture_Resource() {
	//Texture Resource Info
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTextureLayout = {};
	UINT nTextureRowNum = 0;
	UINT64 n64TextureRowByteSize = 0;
	UINT64 n64TextureByteSize = 0;
	{
		pID3D12Device4->GetCopyableFootprints(&stD3D12TextureResourceDesc
			, 0
			, 1
			, 0
			, &stTextureLayout
			, &nTextureRowNum
			, &n64TextureRowByteSize
			, &n64TextureByteSize
		);
	}
	//Texture Upload Resource
	{
		//Upload Texture Heap Properties
		{
			stD3D12UploadTextureHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			stD3D12UploadTextureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stD3D12UploadTextureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stD3D12UploadTextureHeapProperties.CreationNodeMask = 0;
			stD3D12UploadTextureHeapProperties.VisibleNodeMask = 0;
		}
		//Upload Texture Heap
		{
			stD3D12UploadTextureHeap.SizeInBytes = GRS_UPPER(2 * nImageRowByteSize * nImageHeight, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			stD3D12UploadTextureHeap.Properties = stD3D12UploadTextureHeapProperties;
			stD3D12UploadTextureHeap.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stD3D12UploadTextureHeap.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		}
		RETURN_IF_FAILED(pID3D12Device4->CreateHeap(&stD3D12UploadTextureHeap, IID_PPV_ARGS(&pID3D12UploadTextureHeap)));
		//Upload Texture Resource Desc
		{
			stD3D12UploadTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stD3D12UploadTextureResourceDesc.Alignment = 0;
			stD3D12UploadTextureResourceDesc.Width = n64TextureByteSize;
			stD3D12UploadTextureResourceDesc.Height = 1;
			stD3D12UploadTextureResourceDesc.DepthOrArraySize = 1;
			stD3D12UploadTextureResourceDesc.MipLevels = 1;
			stD3D12UploadTextureResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			stD3D12UploadTextureResourceDesc.SampleDesc = stDXGISwapChainSampleDesc;
			stD3D12UploadTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stD3D12UploadTextureResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		}
		//Create Upload Resource In Heap 
		{
			/*RETURN_IF_FAILED(pID3D12Device4->CreateCommittedResource(&stD3D12UploadTextureHeapProperties
				, D3D12_HEAP_FLAG_NONE
				, &stD3D12UploadTextureResourceDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pID3D12UploadeTextureResource)
			));*/
			RETURN_IF_FAILED(pID3D12Device4->CreatePlacedResource(pID3D12UploadTextureHeap.Get()
				, 0
				, &stD3D12UploadTextureResourceDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pID3D12UploadeTextureResource)
			));
		}
	}
	//Load Image To Upload Resource
	{
		//Image data
		LPVOID pImageData = ::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n64TextureByteSize);//TextureByteSize=floor(ImageByteSize)
		if (pImageData == nullptr)
			return E_FAIL;
		RETURN_IF_FAILED(pIWICBitmapSource->CopyPixels(nullptr, nImageRowByteSize, nImageByteSize, reinterpret_cast<PBYTE>(pImageData)));
		//Copy To BUffer
		PBYTE pBufferData = nullptr;
		RETURN_IF_FAILED(pID3D12UploadeTextureResource->Map(0, nullptr, reinterpret_cast<PVOID*>(&pBufferData)));
		for (UINT y = 0; y < nTextureRowNum; ++y)
			memcpy(pBufferData + static_cast<SIZE_T>(stTextureLayout.Footprint.RowPitch) * y
				, reinterpret_cast<PBYTE>(pImageData) + static_cast<SIZE_T>(n64TextureRowByteSize) * y
				, static_cast<SIZE_T>(n64TextureRowByteSize)
			);
		pID3D12UploadeTextureResource->Unmap(0, nullptr);
		::HeapFree(::GetProcessHeap(), 0, pImageData);
	}
	//Set Load Command
	D3D12_TEXTURE_COPY_LOCATION	stDstLoaction = {};
	D3D12_TEXTURE_COPY_LOCATION	stSrcLoaction = {};
	{
		{
			//Dst
			stDstLoaction.pResource = pID3D12TextureResource.Get();
			stDstLoaction.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			stDstLoaction.SubresourceIndex = 0;
			//SRC
			stSrcLoaction.pResource = pID3D12UploadeTextureResource.Get();
			stSrcLoaction.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			stSrcLoaction.PlacedFootprint = stTextureLayout;
		}
		pID3D12GraphicsCommandList->CopyTextureRegion(&stDstLoaction, 0, 0, 0, &stSrcLoaction, nullptr);
	}
	//Execute Command
	{
		D3D12_RESOURCE_BARRIER stResBar = {};
		stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stResBar.Transition.pResource = pID3D12TextureResource.Get();
		stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		stResBar.Transition.StateAfter = /*D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |*/ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		pID3D12GraphicsCommandList->ResourceBarrier(1, &stResBar);
		RETURN_IF_FAILED(pID3D12GraphicsCommandList->Close());

		ID3D12CommandList* ppCommandLists[] = { pID3D12GraphicsCommandList.Get() };
		pID3D12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		RETURN_IF_FAILED(Wait_For_Previous_Frame());
	}
	return S_OK;
}

HRESULT Load_Asset() {
	RETURN_IF_FAILED(Create_D3D12_Root_Signature());
	RETURN_IF_FAILED(Compile_D3D_Shaders());
	RETURN_IF_FAILED(Set_D3D12_Input_Layout());
	RETURN_IF_FAILED(Create_D3D12_Pipeline_State());
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List());
	RETURN_IF_FAILED(Create_D3D12_Fence());
	RETURN_IF_FAILED(Create_D3D12_Fence_Event());
	RETURN_IF_FAILED(Load_Texture_Resource());
	RETURN_IF_FAILED(Load_D3D12_Vertex_Buffer());
	RETURN_IF_FAILED(Load_D3D12_Index_Buffer());
	RETURN_IF_FAILED(Create_D3D12_Resource_Barrier());
	return S_OK;
}

HRESULT Populate_CommandList() {
	RETURN_IF_FAILED(pID3D12CommandAllocator->Reset());
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Reset(pID3D12CommandAllocator.Get(), pID3D12PipelineState.Get()));

	//Set State
	pID3D12GraphicsCommandList->SetPipelineState(pID3D12PipelineState.Get());

	//Set Root Signature
	pID3D12GraphicsCommandList->SetGraphicsRootSignature(pID3D12RootSignature.Get());

	//Set Discriptor Heap
	ID3D12DescriptorHeap* ppDiscriptorHeaps[] = { pID3D12CBVSRVHeap.Get(),pID3D12SamplerHeap.Get() };
	pID3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppDiscriptorHeaps), ppDiscriptorHeaps);

	//0 CBV
	D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUCBVSRVHandle = pID3D12CBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
	pID3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(0, stD3D12GPUCBVSRVHandle);

	//1 RSV
	stD3D12GPUCBVSRVHandle.ptr += nCBVSRVHandleIncrementSize;
	pID3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(1, stD3D12GPUCBVSRVHandle);

	//2 Sampler
	D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUSamplerHandle = pID3D12SamplerHeap->GetGPUDescriptorHandleForHeapStart();
	stD3D12GPUSamplerHandle.ptr += static_cast<UINT64>(nCurrentSamplerNO) * nSamplerHandleIncrementSize;
	pID3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(2, stD3D12GPUSamplerHandle);

	//Set View
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

	//Clean
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
	pID3D12GraphicsCommandList->DrawIndexedInstanced(_countof(TriangleIndexs), 1, 0, 0, 0);

	//End
	stD3D12EndResourceBarrier.Transition.pResource = pID3D12RenderTargetsResource[nFrameIndex].Get();
	pID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12EndResourceBarrier);

	//Close
	RETURN_IF_FAILED(pID3D12GraphicsCommandList->Close());
	return S_OK;
}

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

	n64FrameCurrentTime = ::GetTickCount64();
	dModelRotationYAngle += ((n64FrameCurrentTime - n64FrameStatrTime) / 1000.0f) * fPalstance;
	if (dModelRotationYAngle > XM_2PI)
		dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);

    XMMATRIX m = XMMatrixRotationY(static_cast<float>(dModelRotationYAngle));

	XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(Pos, At, Up);

	XMMATRIX p = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(LClientWidth) / LClientHeight, 1.0f, 1000.0f);

	XMMATRIX m_MVP = m * v * p;

	MVPBuffer objConstants;
	XMStoreFloat4x4(&objConstants.m_MVP, XMMatrixTranspose(m_MVP));
	memcpy(pCBVData, &objConstants, sizeof(objConstants));
}

void OnRender() {
	try {
		THROW_IF_FAILED(Populate_CommandList());
		ID3D12CommandList* ppCommandLists[] = { pID3D12GraphicsCommandList.Get() };
		pID3D12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		THROW_IF_FAILED(pIDXGISwapChain3->Present(1, 0));
		THROW_IF_FAILED(Wait_For_Previous_Frame());
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
	case WM_KEYUP: {
		if (VK_SPACE == (wParam & 0xFF)) {
			++nCurrentSamplerNO;
			nCurrentSamplerNO %= nSamplerNum;
		}
	}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	try {
		THROW_IF_FAILED(Get_Path());
		THROW_IF_FAILED(Window_Initialize(hInstance, nCmdShow));
		RETURN_IF_FAILED(Load_Image());
		THROW_IF_FAILED(Load_Pipeline());
		THROW_IF_FAILED(Load_Asset());
		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);
		n64FrameStatrTime = ::GetTickCount64();
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