#define WIN32_LEAN_AND_MEAN
#include<windows.h>

#include<wrl.h>
using Microsoft::WRL::ComPtr;

#include<tchar.h>

#include<string>
#include<fstream>
#include<utility>
#include<vector>
#include<memory>

using std::string;
using std::wstring;
#ifdef UNICODE
typedef wstring TSTRING;
#else 
typedef  string TSTRING;
#endif // UNICODE

using std::pair;
using std::vector;

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

_Check_return_ HRESULT Get_CatallogPath(_Out_ TSTRING* tstrCatallogPath) {
	TCHAR path[MAX_PATH] = {};
	if (!GetModuleFileName(nullptr, path, MAX_PATH))
		RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	*tstrCatallogPath = path;
	//Get Catallog
	{
		//Erase "fimename.exe"
		tstrCatallogPath->erase(tstrCatallogPath->find_last_of(_T('\\')));
		//Erase "x64"
		tstrCatallogPath->erase(tstrCatallogPath->find_last_of(_T("\\")));
		//Erase "Debug/Release"
		tstrCatallogPath->erase(tstrCatallogPath->find_last_of(_T("\\")) + 1);
	}
	return S_OK;
}

//Window
_Check_return_ HRESULT Register_WndClassEx(_In_ const HINSTANCE hInstance, _In_ const wstring* wstrClassName, _In_ WNDPROC lpfnWndProc) {
	WNDCLASSEX stWndClassEx = {};
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
	if (!RegisterClassEx(&stWndClassEx))
		RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	return S_OK;
}

_Check_return_ HRESULT Initialize_WindowEx(_In_opt_ const HINSTANCE hInstance, _In_ const wstring* wstrClassName, _In_ const wstring* wstrWindowExName, _In_ int nClientWidth, _In_ int nClientHeight, _Outptr_ HWND* hWnd) {
	RECT rectWnd = { 0,0,nClientWidth,nClientHeight };
	if (AdjustWindowRectEx(&rectWnd, WS_OVERLAPPEDWINDOW, TRUE, WS_EX_LEFT) == TRUE)
		RETURN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
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
	if (!*hWnd)
		return E_HANDLE;
	return S_OK;
}

