#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<commdlg.h>
#include<wrl.h>

#include<tchar.h>

#include<string>
#include<fstream>
#include<utility>
#include<vector>
#include<unordered_map>
#include<memory>
#include <iostream>


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

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

using std::wstring;
using std::pair;
using std::vector;
using std::unordered_map;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

const XMMATRIX& MXEqual(XMMATRIX& mxDX, const aiMatrix4x4& mxAI) {
	mxDX = XMMatrixSet(
		mxAI.a1, mxAI.a2, mxAI.a3, mxAI.a4,
		mxAI.b1, mxAI.b2, mxAI.b3, mxAI.b4,
		mxAI.c1, mxAI.c2, mxAI.c3, mxAI.c4,
		mxAI.d1, mxAI.d2, mxAI.d3, mxAI.d4);
	return mxDX;
}


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


_Check_return_ HRESULT Get_CatallogPath(_Out_ wstring* wstrCatallogPath) {

	TCHAR path[MAX_PATH] = {};
	if (0 == GetModuleFileName(nullptr, path, MAX_PATH))
		return HRESULT_FROM_WIN32(GetLastError());

	*wstrCatallogPath = path;

	// NOTE : Get Catallog
	{
		//NOTE : Erase "fimename.exe"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L'\\'));
		//NOTE : Erase "x64"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L"\\"));
		//NOTE : Erase "Debug/Release"
		wstrCatallogPath->erase(wstrCatallogPath->find_last_of(L"\\") + 1);
	}

	return S_OK;
}

constexpr UINT GRS_BONE_DATACNT{ 4 };
constexpr UINT GRS_MAX_BONES{ 256 };
constexpr UINT GRS_INDICES_PER_FACE{ 3 };

static Assimp::Importer g_assimpImporter{};

class ST_GRS_VERTEX_BONE {
public:
	UINT m_nBoneIndex[GRS_BONE_DATACNT];
	float m_fBoneWeight[GRS_BONE_DATACNT];

	void AddBOneData(UINT bBoneID, FLOAT fBoneWeight) {
		for (UINT Index = 0; Index < GRS_BONE_DATACNT; ++Index) {
			if (0.0f == m_fBoneWeight[Index]) {
				m_nBoneIndex[Index] = bBoneID;
				m_fBoneWeight[Index] = fBoneWeight;
				return;
			}
		}
	}
};

struct  ST_GRS_SUBMESH_DATA {
	UINT m_nNumIndices;
	UINT m_nBaseVertex;
	UINT m_nBaseIndex;
	UINT m_nMaterialIndex;

	BOOL m_bHasPosition;
	BOOL m_bHasNormal;
	BOOL M_bHasTexCoords;
	BOOL m_bHasTangent;
};

struct ST_GRS_BONE_DATA {
	XMMATRIX m_MxBoneOffset;
	XMMATRIX m_MxFinalTransformation;
};

struct  ST_GRS_MESH_DATA {
	XMMATRIX m_mxModle;
	wstring m_wstrFileName;
	const aiScene* m_paiModel;

	vector<ST_GRS_SUBMESH_DATA> m_vecSubMeshData;

	vector<XMFLOAT4> m_vecPositions;
	vector<XMFLOAT4> m_vecNormals;
	vector<XMFLOAT2> m_vecTexCoords;
	vector<XMFLOAT4> m_vecTangents;
	vector<XMFLOAT4> mvecBitangents;

	vector<ST_GRS_VERTEX_BONE> m_vecVertexBones;
	vector<UINT> m_vecIndices;

	vector<wstring> m_vecTextureNames;
	unordered_map<wstring, UINT> m_mapTextureNameToIndex;
	unordered_map<UINT, UINT> m_mapTextureIndexToHeapIndex;

	vector<ST_GRS_BONE_DATA> m_vecBoneData;

	unordered_map<wstring, UINT> m_mapBoneNameToIndex;
	unordered_map<wstring, UINT> m_mapanimationNameToIndex;

	UINT m_nCurrentAnimationIndex;
};


HRESULT LoadMesh(ST_GRS_MESH_DATA& stMeshData, UINT nFlag) {

	stMeshData.m_nCurrentAnimationIndex = 0;

	int SizeNeed = WideCharToMultiByte(CP_UTF8, 0, stMeshData.m_wstrFileName.c_str(), static_cast<int>(stMeshData.m_wstrFileName.size()), nullptr, 0, nullptr, nullptr);
	std::string strFileName(SizeNeed, 0);
	WideCharToMultiByte(CP_UTF8, 0, stMeshData.m_wstrFileName.c_str(), static_cast<int>(stMeshData.m_wstrFileName.size()), strFileName.data(), SizeNeed, nullptr, nullptr);

	stMeshData.m_paiModel = g_assimpImporter.ReadFile(strFileName, nFlag);
	if (nullptr == stMeshData.m_paiModel)
		return E_FAIL;

	stMeshData.m_mxModle = XMMatrixTranspose(MXEqual(stMeshData.m_mxModle, stMeshData.m_paiModel->mRootNode->mTransformation));

	UINT nNumMeshes = stMeshData.m_paiModel->mNumMeshes;
	if (0 == nNumMeshes)
		return E_FAIL;
	stMeshData.m_vecSubMeshData.resize(nNumMeshes);

	const aiMesh* paiSubMesh = nullptr;
	const aiVector3D Zero3D{ 0.0f,0.0f,0.0f };
	UINT nNumBones{ 0 };
	UINT nNumVertices{ 0 };
	UINT nNumIndices{ 0 };

	for (UINT Index = 0; Index < nNumMeshes; ++Index) {
		paiSubMesh = stMeshData.m_paiModel->mMeshes[Index];

		stMeshData.m_vecSubMeshData[Index].m_nMaterialIndex = paiSubMesh->mMaterialIndex;
		stMeshData.m_vecSubMeshData[Index].m_nNumIndices = paiSubMesh->mNumFaces * GRS_INDICES_PER_FACE;
		stMeshData.m_vecSubMeshData[Index].m_nBaseVertex = nNumVertices;
		stMeshData.m_vecSubMeshData[Index].m_nBaseIndex = nNumIndices;

		stMeshData.m_vecSubMeshData[Index].m_bHasPosition = paiSubMesh->HasPositions();
		stMeshData.m_vecSubMeshData[Index].m_bHasNormal = paiSubMesh->HasNormals();
		stMeshData.m_vecSubMeshData[Index].M_bHasTexCoords = paiSubMesh->HasTextureCoords(0);
		stMeshData.m_vecSubMeshData[Index].m_bHasTangent = paiSubMesh->HasTangentsAndBitangents();

		nNumVertices += stMeshData.m_paiModel->mMeshes[Index]->mNumVertices;
		nNumIndices += stMeshData.m_vecSubMeshData[Index].m_nNumIndices;

		for (UINT J = 0; J < paiSubMesh->mNumVertices; ++J) {
			if (stMeshData.m_vecSubMeshData[Index].m_bHasPosition) {
				stMeshData.m_vecPositions.push_back(XMFLOAT4{
					paiSubMesh->mVertices[J].x,
					paiSubMesh->mVertices[J].y,
					paiSubMesh->mVertices[J].z,
					1.0f
					}
				);
			}
			else
				stMeshData.m_vecPositions.push_back(XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f });

			if (stMeshData.m_vecSubMeshData[Index].m_bHasNormal) {
				stMeshData.m_vecNormals.push_back(XMFLOAT4{
					paiSubMesh->mNormals[J].x,
					paiSubMesh->mNormals[J].y,
					paiSubMesh->mNormals[J].z,
					0.0f
					}
				);
			}
			else
				stMeshData.m_vecNormals.push_back(XMFLOAT4{ 0.0f,0.0f,0.0f,0.0f });

			if (stMeshData.m_vecSubMeshData[Index].M_bHasTexCoords) {
				stMeshData.m_vecTexCoords.push_back(XMFLOAT2{
					paiSubMesh->mTextureCoords[0][J].x,
					paiSubMesh->mTextureCoords[0][J].y
					}
				);
			}
			else
				stMeshData.m_vecTexCoords.push_back(XMFLOAT2{ 0.0f,0.0f });

			if (stMeshData.m_vecSubMeshData[Index].m_bHasTangent) {
				stMeshData.m_vecTangents.push_back(XMFLOAT4{
					paiSubMesh->mTangents[J].x,
					paiSubMesh->mTangents[J].y,
					paiSubMesh->mTangents[J].z,
					0.0f
					}
				);
				stMeshData.mvecBitangents.push_back(XMFLOAT4{
					paiSubMesh->mBitangents[J].x,
					paiSubMesh->mBitangents[J].y,
					paiSubMesh->mBitangents[J].z,
					0.0f
					}
				);
			}
			else {
				stMeshData.m_vecTangents.push_back(XMFLOAT4{ 0.0f,0.0f,0.0f,0.0f });
				stMeshData.mvecBitangents.push_back(XMFLOAT4{ 0.0f,0.0f,0.0f,0.0f });
			}
		}


	}

	return S_OK;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	//Path
	wstring wstrCatallogPath{};

	//Mesh Data
	ST_GRS_MESH_DATA stMeshData{};

	OPENFILENAMEW ofn{};

	try {
		//NOTE : Initialize WIC and COM
		THROW_IF_FAILED(::CoInitialize(nullptr));

		//NOTE : Window Catallog Path
		THROW_IF_FAILED(Get_CatallogPath(&wstrCatallogPath));

		// NOTE : LOAD MODLE
		{
			//NOTE : Defualt File
			stMeshData.m_wstrFileName.resize(MAX_PATH);
			//BUG : Must Use Windows File  Path Separator
			stMeshData.m_wstrFileName = L"D:\\Amy_3\\Directx12\\Assets\\third_party_model\\hero.X";

			//NOTE : Inittialize OPENFILENAMEW
			{
				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = nullptr;
				ofn.hInstance = nullptr;

				ofn.lpstrFilter =
					L"X File\0*.x;*.X\0"
					L"ObJ File\0*.obj\0"
					L"FBX File\0*.fbx\0"
					L"所有文件\0*.*\0";
				ofn.lpstrCustomFilter = nullptr;
				ofn.nMaxCustFilter = 0;

				ofn.lpstrFile = stMeshData.m_wstrFileName.data();
				ofn.nMaxFile = MAX_PATH;
				//ofn.lpstrFileTitle = nullptr;// NOTE : OUT File Name
				ofn.lpstrInitialDir = nullptr;
				ofn.lpstrDefExt = nullptr;

				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

				//NOTE : OUT FIle Name First Index In Path
				ofn.nFileOffset = 0;

				ofn.lpstrTitle = L"Open File";
			}

			if (!GetOpenFileNameW(&ofn))
				throw ::HRESULT_FROM_WIN32(::GetLastError());



		}

	}
	catch (COMException& e) {
		::OutputDebugString((std::to_wstring(e.Line) + _T('\n')).c_str());
		return e.Error();

	}
}