//WIC
WICPixelFormatGUID Get_ConvertTo_WICFormat(_In_ WICPixelFormatGUID guidFormat) {
	if (guidFormat == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (guidFormat == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (guidFormat == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (guidFormat == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (guidFormat == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (guidFormat == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (guidFormat == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (guidFormat == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (guidFormat == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (guidFormat == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (guidFormat == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (guidFormat == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (guidFormat == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (guidFormat == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (guidFormat == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (guidFormat == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif
	else return GUID_WICPixelFormatDontCare;
}

DXGI_FORMAT Get_DXGIFormat_From_WICFormat(_In_ WICPixelFormatGUID guidFormat) {
	if (guidFormat == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (guidFormat == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (guidFormat == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (guidFormat == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (guidFormat == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (guidFormat == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (guidFormat == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (guidFormat == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (guidFormat == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (guidFormat == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (guidFormat == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (guidFormat == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (guidFormat == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (guidFormat == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (guidFormat == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}

INT Get_DXGIFormat_BitsPerPixel(_In_ DXGI_FORMAT enFormat) {
	if (enFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (enFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (enFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (enFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (enFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (enFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (enFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (enFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (enFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (enFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (enFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (enFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (enFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (enFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (enFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return 0;
}

_Check_return_ HRESULT Create_WIC_Image_Factory(_COM_Outptr_ IWICImagingFactory** ppIWICImageFactory) {
	RETURN_IF_FAILED(::CoInitialize(nullptr));  //for WIC & COM
	RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(ppIWICImageFactory)));
	return S_OK;
}

_Check_return_ HRESULT Create_WIC_Bitmap_Decoder(_In_ IWICImagingFactory* pIWICImageFactory, _In_ const wstring* wstrImageFimeName, _COM_Outptr_ IWICBitmapDecoder** ppIWICBitmapDecoder) {
	RETURN_IF_FAILED(pIWICImageFactory->CreateDecoderFromFilename(wstrImageFimeName->c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, ppIWICBitmapDecoder));
	return S_OK;
}

_Check_return_ HRESULT Get_WIC_Bitmap_Source(_In_ IWICImagingFactory* pIWICImageFactory, _In_ ComPtr<IWICBitmapFrameDecode>& refcomIWICBitmapFrameDecode, _Outref_ ComPtr<IWICBitmapSource>& refcomIWICBitmapSource) {
	WICPixelFormatGUID WICSourceFormatguid = {};
	RETURN_IF_FAILED((refcomIWICBitmapFrameDecode)->GetPixelFormat(&WICSourceFormatguid));

	WICPixelFormatGUID WICTargetFormatguid = Get_ConvertTo_WICFormat(WICSourceFormatguid);
	if (WICTargetFormatguid == GUID_WICPixelFormatDontCare)
		return E_FAIL;
	//Source To Target
	{
		if (!InlineIsEqualGUID(WICSourceFormatguid, WICTargetFormatguid)) {
			ComPtr<IWICFormatConverter> pIWICFormatConverter;
			RETURN_IF_FAILED(pIWICImageFactory->CreateFormatConverter(&pIWICFormatConverter));
			RETURN_IF_FAILED(pIWICFormatConverter->Initialize(refcomIWICBitmapFrameDecode.Get()
				, WICTargetFormatguid
				, WICBitmapDitherTypeNone
				, nullptr
				, 0.0f
				, WICBitmapPaletteTypeCustom
			));
			RETURN_IF_FAILED(pIWICFormatConverter.As(&refcomIWICBitmapSource));
		}
		else
			RETURN_IF_FAILED(refcomIWICBitmapFrameDecode.As(&refcomIWICBitmapSource));
	}
	return S_OK;
}

_Check_return_ HRESULT Get_WIC_Signal_Bitmap_Source(_In_ IWICImagingFactory* pIWICImageFactory, _In_ const wstring* wstrImageFimeName, _Outref_ ComPtr<IWICBitmapSource>& refcomIWICBitmapSource) {
	IWICBitmapDecoder* pIWICBitmapDecoder = nullptr;
	RETURN_IF_FAILED(Create_WIC_Bitmap_Decoder(pIWICImageFactory, wstrImageFimeName, &pIWICBitmapDecoder));
	ComPtr<IWICBitmapFrameDecode> pIWICBitmapFrameDecode;
	RETURN_IF_FAILED(pIWICBitmapDecoder->GetFrame(0, &pIWICBitmapFrameDecode));
	RETURN_IF_FAILED(Get_WIC_Bitmap_Source(pIWICImageFactory, pIWICBitmapFrameDecode, refcomIWICBitmapSource));
	return S_OK;
}

_Check_return_ HRESULT Get_WIC_Image_Info(_In_ IWICImagingFactory* pIWICImageFactory, _In_ IWICBitmapSource* pIWICBitmapSouerce, _Out_ DXGI_FORMAT* pnDXGIFormat, _Out_ UINT* pnPixelWetdh, _Out_ UINT* pnPixelHeight, _Out_ UINT* pnRowByteSize) {
	//Width and Height
	RETURN_IF_FAILED(pIWICBitmapSouerce->GetSize(pnPixelWetdh, pnPixelHeight));

	//Format
	WICPixelFormatGUID WICFormatguid = {};
	RETURN_IF_FAILED(pIWICBitmapSouerce->GetPixelFormat(&WICFormatguid));

	*pnDXGIFormat = Get_DXGIFormat_From_WICFormat(WICFormatguid);
	if (*pnDXGIFormat == DXGI_FORMAT_UNKNOWN)
		return E_FAIL;
	//BPP
	UINT nBPP = 0;
	{
		ComPtr<IWICComponentInfo> pIWICCOMInfo;
		RETURN_IF_FAILED(pIWICImageFactory->CreateComponentInfo(WICFormatguid, &pIWICCOMInfo));
		WICComponentType enType;
		RETURN_IF_FAILED(pIWICCOMInfo->GetComponentType(&enType));
		if (enType != WICPixelFormat)
			return E_FAIL;
		ComPtr<IWICPixelFormatInfo> pIWICPixelFormatInfo;
		RETURN_IF_FAILED(pIWICCOMInfo.As(&pIWICPixelFormatInfo));
		RETURN_IF_FAILED(pIWICPixelFormatInfo->GetBitsPerPixel(&nBPP));
	}
	*pnRowByteSize = static_cast<UINT>(Floor_DIV((*pnPixelWetdh) * static_cast<UINT64>(nBPP), 8));

	return S_OK;
}

_Check_return_ HRESULT Copy_WIC_Image(_In_ IWICBitmapSource* pIWICBitmapSource, _In_ UINT nRowByteSize, _In_ UINT nTotalByteSize, _Outptr_  LPVOID* pData) {
	RETURN_IF_FAILED(pIWICBitmapSource->CopyPixels(nullptr, nRowByteSize, nTotalByteSize, reinterpret_cast<PBYTE>(*pData)));
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
	stD3D12RootParameter1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stD3D12RootParameter1.DescriptorTable.NumDescriptorRanges = nDescriptorRangeNum;
	stD3D12RootParameter1.DescriptorTable.pDescriptorRanges = pD3D12DescriptorRange1s;
	stD3D12RootParameter1.ShaderVisibility = enD3D12ShaderVisibility;
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
		stD3D12RasterizerDesc.DepthClipEnable = TRUE;
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
	stD3D12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	stD3D12ResourceDesc.Alignment = Alignment;//fill int ot system Optimize
	stD3D12ResourceDesc.Width = nWidth;
	stD3D12ResourceDesc.Height = nHeight;
	stD3D12ResourceDesc.DepthOrArraySize = 1;
	stD3D12ResourceDesc.MipLevels = 1;
	stD3D12ResourceDesc.Format = enDXGIFormat;
	stD3D12ResourceDesc.SampleDesc.Count = 1;
	stD3D12ResourceDesc.SampleDesc.Quality = 0;
	stD3D12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	stD3D12ResourceDesc.Flags = Flags;
	return stD3D12ResourceDesc;
}

D3D12_RESOURCE_DESC Fill_D3D12_Resource_Desc_Buffer(_In_ UINT64 n64TotailByteSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) {
	D3D12_RESOURCE_DESC	stD3D12ResourceDesc = {};
	stD3D12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	stD3D12ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	stD3D12ResourceDesc.Width = n64TotailByteSize;
	stD3D12ResourceDesc.Height = 1;
	stD3D12ResourceDesc.DepthOrArraySize = 1;
	stD3D12ResourceDesc.MipLevels = 1;
	stD3D12ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	stD3D12ResourceDesc.SampleDesc.Count = 1;
	stD3D12ResourceDesc.SampleDesc.Quality = 0;
	stD3D12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	stD3D12ResourceDesc.Flags = Flags;
	return stD3D12ResourceDesc;
}

D3D12_VERTEX_BUFFER_VIEW Fill_D3D12_VertexBufferView(_In_ UINT nSignlByteSize, _In_ ID3D12Resource* pID3D12VertexBufferResource) {
	D3D12_VERTEX_BUFFER_VIEW stD3D12VertexBufferView = {};
	stD3D12VertexBufferView.BufferLocation = pID3D12VertexBufferResource->GetGPUVirtualAddress();
	stD3D12VertexBufferView.StrideInBytes = nSignlByteSize;
	stD3D12VertexBufferView.SizeInBytes = static_cast<UINT>(pID3D12VertexBufferResource->GetDesc().Width);
	return stD3D12VertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Fill_D3D12_IndexBufferView(_In_ UINT nTotalByteSize, _In_ ID3D12Resource* pID3D12IndexBufferResource) {
	D3D12_INDEX_BUFFER_VIEW	stD3D12IndexBufferView = {};
	stD3D12IndexBufferView.BufferLocation = pID3D12IndexBufferResource->GetGPUVirtualAddress();
	stD3D12IndexBufferView.SizeInBytes = nTotalByteSize;
	stD3D12IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	return stD3D12IndexBufferView;
}

D3D12_RESOURCE_BARRIER Fill_D3D12_Barrier_Transition(_In_ ID3D12Resource* pID3D12Resource, _In_ D3D12_RESOURCE_STATES enD3D12ResourceStateBefore, _In_ D3D12_RESOURCE_STATES enD3D12ResourceStateAfter) {
	D3D12_RESOURCE_BARRIER stResBar = {};
	stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	stResBar.Transition.pResource = pID3D12Resource;
	stResBar.Transition.StateBefore = enD3D12ResourceStateBefore;
	stResBar.Transition.StateAfter = enD3D12ResourceStateAfter;
	stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return stResBar;
}

// D3D12 Init
_Check_return_ HRESULT Set_D3D12_Debug(_Inout_ UINT* nDXGIFactoryFlags) {
#ifdef _DEBUG
	ID3D12Debug* pID3D12Debug = nullptr;
	RETURN_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12Debug)));
	pID3D12Debug->EnableDebugLayer();
	*nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG
	return S_OK;
}

_Check_return_ HRESULT Create_DXGI_Factory(_In_opt_ UINT nDXGIFactoryFlags, _In_ const HWND hWnd, _COM_Outptr_ IDXGIFactory6** ppIDXGIFactory6) {
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

_Check_return_ HRESULT Create_DXGI_SwapChain(_In_ IDXGIFactory6* pIDXGIFactory6, _In_ UINT nWidth, _In_ UINT nHeight, _In_ UINT nBufferCount, _In_ ID3D12CommandQueue* pID3D12CommandQueue, _In_ HWND hWnd, _Outref_ ComPtr<IDXGISwapChain3>& refcomIDXGISwapChain3) {
	DXGI_SWAP_CHAIN_DESC1 stDXGISwapChainDesc1 = {};
	{
		stDXGISwapChainDesc1.Width = nWidth;
		stDXGISwapChainDesc1.Height = nHeight;
		stDXGISwapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

_Check_return_ HRESULT Create_D3D12_Graphics_Command_List(_In_ ID3D12CommandAllocator* pID3D12CommandAllocator, _In_ D3D12_COMMAND_LIST_TYPE enD3D12CommandListType, _COM_Outptr_ ID3D12GraphicsCommandList** ppID3D12GraphicsCommandList) {
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
	ID3DBlob* pID3DErrorBlob = nullptr;
	RETURN_IF_FAILED(D3D12SerializeVersionedRootSignature(&stD3D12VersionedRootSignatureDesc, &pID3DSignatureBlob, &pID3DErrorBlob));
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

_Check_return_ HRESULT Create_D3D12_Pipeline_State(_In_ ID3D12RootSignature* pID3D12RootSignature, _In_ const wstring* wstrShaderFileName, _In_ D3D12_BLEND_DESC* pstD3D12BlendDesc, _In_ D3D12_RASTERIZER_DESC* pstD3D12RasterizerDesc, _In_ D3D12_DEPTH_STENCIL_DESC* pstD3D12DepthStencilDesc, _In_ D3D12_INPUT_LAYOUT_DESC* pstD3D12InputLayoutDesc, _COM_Outptr_ ID3D12PipelineState** ppID3D12PipelineState) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC stD3D12GraphicsPipelineStateDesc = {};
	ID3DBlob* pID3DVertexBlob = nullptr;
	ID3DBlob* pID3DPixelBlob = nullptr;
	RETURN_IF_FAILED(Compile_D3D_VertexShader(wstrShaderFileName, &pID3DVertexBlob));
	RETURN_IF_FAILED(Compile_D3D_PixelShader(wstrShaderFileName, &pID3DPixelBlob));
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
		stD3D12GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		stD3D12GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
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

_Check_return_ HRESULT Create_System_Event(_Outptr_ HANDLE* phEvent) {
	*phEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (nullptr == *phEvent)
		THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	SetEvent(*phEvent);
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_RTVs(_In_ ID3D12DescriptorHeap* pID3D12RTVHeap, _In_ IDXGISwapChain3* pIDXGISwapChain3, _Out_ ComPtr<ID3D12Resource> comID3D12RenderTargetResources[]) {
	D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPURTVHandle = pID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
	ID3D12Device4* pID3D12Device4 = nullptr;
	RETURN_IF_FAILED(pID3D12RTVHeap->GetDevice(__uuidof(*pID3D12Device4), reinterpret_cast<void**>(&pID3D12Device4)));
	for (UINT Idx = 0; Idx < pID3D12RTVHeap->GetDesc().NumDescriptors; ++Idx) {
		RETURN_IF_FAILED(pIDXGISwapChain3->GetBuffer(Idx, IID_PPV_ARGS(&comID3D12RenderTargetResources[Idx])));
		pID3D12Device4->CreateRenderTargetView(comID3D12RenderTargetResources[Idx].Get(), nullptr, stD3D12CPURTVHandle);
		stD3D12CPURTVHandle.ptr += pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	pID3D12Device4->Release();
	return S_OK;
}

_Check_return_ HRESULT Create_D3D12_DSVs(_In_ ID3D12DescriptorHeap* pID3D12DSVHeap, _In_ UINT64 nWidth, _In_ UINT nHeight, _Out_ ComPtr<ID3D12Resource> comID3D12DepthStencilResources[]) {
	D3D12_HEAP_PROPERTIES stD3D12HeapProperties = Fill_D3D12_Heap_Properties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC	stD3D12DepthStencilResourceDesc = Fill_D3D12_Resource_Desc_Tex2D(DXGI_FORMAT_D32_FLOAT, nWidth, nHeight, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE stD3D12DepthStencilClearValue = {};
	{
		stD3D12DepthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		stD3D12DepthStencilClearValue.DepthStencil.Depth = 1.0f;
		stD3D12DepthStencilClearValue.DepthStencil.Stencil = 0;
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC stD3D12DepthStencilViewDesc = {};
	{
		stD3D12DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
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
			break;
		}
		case D3D12_SRV_DIMENSION_TEXTURECUBE: {
			stD3D12SRVDesc.TextureCube.MipLevels = stD3D12ResourceDesc.MipLevels;
			break;
		}
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

_Check_return_ HRESULT Wait_For_Previous_Frame(_In_ ID3D12CommandQueue* pID3D12CommandQueue, _In_ ID3D12Fence* pID3D12Fence, _In_ UINT64* pnCPUFenceValue, _In_ HANDLE hFenceEvent) {
	RETURN_IF_FAILED(pID3D12CommandQueue->Signal(pID3D12Fence, *pnCPUFenceValue));
	RETURN_IF_FAILED(pID3D12Fence->SetEventOnCompletion((*pnCPUFenceValue)++, hFenceEvent));
	WaitForSingleObject(hFenceEvent, INFINITE);
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

//WIC
_Check_return_ HRESULT Create_WICImage_Placed_Resource(_In_ ID3D12Device4* pID3D12Device4, _In_ IWICImagingFactory* pIWICImageFactory, _In_ const wstring* pwstrImageFileName, _In_ ID3D12Heap* pID3D12Heap, _In_ UINT64 pnHeapOffset, _COM_Outptr_ ID3D12Resource** ppID3D12Resource, _Outref_ vector<D3D12_SUBRESOURCE_DATA>& refD3D12SubResourcDatas) {
	ComPtr<IWICBitmapSource> pIWICBitmapSource;
	RETURN_IF_FAILED(Get_WIC_Signal_Bitmap_Source(pIWICImageFactory, pwstrImageFileName, pIWICBitmapSource));

	DXGI_FORMAT enDXGIFormat = DXGI_FORMAT_UNKNOWN;
	UINT nWidth = 0;
	UINT nHeight = 0;
	UINT nRowByteSize = 0;
	RETURN_IF_FAILED(Get_WIC_Image_Info(pIWICImageFactory, pIWICBitmapSource.Get(), &enDXGIFormat, &nWidth, &nHeight, &nRowByteSize));

	D3D12_RESOURCE_DESC stD3D12ResourceDesc = Fill_D3D12_Resource_Desc_Tex2D(enDXGIFormat, nWidth, nHeight);
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource(&stD3D12ResourceDesc, pID3D12Heap, pnHeapOffset, D3D12_RESOURCE_STATE_COPY_DEST, ppID3D12Resource));

	UINT nTotalByteSize = nRowByteSize * nHeight;
	LPVOID pData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, nTotalByteSize);
	Copy_WIC_Image(pIWICBitmapSource.Get(), nRowByteSize, nTotalByteSize, &pData);

	refD3D12SubResourcDatas.emplace_back(D3D12_SUBRESOURCE_DATA{ pData, nRowByteSize, nTotalByteSize });

	return S_OK;
}



//Def
const UINT									mnSamplerDescNum = 5;
D3D12_SAMPLER_DESC							mstD3D12SamplerDescs[mnSamplerDescNum] = {};
void Def_D3D12_Sampler_Descs() {
	//Samplers Desc
	{
		mstD3D12SamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		mstD3D12SamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		mstD3D12SamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		mstD3D12SamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		mstD3D12SamplerDescs[0].MipLODBias = 0.0f;
		mstD3D12SamplerDescs[0].MaxAnisotropy = 1;
		mstD3D12SamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mstD3D12SamplerDescs[0].BorderColor[0] = 1.0f;
		mstD3D12SamplerDescs[0].BorderColor[1] = 0.0f;
		mstD3D12SamplerDescs[0].BorderColor[2] = 1.0f;
		mstD3D12SamplerDescs[0].BorderColor[3] = 1.0f;
		mstD3D12SamplerDescs[0].MinLOD = 0.0f;
		mstD3D12SamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		mstD3D12SamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		mstD3D12SamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		mstD3D12SamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		mstD3D12SamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		mstD3D12SamplerDescs[1].MipLODBias = 0.0f;
		mstD3D12SamplerDescs[1].MaxAnisotropy = 1;
		mstD3D12SamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mstD3D12SamplerDescs[1].BorderColor[0] = 1.0f;
		mstD3D12SamplerDescs[1].BorderColor[1] = 0.0f;
		mstD3D12SamplerDescs[1].BorderColor[2] = 1.0f;
		mstD3D12SamplerDescs[1].BorderColor[3] = 1.0f;
		mstD3D12SamplerDescs[1].MinLOD = 0.0f;
		mstD3D12SamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		mstD3D12SamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		mstD3D12SamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		mstD3D12SamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		mstD3D12SamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		mstD3D12SamplerDescs[2].MipLODBias = 0.0f;
		mstD3D12SamplerDescs[2].MaxAnisotropy = 1;
		mstD3D12SamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mstD3D12SamplerDescs[2].BorderColor[0] = 1.0f;
		mstD3D12SamplerDescs[2].BorderColor[1] = 0.0f;
		mstD3D12SamplerDescs[2].BorderColor[2] = 1.0f;
		mstD3D12SamplerDescs[2].BorderColor[3] = 1.0f;
		mstD3D12SamplerDescs[2].MinLOD = 0.0f;
		mstD3D12SamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		mstD3D12SamplerDescs[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		mstD3D12SamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		mstD3D12SamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		mstD3D12SamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		mstD3D12SamplerDescs[3].MipLODBias = 0.0f;
		mstD3D12SamplerDescs[3].MaxAnisotropy = 1;
		mstD3D12SamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mstD3D12SamplerDescs[3].BorderColor[0] = 1.0f;
		mstD3D12SamplerDescs[3].BorderColor[1] = 0.0f;
		mstD3D12SamplerDescs[3].BorderColor[2] = 1.0f;
		mstD3D12SamplerDescs[3].BorderColor[3] = 1.0f;
		mstD3D12SamplerDescs[3].MinLOD = 0.0f;
		mstD3D12SamplerDescs[3].MaxLOD = D3D12_FLOAT32_MAX;
	}
	{
		mstD3D12SamplerDescs[4].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		mstD3D12SamplerDescs[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		mstD3D12SamplerDescs[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		mstD3D12SamplerDescs[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		mstD3D12SamplerDescs[4].MipLODBias = 0.0f;
		mstD3D12SamplerDescs[4].MaxAnisotropy = 1;
		mstD3D12SamplerDescs[4].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mstD3D12SamplerDescs[4].BorderColor[0] = 1.0f;
		mstD3D12SamplerDescs[4].BorderColor[1] = 0.0f;
		mstD3D12SamplerDescs[4].BorderColor[2] = 1.0f;
		mstD3D12SamplerDescs[4].BorderColor[3] = 1.0f;
		mstD3D12SamplerDescs[4].MinLOD = 0.0f;
		mstD3D12SamplerDescs[4].MaxLOD = D3D12_FLOAT32_MAX;
	}
}

//Path
TSTRING										g_tstrCatallogPath = {};

//Window
const wstring								g_wstrWndClassName = L"Class Name";
const wstring								g_wstrWindowExName = L"Window Name";
const UINT									g_nClientWidth = 800;
const UINT									g_nClientHeight = 600;
HWND										g_hWnd = nullptr;

//Init
ComPtr<ID3D12Device4>						mpID3D12Device4;

ComPtr<ID3D12CommandQueue>					mpID3D12DirectCommandQueue;

ComPtr<IDXGISwapChain3>						mpIDXGISwapChain3;
const UINT									mnBackBufferCount = 3;

ComPtr<ID3D12DescriptorHeap>				mpID3D12RTVHeap;
const UINT									mnRTVNum = mnBackBufferCount;

ComPtr<ID3D12DescriptorHeap>				mpID3D12DSVHeap;
const UINT									mnDSVNum = 1;

ComPtr<ID3D12Resource>						mpID3D12RenderTargetResources[mnRTVNum];

ComPtr<ID3D12Resource>						mpID3D12DepthStencilResources[mnDSVNum];


enum  Object { SkyBox = 0, Earth = 1 };
const UINT									ObjectNum = 2;

ComPtr<ID3D12DescriptorHeap>				mpID3D12CSUVHeap[ObjectNum];
const UINT									mnCSUVNum[ObjectNum] = { 2,2 };

ComPtr<ID3D12DescriptorHeap>				mpID3D12SamplerHeap[ObjectNum];
const UINT									mnSamplerNum[ObjectNum] = { 1,mnSamplerDescNum };

//Command List

//Direct
ComPtr<ID3D12CommandAllocator>				mpID3D12DirectCommandAllocator;
ComPtr<ID3D12GraphicsCommandList>			mpID3D12GraphicsCommandList;

//Bundle
ComPtr<ID3D12CommandAllocator>				mpID3D12BundleCommandAllocator[ObjectNum];
ComPtr<ID3D12GraphicsCommandList>			mpID3D12BundleCommandList[ObjectNum];

//Assert
const wstring								mstrShaderFileName[ObjectNum] = { L"SkyBox.hlsl" ,L"TextureCube.hlsl" };

ComPtr<ID3D12RootSignature>					mpID3D12RootSignature;

ComPtr<ID3D12PipelineState>					mpID3D12PipelineState[ObjectNum];

ComPtr<ID3D12Fence>							mpID3D12Fence;
UINT64										mnFenceCPUValue = 0;
HANDLE										mhFenceEvent = nullptr;

//Time
HANDLE										mhTimer = nullptr;

_Check_return_ HRESULT Set_Timer(_In_ INT64 time, _Outptr_ HANDLE* phTimer) {
	*phTimer = CreateWaitableTimer(nullptr, FALSE, nullptr);
	LARGE_INTEGER liDueTime = {};
	liDueTime.QuadPart = -100000i64;
	if (FALSE == SetWaitableTimer(*phTimer, &liDueTime, 100000, nullptr, nullptr, FALSE))
		return E_FAIL;
	return S_OK;
}

//WIC
ComPtr<IWICImagingFactory>					mpIWICImageFactory;


const wstring								mstrImageFileName[ObjectNum] = { L"cube.dds",L"anbi.jpg" };

//CBV
struct MVPBuffer {
	DirectX::XMFLOAT4X4 m_MVP;
};
ComPtr<ID3D12Resource>						mpID3D12ConstantBufferResource[ObjectNum];
const UINT64								mnConstantBufferByteSize = GRS_ALIGNMENT_UPPER(sizeof(MVPBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
LPVOID										mpConstBufferData[ObjectNum] = {};

//Heap
ComPtr<ID3D12Heap>							mpID3D12ConstBufferHeap;
const UINT64								mnConstBufferHeapSize = 2 * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
UINT64										mnConstantBufferHeapOffset = 0;

//SRV
ComPtr<ID3D12Resource>						mpID3D12ShaderResource[ObjectNum];
UINT64										mnShaderResourceByteSize[ObjectNum] = {};

ComPtr<ID3D12Heap>							mpID3D12ShaderResourceHeap;
const UINT64								mnShaderResourceHeapSize = 70 * 1024 * 1024;
UINT64										mnShaderResourceHeapOffset = 0;

//Upload Heap
ComPtr<ID3D12Resource>						mpID3D12UploadShaderResource[ObjectNum];

ComPtr<ID3D12Heap>							mpID3D12UploadHeap;
UINT64										mnUpLoadHeapSize = {};
UINT64										mnUploadHeapOffset = 0;

//Sampler Descriptor In Top
UINT										mnCurrentSamplerNO = 0;


float fHighW = -1.0f - (1.0f / (float)g_nClientWidth);
float fHighH = -1.0f - (1.0f / (float)g_nClientHeight);
float fLowW = 1.0f + (1.0f / (float)g_nClientWidth);
float fLowH = 1.0f + (1.0f / (float)g_nClientHeight);

//SkyBox Vertex
struct SKYBOX_VERTEX {
	XMFLOAT4 m_vPos;
};
const UINT									nSkyBoxVertexCount = 4;
SKYBOX_VERTEX mpSkyBoxVertexData[nSkyBoxVertexCount] = {
	XMFLOAT4(fLowW, fLowH, 1.0f, 1.0f),
	XMFLOAT4(fLowW, fHighH, 1.0f, 1.0f),
	XMFLOAT4(fHighW, fLowH, 1.0f, 1.0f),
	XMFLOAT4(fHighW, fHighH, 1.0f, 1.0f)
};
UINT mpSkyBoxIndexData[nSkyBoxVertexCount] = { 0,1,2,3 };

//Earth Vertex
struct VERTEX {
	XMFLOAT3 m_vPos;		//Position
	XMFLOAT2 m_vTex;		//Texcoord
	XMFLOAT3 m_vNor;		//Normal
};
vector<VERTEX>								mvecEarthVertexData;
vector<UINT>								mvecEarthIndexData;


//Vertex
ComPtr<ID3D12Resource>						mpID3D12VertexBufferResource[ObjectNum];
UINT										mnVertexCount[ObjectNum] = { nSkyBoxVertexCount ,0 };
UINT										mnVertexBufferByteSize[ObjectNum] = { nSkyBoxVertexCount * sizeof(SKYBOX_VERTEX),0 };
D3D12_VERTEX_BUFFER_VIEW					mstD3D12VertexBufferView[ObjectNum] = {};

ComPtr<ID3D12Heap>							mpID3D12VertexHeap;
UINT64										mnVertexHeapSize = 0;
UINT64										mnVertexHeapOffset = 0;

//Index
ComPtr<ID3D12Resource>						mpID3D12IndexBufferResource[ObjectNum];
UINT										mnIndexCount[ObjectNum] = { nSkyBoxVertexCount ,0 };
UINT										mnIndexBufferByteSize[ObjectNum] = { nSkyBoxVertexCount * sizeof(UINT),0 };
D3D12_INDEX_BUFFER_VIEW						mstD3D12IndexBufferView[ObjectNum] = {};

ComPtr<ID3D12Heap>							mpID3D12IndexHeap;
UINT64										mnIndexHeapSize = 0;
UINT64										mnIndexHeapOffset = 0;

void Import_Vertex() {
	std::ifstream fin;
	char input;

	fin.open("D:\\cg-api\\leanDirectX12\\sphere.txt");

	fin.get(input);
	while (input != ':')
		fin.get(input);

	fin >> mnVertexCount[Earth];
	mnIndexCount[Earth] = mnVertexCount[Earth];

	fin.get(input);
	while (input != ':')
		fin.get(input);
	fin.get(input);
	fin.get(input);

	mnVertexBufferByteSize[Earth] = mnVertexCount[Earth] * sizeof(VERTEX);
	mnIndexBufferByteSize[Earth] = mnIndexCount[Earth] * sizeof(UINT);

	mvecEarthVertexData.resize(mnVertexCount[Earth]);
	mvecEarthIndexData.resize(mnIndexCount[Earth]);

	for (UINT i = 0; i < mnVertexCount[Earth]; ++i) {
		fin >> mvecEarthVertexData[i].m_vPos.x >> mvecEarthVertexData[i].m_vPos.y >> mvecEarthVertexData[i].m_vPos.z;
		fin >> mvecEarthVertexData[i].m_vTex.x >> mvecEarthVertexData[i].m_vTex.y;
		fin >> mvecEarthVertexData[i].m_vNor.x >> mvecEarthVertexData[i].m_vNor.y >> mvecEarthVertexData[i].m_vNor.z;

		mvecEarthIndexData[i] = i;
	}
}


const D3D12_VIEWPORT						mstViewPort = { 0.0f, 0.0f, static_cast<float>(g_nClientWidth), static_cast<float>(g_nClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
const D3D12_RECT							mstScissorRect = { 0, 0, static_cast<LONG>(g_nClientWidth), static_cast<LONG>(g_nClientHeight) };

//time
UINT64										n64FrameStatrTime = 0;
UINT64										n64FrameCurrentTime = 0;
double										fPalstance = 10.0f * XM_PI / 180.0f;	//W£ºrod/s
double										dModelRotationYAngle = 0;

_Check_return_ HRESULT Load_Pipeline() {
	UINT nDXGIFactoryFlags = 0;
	RETURN_IF_FAILED(Set_D3D12_Debug(&nDXGIFactoryFlags));

	IDXGIFactory6* pIDXGIFactory6 = nullptr;
	RETURN_IF_FAILED(Create_DXGI_Factory(nDXGIFactoryFlags, g_hWnd, &pIDXGIFactory6));
	RETURN_IF_FAILED(pIDXGIFactory6->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER));

	IDXGIAdapter1* pIDXGIAdapter1 = nullptr;
	RETURN_IF_FAILED(Create_DXGI_Adapter(pIDXGIFactory6, &pIDXGIAdapter1));
	RETURN_IF_FAILED(Create_D3D12_Device(pIDXGIAdapter1, D3D_FEATURE_LEVEL_12_1, &mpID3D12Device4));

	RETURN_IF_FAILED(Create_D3D12_Direct_Command_Queue(mpID3D12Device4.Get(), &mpID3D12DirectCommandQueue));

	RETURN_IF_FAILED(Create_DXGI_SwapChain(pIDXGIFactory6, g_nClientWidth, g_nClientHeight, mnBackBufferCount, mpID3D12DirectCommandQueue.Get(), g_hWnd, mpIDXGISwapChain3));

	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mnRTVNum, &mpID3D12RTVHeap));
	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, mnDSVNum, &mpID3D12DSVHeap));

	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mnCSUVNum[SkyBox], &mpID3D12CSUVHeap[SkyBox]));
	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mnSamplerNum[SkyBox], &mpID3D12SamplerHeap[SkyBox]));

	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mnCSUVNum[Earth], &mpID3D12CSUVHeap[Earth]));
	RETURN_IF_FAILED(Create_D3D12_Descriptor_Heap(mpID3D12Device4.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mnSamplerNum[Earth], &mpID3D12SamplerHeap[Earth]));

	//Direct Command List
	RETURN_IF_FAILED(Create_D3D12_Command_Allocator(mpID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &mpID3D12DirectCommandAllocator));
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List(mpID3D12DirectCommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, &mpID3D12GraphicsCommandList));

	//Bound
	RETURN_IF_FAILED(Create_D3D12_Command_Allocator(mpID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_BUNDLE, &mpID3D12BundleCommandAllocator[SkyBox]));
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List(mpID3D12BundleCommandAllocator[SkyBox].Get(), D3D12_COMMAND_LIST_TYPE_BUNDLE, &mpID3D12BundleCommandList[SkyBox]));

	RETURN_IF_FAILED(Create_D3D12_Command_Allocator(mpID3D12Device4.Get(), D3D12_COMMAND_LIST_TYPE_BUNDLE, &mpID3D12BundleCommandAllocator[Earth]));
	RETURN_IF_FAILED(Create_D3D12_Graphics_Command_List(mpID3D12BundleCommandAllocator[Earth].Get(), D3D12_COMMAND_LIST_TYPE_BUNDLE, &mpID3D12BundleCommandList[Earth]));

	return S_OK;
}

_Check_return_ HRESULT Load_CBCSRVUAV_Assert() {
	//Heap
	RETURN_IF_FAILED(Create_D3D12_Heap(mpID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, mnConstBufferHeapSize, &mpID3D12ConstBufferHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

	RETURN_IF_FAILED(Create_D3D12_Heap(mpID3D12Device4.Get(), D3D12_HEAP_TYPE_DEFAULT, mnShaderResourceHeapSize, &mpID3D12ShaderResourceHeap, D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

	//CBV/SRV/UAV Resource
	D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUCSUVHandle[ObjectNum] = { mpID3D12CSUVHeap[SkyBox]->GetCPUDescriptorHandleForHeapStart(), mpID3D12CSUVHeap[Earth]->GetCPUDescriptorHandleForHeapStart() };

	//Heap Placeed Const Buffer
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnConstantBufferByteSize, mpID3D12ConstBufferHeap.Get(), mnConstantBufferHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12ConstantBufferResource[SkyBox]));
	mnConstantBufferHeapOffset = Alignment_Upper(mnConstantBufferHeapOffset + mnConstantBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnConstantBufferByteSize, mpID3D12ConstBufferHeap.Get(), mnConstantBufferHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12ConstantBufferResource[Earth]));
	mnConstantBufferHeapOffset = Alignment_Upper(mnConstantBufferHeapOffset + mnConstantBufferByteSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	D3D12_RANGE stReadRange = { 0,0 };
	//Const Buffer Data
	RETURN_IF_FAILED(mpID3D12ConstantBufferResource[SkyBox]->Map(0, &stReadRange, &mpConstBufferData[SkyBox]));

	RETURN_IF_FAILED(mpID3D12ConstantBufferResource[Earth]->Map(0, &stReadRange, &mpConstBufferData[Earth]));

	//CBVs
	RETURN_IF_FAILED(Set_D3D12_CBV(stD3D12CPUCSUVHandle[SkyBox], mpID3D12ConstantBufferResource[SkyBox].Get()));
	stD3D12CPUCSUVHandle[SkyBox].ptr += mpID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	RETURN_IF_FAILED(Set_D3D12_CBV(stD3D12CPUCSUVHandle[Earth], mpID3D12ConstantBufferResource[Earth].Get()));
	stD3D12CPUCSUVHandle[Earth].ptr += mpID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//DDSTex
	std::unique_ptr<uint8_t[]> ddsData;

	//WIC
	RETURN_IF_FAILED(Create_WIC_Image_Factory(&mpIWICImageFactory));

	//Resource
	vector<D3D12_SUBRESOURCE_DATA> vecD3D12SubResourceData[ObjectNum];

	RETURN_IF_FAILED(Create_DDSTex_Placed_Resource(mpID3D12Device4.Get(), &mstrImageFileName[SkyBox], mpID3D12ShaderResourceHeap.Get(), mnShaderResourceHeapOffset, &mpID3D12ShaderResource[SkyBox], ddsData, vecD3D12SubResourceData[SkyBox]));
	mnShaderResourceByteSize[SkyBox] = GetRequiredIntermediateSize(mpID3D12ShaderResource[SkyBox].Get(), 0, static_cast<UINT>(vecD3D12SubResourceData[SkyBox].size()));
	mnShaderResourceHeapOffset = Alignment_Upper(mnShaderResourceHeapOffset + mnShaderResourceByteSize[SkyBox], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	RETURN_IF_FAILED(Create_WICImage_Placed_Resource(mpID3D12Device4.Get(), mpIWICImageFactory.Get(), &mstrImageFileName[Earth], mpID3D12ShaderResourceHeap.Get(), mnShaderResourceHeapOffset, &mpID3D12ShaderResource[Earth], vecD3D12SubResourceData[Earth]));
	mnShaderResourceByteSize[Earth] = GetRequiredIntermediateSize(mpID3D12ShaderResource[Earth].Get(), 0, 1/* static_cast<UINT>(vecD3D12SubResourceData[Earth].size())*/);
	mnShaderResourceHeapOffset = Alignment_Upper(mnShaderResourceHeapOffset + mnShaderResourceByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);


	mnUpLoadHeapSize = Alignment_Upper(mnShaderResourceByteSize[SkyBox], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	mnUpLoadHeapSize = Alignment_Upper(mnUpLoadHeapSize + mnShaderResourceByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	RETURN_IF_FAILED(Create_D3D12_Heap(mpID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, mnUpLoadHeapSize, &mpID3D12UploadHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

	//Uplaod Shader Resource
	{
		RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnShaderResourceByteSize[SkyBox], mpID3D12UploadHeap.Get(), mnUploadHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12UploadShaderResource[SkyBox]));
		mnUploadHeapOffset = Alignment_Upper(mnUploadHeapOffset + mnShaderResourceByteSize[SkyBox], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		UpdateSubresources(mpID3D12GraphicsCommandList.Get(), mpID3D12ShaderResource[SkyBox].Get(), mpID3D12UploadShaderResource[SkyBox].Get(), 0, 0, static_cast<UINT>(vecD3D12SubResourceData[SkyBox].size()), vecD3D12SubResourceData[SkyBox].data());
		D3D12_RESOURCE_BARRIER stD3D12SkyBoxResourceBarrier = Fill_D3D12_Barrier_Transition(mpID3D12ShaderResource[SkyBox].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mpID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12SkyBoxResourceBarrier);


		RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnShaderResourceByteSize[Earth], mpID3D12UploadHeap.Get(), mnUploadHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12UploadShaderResource[Earth]));
		mnUploadHeapOffset = Alignment_Upper(mnUploadHeapOffset + mnShaderResourceByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		UpdateSubresources(mpID3D12GraphicsCommandList.Get(), mpID3D12ShaderResource[Earth].Get(), mpID3D12UploadShaderResource[Earth].Get(), 0, 0, static_cast<UINT>(vecD3D12SubResourceData[Earth].size()), vecD3D12SubResourceData[Earth].data());
		D3D12_RESOURCE_BARRIER stD3D12EarthResourceBarrier = Fill_D3D12_Barrier_Transition(mpID3D12ShaderResource[Earth].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mpID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12EarthResourceBarrier);

		//Excute Command
		RETURN_IF_FAILED(mpID3D12GraphicsCommandList->Close());
		ID3D12CommandList* list[] = { mpID3D12GraphicsCommandList.Get() };
		mpID3D12DirectCommandQueue->ExecuteCommandLists(_countof(list), list);
		RETURN_IF_FAILED(Wait_For_Previous_Frame(mpID3D12DirectCommandQueue.Get(), mpID3D12Fence.Get(), &mnFenceCPUValue, mhFenceEvent));
	}

	//SRV
	RETURN_IF_FAILED(Set_D3D12_SRV(stD3D12CPUCSUVHandle[SkyBox], mpID3D12ShaderResource[SkyBox].Get(), D3D12_SRV_DIMENSION_TEXTURECUBE));

	RETURN_IF_FAILED(Set_D3D12_SRV(stD3D12CPUCSUVHandle[Earth], mpID3D12ShaderResource[Earth].Get(), D3D12_SRV_DIMENSION_TEXTURE2D));

	::HeapFree(::GetProcessHeap(), 0, const_cast<LPVOID>(vecD3D12SubResourceData[Earth][0].pData));
	ddsData.reset(nullptr);
	return S_OK;
}

_Check_return_ HRESULT Load_Asset() {
	RETURN_IF_FAILED(Create_D3D12_Root_Signature(mpID3D12Device4.Get(), &mpID3D12RootSignature));

	//Pipeline
	{
		D3D12_BLEND_DESC stD3D12BlendDesc = Fill_D3D12_Blend_Desc();
		D3D12_RASTERIZER_DESC stD3D12RasterizerDesc = Fill_D3D12_Rasterizer_Desc();
		D3D12_DEPTH_STENCIL_DESC stD3D12DepthStencilDesc = Fill_D3D12_DepthStencil_Desc();
		D3D12_INPUT_LAYOUT_DESC stD3D12InputDesc = Fill_D3D12_Input_Layout();

		stD3D12DepthStencilDesc.DepthEnable = FALSE;
		RETURN_IF_FAILED(Create_D3D12_Pipeline_State(mpID3D12RootSignature.Get(), &mstrShaderFileName[SkyBox], &stD3D12BlendDesc, &stD3D12RasterizerDesc, &stD3D12DepthStencilDesc, &stD3D12InputDesc, &mpID3D12PipelineState[SkyBox]));

		stD3D12DepthStencilDesc.DepthEnable = TRUE;
		RETURN_IF_FAILED(Create_D3D12_Pipeline_State(mpID3D12RootSignature.Get(), &mstrShaderFileName[Earth], &stD3D12BlendDesc, &stD3D12RasterizerDesc, &stD3D12DepthStencilDesc, &stD3D12InputDesc, &mpID3D12PipelineState[Earth]));
	}

	RETURN_IF_FAILED(Create_D3D12_Fence(mpID3D12Device4.Get(), mnFenceCPUValue++, &mpID3D12Fence));
	RETURN_IF_FAILED(Create_System_Event(&mhFenceEvent));

	RETURN_IF_FAILED(Create_D3D12_RTVs(mpID3D12RTVHeap.Get(), mpIDXGISwapChain3.Get(), mpID3D12RenderTargetResources));
	RETURN_IF_FAILED(Create_D3D12_DSVs(mpID3D12DSVHeap.Get(), g_nClientWidth, g_nClientHeight, mpID3D12DepthStencilResources));

	RETURN_IF_FAILED(Load_CBCSRVUAV_Assert());

	//Sampler Descriptor
	Def_D3D12_Sampler_Descs();

	RETURN_IF_FAILED(Create_D3D12_Samplers(mstD3D12SamplerDescs, mpID3D12SamplerHeap[SkyBox].Get()));
	RETURN_IF_FAILED(Create_D3D12_Samplers(mstD3D12SamplerDescs, mpID3D12SamplerHeap[Earth].Get()));

	Import_Vertex();

	//Heap
	{
		mnVertexHeapSize = Alignment_Upper(static_cast<UINT64>(mnVertexBufferByteSize[SkyBox]), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		mnVertexHeapSize = Alignment_Upper(static_cast<UINT64>(mnVertexHeapSize + mnVertexBufferByteSize[Earth]), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		RETURN_IF_FAILED(Create_D3D12_Heap(mpID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, mnVertexHeapSize, &mpID3D12VertexHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

		mnIndexHeapSize = Alignment_Upper(static_cast<UINT64>(mnIndexBufferByteSize[SkyBox]), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		mnIndexHeapSize = Alignment_Upper(mnIndexHeapSize + mnIndexBufferByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		RETURN_IF_FAILED(Create_D3D12_Heap(mpID3D12Device4.Get(), D3D12_HEAP_TYPE_UPLOAD, mnIndexHeapSize, &mpID3D12IndexHeap, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
	}

	//Vertex Buffer Resource
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnVertexBufferByteSize[SkyBox], mpID3D12VertexHeap.Get(), mnVertexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12VertexBufferResource[SkyBox]));
	mnVertexHeapOffset = Alignment_Upper(mnVertexHeapOffset + mnVertexBufferByteSize[SkyBox], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnVertexBufferByteSize[Earth], mpID3D12VertexHeap.Get(), mnVertexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12VertexBufferResource[Earth]));
	mnVertexHeapOffset = Alignment_Upper(mnVertexHeapOffset + mnVertexBufferByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	//Vertex Buffer View
	mstD3D12VertexBufferView[SkyBox] = Fill_D3D12_VertexBufferView(sizeof(SKYBOX_VERTEX), mpID3D12VertexBufferResource[SkyBox].Get());
	mstD3D12VertexBufferView[Earth] = Fill_D3D12_VertexBufferView(sizeof(VERTEX), mpID3D12VertexBufferResource[Earth].Get());

	D3D12_RANGE stReadRange = { 0,0 };
	//Vertex Data
	LPVOID	pVertexData = nullptr;
	RETURN_IF_FAILED(mpID3D12VertexBufferResource[SkyBox]->Map(0, &stReadRange, &pVertexData));
	memcpy(pVertexData, mpSkyBoxVertexData, mnVertexBufferByteSize[SkyBox]);
	mpID3D12VertexBufferResource[SkyBox]->Unmap(0, nullptr);

	RETURN_IF_FAILED(mpID3D12VertexBufferResource[Earth]->Map(0, &stReadRange, &pVertexData));
	memcpy(pVertexData, mvecEarthVertexData.data(), mnVertexBufferByteSize[Earth]);
	mpID3D12VertexBufferResource[Earth]->Unmap(0, nullptr);

	//Index Buffer Resource
	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnIndexBufferByteSize[SkyBox], mpID3D12IndexHeap.Get(), mnIndexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12IndexBufferResource[SkyBox]));
	mnIndexHeapOffset = Alignment_Upper(mnIndexHeapOffset + mnIndexBufferByteSize[SkyBox], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	RETURN_IF_FAILED(Create_D3D12_Placed_Resource_Buffer(mnIndexBufferByteSize[Earth], mpID3D12IndexHeap.Get(), mnIndexHeapOffset, D3D12_RESOURCE_STATE_GENERIC_READ, &mpID3D12IndexBufferResource[Earth]));
	mnIndexHeapOffset = Alignment_Upper(mnIndexHeapOffset + mnIndexBufferByteSize[Earth], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	//Index Buffer View
	mstD3D12IndexBufferView[SkyBox] = Fill_D3D12_IndexBufferView(mnIndexBufferByteSize[SkyBox], mpID3D12IndexBufferResource[SkyBox].Get());
	mstD3D12IndexBufferView[Earth] = Fill_D3D12_IndexBufferView(mnIndexBufferByteSize[Earth], mpID3D12IndexBufferResource[Earth].Get());

	//Index Data
	LPVOID pIndexData = nullptr;

	RETURN_IF_FAILED(mpID3D12IndexBufferResource[SkyBox]->Map(0, &stReadRange, &pIndexData));
	memcpy(pIndexData, mpSkyBoxIndexData, mnIndexBufferByteSize[SkyBox]);
	mpID3D12IndexBufferResource[SkyBox]->Unmap(0, nullptr);

	RETURN_IF_FAILED(mpID3D12IndexBufferResource[Earth]->Map(0, &stReadRange, &pIndexData));
	memcpy(pIndexData, mvecEarthIndexData.data(), mnIndexBufferByteSize[Earth]);
	mpID3D12IndexBufferResource[Earth]->Unmap(0, nullptr);

	return S_OK;
}

_Check_return_ HRESULT Bundle_Command() {
	D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUCSUVHandle[ObjectNum] = {};
	D3D12_GPU_DESCRIPTOR_HANDLE stD3D12GPUSamplerHandle[ObjectNum] = {};

	//SkyBox
	{
		mpID3D12BundleCommandList[SkyBox]->SetGraphicsRootSignature(mpID3D12RootSignature.Get());
		mpID3D12BundleCommandList[SkyBox]->SetPipelineState(mpID3D12PipelineState[SkyBox].Get());

		ID3D12DescriptorHeap* const pID3D12SkyBoxDescriptorHeap[] = { mpID3D12CSUVHeap[SkyBox].Get(),mpID3D12SamplerHeap[SkyBox].Get() };
		mpID3D12BundleCommandList[SkyBox]->SetDescriptorHeaps(_countof(pID3D12SkyBoxDescriptorHeap), pID3D12SkyBoxDescriptorHeap);

		stD3D12GPUCSUVHandle[SkyBox] = mpID3D12CSUVHeap[SkyBox]->GetGPUDescriptorHandleForHeapStart();
		mpID3D12BundleCommandList[SkyBox]->SetGraphicsRootDescriptorTable(0, stD3D12GPUCSUVHandle[SkyBox]);

		stD3D12GPUCSUVHandle[SkyBox].ptr += mpID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mpID3D12BundleCommandList[SkyBox]->SetGraphicsRootDescriptorTable(1, stD3D12GPUCSUVHandle[SkyBox]);

		stD3D12GPUSamplerHandle[SkyBox] = mpID3D12SamplerHeap[SkyBox]->GetGPUDescriptorHandleForHeapStart();
		mpID3D12BundleCommandList[SkyBox]->SetGraphicsRootDescriptorTable(2, stD3D12GPUSamplerHandle[SkyBox]);

		mpID3D12BundleCommandList[SkyBox]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		mpID3D12BundleCommandList[SkyBox]->IASetVertexBuffers(0, 1, &mstD3D12VertexBufferView[SkyBox]);
		mpID3D12BundleCommandList[SkyBox]->IASetIndexBuffer(&mstD3D12IndexBufferView[SkyBox]);

		mpID3D12BundleCommandList[SkyBox]->DrawIndexedInstanced(mnIndexCount[SkyBox], 1, 0, 0, 0);

		RETURN_IF_FAILED(mpID3D12BundleCommandList[SkyBox]->Close());
	}

	//Earth
	{
		mpID3D12BundleCommandList[Earth]->SetGraphicsRootSignature(mpID3D12RootSignature.Get());
		mpID3D12BundleCommandList[Earth]->SetPipelineState(mpID3D12PipelineState[Earth].Get());

		ID3D12DescriptorHeap* const pID3D12EarthDescriptorHeaps[] = { mpID3D12CSUVHeap[Earth].Get(),mpID3D12SamplerHeap[Earth].Get() };
		mpID3D12BundleCommandList[Earth]->SetDescriptorHeaps(_countof(pID3D12EarthDescriptorHeaps), pID3D12EarthDescriptorHeaps);

		stD3D12GPUCSUVHandle[Earth] = mpID3D12CSUVHeap[Earth]->GetGPUDescriptorHandleForHeapStart();
		mpID3D12BundleCommandList[Earth]->SetGraphicsRootDescriptorTable(0, stD3D12GPUCSUVHandle[Earth]);

		stD3D12GPUCSUVHandle[Earth].ptr += mpID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mpID3D12BundleCommandList[Earth]->SetGraphicsRootDescriptorTable(1, stD3D12GPUCSUVHandle[Earth]);

		stD3D12GPUSamplerHandle[Earth] = mpID3D12SamplerHeap[Earth]->GetGPUDescriptorHandleForHeapStart();
		mpID3D12BundleCommandList[Earth]->SetGraphicsRootDescriptorTable(2, stD3D12GPUSamplerHandle[Earth]);

		mpID3D12BundleCommandList[Earth]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mpID3D12BundleCommandList[Earth]->IASetVertexBuffers(0, 1, &mstD3D12VertexBufferView[Earth]);
		mpID3D12BundleCommandList[Earth]->IASetIndexBuffer(&mstD3D12IndexBufferView[Earth]);

		mpID3D12BundleCommandList[Earth]->DrawIndexedInstanced(mnIndexCount[Earth], 1, 0, 0, 0);

		RETURN_IF_FAILED(mpID3D12BundleCommandList[Earth]->Close());
	}

	return S_OK;
}

_Check_return_ HRESULT Populate_CommandList() {
	RETURN_IF_FAILED(mpID3D12DirectCommandAllocator->Reset());
	RETURN_IF_FAILED(mpID3D12GraphicsCommandList->Reset(mpID3D12DirectCommandAllocator.Get(), nullptr));

	//Set View
	mpID3D12GraphicsCommandList->RSSetViewports(1, &mstViewPort);
	mpID3D12GraphicsCommandList->RSSetScissorRects(1, &mstScissorRect);

	UINT nFrameIndex = mpIDXGISwapChain3->GetCurrentBackBufferIndex();

	//Begin
	D3D12_RESOURCE_BARRIER stD3D12ResourceBarrierBegin = Fill_D3D12_Barrier_Transition(mpID3D12RenderTargetResources[nFrameIndex].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mpID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12ResourceBarrierBegin);

	//Render Target And Depen Stencil And Clear 
	{
		//Render Target
		D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPURTVHandle = mpID3D12RTVHeap->GetCPUDescriptorHandleForHeapStart();
		stD3D12CPURTVHandle.ptr += static_cast<unsigned long long>(nFrameIndex) * mpID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE stD3D12CPUDSVHandle = mpID3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
		//set
		mpID3D12GraphicsCommandList->OMSetRenderTargets(1, &stD3D12CPURTVHandle, FALSE, &stD3D12CPUDSVHandle);

		//Clear
		const float ClearColor[] = { 0.0f,0.0f,0.0f,1.0f };
		mpID3D12GraphicsCommandList->ClearRenderTargetView(stD3D12CPURTVHandle, ClearColor, 0, nullptr);
		mpID3D12GraphicsCommandList->ClearDepthStencilView(stD3D12CPUDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	//Bundle Command
	{
		ID3D12DescriptorHeap* const pID3D12SkyBoxDescriptorHeap[] = { mpID3D12CSUVHeap[SkyBox].Get(),mpID3D12SamplerHeap[SkyBox].Get() };
		mpID3D12GraphicsCommandList->SetDescriptorHeaps(_countof(pID3D12SkyBoxDescriptorHeap), pID3D12SkyBoxDescriptorHeap);
		mpID3D12GraphicsCommandList->ExecuteBundle(mpID3D12BundleCommandList[SkyBox].Get());

		ID3D12DescriptorHeap* const pID3D12EarthDescriptorHeap[] = { mpID3D12CSUVHeap[Earth].Get(),mpID3D12SamplerHeap[Earth].Get() };
		mpID3D12GraphicsCommandList->SetDescriptorHeaps(_countof(pID3D12EarthDescriptorHeap), pID3D12EarthDescriptorHeap);
		mpID3D12GraphicsCommandList->ExecuteBundle(mpID3D12BundleCommandList[Earth].Get());
	}
	//End
	D3D12_RESOURCE_BARRIER stD3D12REsourceBarrierEnd = Fill_D3D12_Barrier_Transition(mpID3D12RenderTargetResources[nFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	mpID3D12GraphicsCommandList->ResourceBarrier(1, &stD3D12REsourceBarrierEnd);

	//Close
	RETURN_IF_FAILED(mpID3D12GraphicsCommandList->Close());
	return S_OK;
}

void OnUpdate() {
	n64FrameCurrentTime = ::GetTickCount64();
	dModelRotationYAngle += ((n64FrameCurrentTime - n64FrameStatrTime) / 1000.0f) * fPalstance;
	if (dModelRotationYAngle > XM_2PI)
		dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);

	XMMATRIX m = XMMatrixRotationY(static_cast<float>(dModelRotationYAngle));

	XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(Pos, At, Up);

	XMMATRIX p = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(g_nClientWidth) / g_nClientHeight, 1.0f, 1000.0f);

	XMMATRIX m_MVP = m * v * p;

	MVPBuffer objConstants = {};
	XMStoreFloat4x4(&objConstants.m_MVP, XMMatrixTranspose(m_MVP));
	memcpy(mpConstBufferData[SkyBox], &objConstants, sizeof(objConstants));
	memcpy(mpConstBufferData[Earth], &objConstants, sizeof(objConstants));
}

void OnRender() {
	try {
		THROW_IF_FAILED(Populate_CommandList());
		ID3D12CommandList* ppCommandLists[] = { mpID3D12GraphicsCommandList.Get() };
		mpID3D12DirectCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		THROW_IF_FAILED(mpIDXGISwapChain3->Present(1, 0));
		THROW_IF_FAILED(Wait_For_Previous_Frame(mpID3D12DirectCommandQueue.Get(), mpID3D12Fence.Get(), &mnFenceCPUValue, mhFenceEvent));

	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		exit(-1);
	}
}

LRESULT CALLBACK Wnd_Proc(_In_opt_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	switch (message) {
	case WM_PAINT: {
		OnUpdate();
		OnRender();
		return 0;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
				   /*case WM_KEYUP: {
					   if (VK_SPACE == (wParam & 0xFF)) {
						   ++mnCurrentSamplerNO;
						   mnCurrentSamplerNO %= mnSamplerNum[Earth];
					   }
					   return 0;
				   }*/
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

_Check_return_ HRESULT mWIndow(_In_ HINSTANCE hInstance, _In_ int nCmdShow) {
	RETURN_IF_FAILED(Register_WndClassEx(hInstance, &g_wstrWndClassName, Wnd_Proc));
	RETURN_IF_FAILED(Initialize_WindowEx(hInstance, &g_wstrWndClassName, &g_wstrWindowExName, g_nClientWidth, g_nClientHeight, &g_hWnd));
	return S_OK;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	try {
		THROW_IF_FAILED(Get_CatallogPath(&g_tstrCatallogPath));
		THROW_IF_FAILED(mWIndow(hInstance, nCmdShow));
		THROW_IF_FAILED(Load_Pipeline());
		THROW_IF_FAILED(Load_Asset());
		THROW_IF_FAILED(Bundle_Command());
		n64FrameStatrTime = ::GetTickCount64();
		MSG msg = {};
		ShowWindow(g_hWnd, nCmdShow);
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

	CloseHandle(mhFenceEvent);
	return 0;
}