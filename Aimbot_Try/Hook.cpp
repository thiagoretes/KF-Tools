////pure d3d aim 1.0
////simplified version 
//
#include <Windows.h>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <d3dx9.h>
#pragma comment(lib, "User32.lib")
#include <fstream>

#pragma comment(lib, "d3dx9.lib")
//#include "detours/detours.h"	//detours 1.5
#include "detours.h"	//detours 3.0 (google microsoft detours)
#pragma comment (lib, "detours.lib") //detours 3.0
#include <vector>
using namespace std;

//==========================================================================================================================

signed int __stdcall hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 Device, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT primCount);
typedef HRESULT(__stdcall* DrawIndexedPrimitive_t)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitive_t OrigDrawIndexedPrimitive;

signed int __stdcall hkEndScene(LPDIRECT3DDEVICE9 Device);
typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
EndScene_t OrigEndScene;

//==========================================================================================================================

// settings
DWORD aimkey = VK_RBUTTON;				//aimkey (google Virtual-Key Codes)
DWORD enablekey = VK_NUMPAD9;				//aimkey (google Virtual-Key Codes)
bool enabled = true;
int aimfov = 90;						//aim fov in %
int aimheight = 0;					//adjust aim height, 0 = feet
int aimsmooth = 1;					//aim smooth (mouse accel messes with aiming)
FILE *pFile = nullptr;
// get stride
IDirect3DVertexBuffer9 *pStreamData;
UINT XOffset = 0;
UINT Stride = 0;

// get pshader
IDirect3DPixelShader9* pShader;
UINT psData;

//get vshader
IDirect3DVertexShader9* vShader;
UINT vsData;

//get viewport
D3DVIEWPORT9 viewport;

//MyAimPoint
D3DXVECTOR3 aimPoint;

//MyVertex
LPDIRECT3DVERTEXBUFFER9 myVertex;
//models
bool MODELS;

#define clot ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==2601 || nVertices==2018 || nVertices==1404) && (pCount==4400 || pCount==3350 || pCount==2154)&& Stride == 32 && sIndex==0))
#define crawler ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==4613 || nVertices==1725 || nVertices==2639) && (pCount==7404 || pCount==2170 || pCount==3654) && Stride == 32 && sIndex==0))
#define gorefast ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==2446 || nVertices==1252) && (pCount==4138 || 1874) && Stride == 32 && sIndex==0))
#define bloat ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==2795 || nVertices==1648) && (pCount==4680 || pCount==2602) && Stride == 32 && sIndex==0))
#define stalker ((BaseVertexIndex==0 && MinIndex==0 && nVertices==3193 && pCount==5422 && Stride == 32 && sIndex==0))
#define siren ((BaseVertexIndex==0 && MinIndex==0 && nVertices==306 && pCount==228 && Stride == 32 && sIndex==0))
#define siren2 ((BaseVertexIndex==0 && MinIndex==306 && nVertices==1924 && pCount==2634 && Stride == 32 && sIndex==684))
#define scrake ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==3888 || nVertices==4433 || nVertices==2740) && (pCount==4970 || pCount==3621 || pCount==5875)&& Stride == 32 && sIndex==0))
#define fleshpound ((BaseVertexIndex==0 && MinIndex==0 && (nVertices==2269 || nVertices==5094 || nVertices==4241) && (pCount==2160 || pCount==4761 || pCount==6173) && Stride == 32 && sIndex==0))
#define patriarch_reloading ((BaseVertexIndex==0 && (MinIndex==213 || MinIndex==1166) && (nVertices==1463 || nVertices==4150) && (pCount==1935 || pCount==6233) && Stride == 32 && (sIndex==414 || sIndex==2400)))
#define husk ((BaseVertexIndex==0 && MinIndex==44 && (nVertices==7724 || nVertices==4706) && (pCount==8809 || 5846) && Stride == 32 && sIndex==120))
//timer
//DWORD gametick0 = timeGetTime();

//==========================================================================================================================

void DrawPoint(LPDIRECT3DDEVICE9 Device, int baseX, int baseY, int baseW, int baseH, D3DCOLOR Cor)
{
	D3DRECT BarRect = { baseX, baseY, baseX + baseW, baseY + baseH };
	Device->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, Cor, 0, 0);
}

struct ModelInfo_t
{
	D3DXVECTOR3 Position2D;
	D3DXVECTOR3 Position3D;
	float CrosshairDistance;
};
vector<ModelInfo_t*>ModelInfo;

float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

typedef struct CUSTOMVERTEX
{
	D3DXVECTOR3 p; // Vertex position
	D3DXVECTOR3 n; // Vertex normal
	D3DXVECTOR2 texture;// Texture co-ordinate
} CUSTOMVERTEX;
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ  |  D3DFVF_NORMAL  |  D3DFVF_TEX1 )
CUSTOMVERTEX *StreamSourceZica;

//w2s for unreal engine 3 games
/*void AddModel(LPDIRECT3DDEVICE9 Device)
{
	ModelInfo_t* pModel = new ModelInfo_t;

	Device->GetViewport(&viewport);
	D3DXMATRIX pProjection, pView, pWorld;
	D3DXVECTOR3 vOut(0, 0, 0), vIn(0, 0, (float)aimheight);

	Device->GetVertexShaderConstantF(0, pProjection, 4);
	Device->GetVertexShaderConstantF(231, pView, 4);

	D3DXMatrixIdentity(&pWorld);
	D3DXVec3Project(&vOut, &vIn, &viewport, &pProjection, &pView, &pWorld);

	if (vOut.z < 1.0f && pProjection._44 > 1.0f)
	{
		pModel->Position2D.x = vOut.x;
		pModel->Position2D.y = vOut.y;
	}

	ModelInfo.push_back(pModel);
}*/


//w2s for some shader driven games
/*void AddModel(LPDIRECT3DDEVICE9 Device)
{
ModelInfo_t* pModel = new ModelInfo_t;

D3DXMATRIX matrix, m1;
D3DXVECTOR4 position;
D3DXVECTOR4 input;
Device->GetViewport(&viewport);
Device->GetVertexShaderConstantF(0, matrix, 4); //many games use 0

input.y = (float)aimheight;

D3DXMatrixTranspose(&matrix, &matrix);
D3DXVec4Transform(&position, &input, &matrix);
//or this (depends on the game)
//D3DXMatrixTranspose(&m1, &matrix);
//D3DXVec4Transform(&position, &input, &m1);

position.x = input.x * matrix._11 + input.y * matrix._21 + input.z * matrix._31 + matrix._41;
position.y = input.x * matrix._12 + input.y * matrix._22 + input.z * matrix._32 + matrix._42;
position.z = input.x * matrix._13 + input.y * matrix._23 + input.z * matrix._33 + matrix._43;
position.w = input.x * matrix._14 + input.y * matrix._24 + input.z * matrix._34 + matrix._44;

pModel->Position2D.x = ((position.x / position.w) * (viewport.Width / 2)) + viewport.X + (viewport.Width / 2);
pModel->Position2D.y = viewport.Y + (viewport.Height / 2) - ((position.y / position.w) * (viewport.Height / 2));

ModelInfo.push_back(pModel);
}*/

//w2s for old settransform games
void AddModel(LPDIRECT3DDEVICE9 Device)
{
ModelInfo_t* pModel = new ModelInfo_t;

Device->GetViewport(&viewport);
D3DXMATRIX projection, view, world, myth;

D3DXVECTOR3 vScreenCoord(0, 0, 0), vWorldLocation(0, 0, 0);


Device->GetTransform(D3DTS_VIEW, &view);
Device->GetTransform(D3DTS_PROJECTION, &projection);
Device->GetTransform(D3DTS_WORLD, &world);
Device->GetTransform(D3DTS_WORLD, &myth);

D3DXMatrixTranslation(&myth, aimPoint.x, aimPoint.y, aimPoint.z);
vWorldLocation.x = myth._41;
vWorldLocation.y = myth._42;
vWorldLocation.z = myth._43;

D3DXVec3Project(&vScreenCoord, &vWorldLocation, &viewport, &projection, &view, &world);
//printf("WORLD Matrix: 14=%.3f 24=%.3f 34=%.3f\n", world._41, world._42, world._43);
//printf("WORLD: X=%.3f Y=%.3f Z=%.3f\n", vWorldLocation.x, vWorldLocation.y, vWorldLocation.z);
//printf("ScreenCoord: X=%.3f Y=%.3f Z=%.3f\n", vScreenCoord.x, vScreenCoord.y, vScreenCoord.z);
if (vScreenCoord.z < 1 && vScreenCoord.z >= 0 && aimPoint.z > 20)
{
	
	pModel->Position2D.x = vScreenCoord.x;
	pModel->Position2D.y = vScreenCoord.y;
	ModelInfo.push_back(pModel);
}


}

void doDisassembleShader(LPDIRECT3DDEVICE9 pDevice, char* FileName)
{
	std::ofstream oLogFile(FileName, std::ios::trunc);

	if (!oLogFile.is_open())
		return;

	IDirect3DVertexShader9* pShader;

	pDevice->GetVertexShader(&pShader);

	UINT pSizeOfData;

	pShader->GetFunction(NULL, &pSizeOfData);

	BYTE* pData = new BYTE[pSizeOfData];

	pShader->GetFunction(pData, &pSizeOfData);

	LPD3DXBUFFER bOut;

	D3DXDisassembleShader(reinterpret_cast<DWORD*>(pData), NULL, NULL, &bOut);

	oLogFile << static_cast<char*>(bOut->GetBufferPointer()) << std::endl;

	oLogFile.close();

	delete[] pData;

	pShader->Release();

}

//void doDisassembleShader(LPDIRECT3DDEVICE9 pDevice, char* FileName)
//{
//
//	FILE* fp = fopen(FileName, "a");
//	//if (fp == NULL) return;
//	IDirect3DVertexShader9* pShader;
//	pDevice->GetVertexShader(&pShader);
//	UINT pSizeOfData;
//	pShader->GetFunction(NULL, &pSizeOfData);
//	BYTE* pData = new BYTE[pSizeOfData];
//	pShader->GetFunction(pData, &pSizeOfData);
//	LPD3DXBUFFER bOut;
//	D3DXDisassembleShader(reinterpret_cast<DWORD*>(pData), NULL, NULL, &bOut);
//	fprintf(fp, "%s\n", static_cast<char*>(bOut->GetBufferPointer()));
//	
//	//oLogFile << static_cast<char*>(bOut->GetBufferPointer()) << std::endl;
//	//oLogFile.close();
//	fclose(fp);
//	delete[] pData;
//	pShader->Release();
//}

HRESULT __stdcall myDrawIndexedPrimitive(LPDIRECT3DDEVICE9 Device, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT nVertices, UINT sIndex, UINT pCount)
{
	LPD3DXBUFFER bOut;
	
	MODELS = FALSE;
	
	//get stride
	if (Device->GetStreamSource(0, &pStreamData, &XOffset, &Stride) == D3D_OK)
		if (pStreamData != NULL) { 
			//pStreamData->Lock(0, sizeof(pStreamData))
			if (Stride == 32) {
				pStreamData->Lock(0, 0, (void**)&StreamSourceZica, D3DLOCK_READONLY);
				
				if (clot)
				{
					//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[0].p.x, StreamSourceZica[0].p.y, StreamSourceZica[0].p.z, StreamSourceZica[0].texture.x, StreamSourceZica[0].texture.y);
					for (int i = 0; i < nVertices; i++)
					{	
						if (StreamSourceZica[i].texture.x > 0.74f && StreamSourceZica[i].texture.x < 0.8f && StreamSourceZica[i].texture.y > 0.136f && StreamSourceZica[i].texture.y < 0.200f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
					

				}
				else if (gorefast)
				{
					
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.3f && StreamSourceZica[i].texture.x < 0.335f && StreamSourceZica[i].texture.y > 0.1f && StreamSourceZica[i].texture.y < 0.150f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (stalker)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.272f && StreamSourceZica[i].texture.x < 0.325f && StreamSourceZica[i].texture.y > 0.07f && StreamSourceZica[i].texture.y < 0.140f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (husk)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.663f && StreamSourceZica[i].texture.x < 0.7f && StreamSourceZica[i].texture.y > 0.16f && StreamSourceZica[i].texture.y < 0.187f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (crawler)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.2f && StreamSourceZica[i].texture.x < 0.3f && StreamSourceZica[i].texture.y > 0.046f && StreamSourceZica[i].texture.y < 0.16f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (siren)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.142f && StreamSourceZica[i].texture.x < 0.217f && StreamSourceZica[i].texture.y > 0.118f && StreamSourceZica[i].texture.y < 0.7f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (siren2)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.2f && StreamSourceZica[i].texture.x < 0.27f && StreamSourceZica[i].texture.y > 0.066 && StreamSourceZica[i].texture.y < 0.157f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (bloat)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.41f && StreamSourceZica[i].texture.x < 0.48f && StreamSourceZica[i].texture.y > 0.1f && StreamSourceZica[i].texture.y < 0.17f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (fleshpound)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.74f && StreamSourceZica[i].texture.x < 0.85f && StreamSourceZica[i].texture.y > 0.261f && StreamSourceZica[i].texture.y < 0.340f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (patriarch_reloading)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.2f && StreamSourceZica[i].texture.x < 0.25f && StreamSourceZica[i].texture.y > 0.09f && StreamSourceZica[i].texture.y < 0.140f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				else if (scrake)
				{
					for (int i = 0; i < nVertices; i++)
					{
						if (StreamSourceZica[i].texture.x > 0.745f && StreamSourceZica[i].texture.x < 0.822f && StreamSourceZica[i].texture.y > 0.1f && StreamSourceZica[i].texture.y < 0.12f) {
							aimPoint = StreamSourceZica[i].p;
							MODELS = TRUE;
							//printf("P:%f %f %f T: %f %f\n", StreamSourceZica[i].p.x, StreamSourceZica[i].p.y, StreamSourceZica[i].p.z, StreamSourceZica[i].texture.x, StreamSourceZica[i].texture.y);
							break;
						}
					}
				}
				



				pStreamData->Unlock();
			}
			
			pStreamData->Release(); 
			pStreamData = NULL;
		}
	

	//get psdata
	if (SUCCEEDED(Device->GetPixelShader(&pShader)))
		if (pShader != NULL)
			if (SUCCEEDED(pShader->GetFunction(NULL, &psData)))
				if (pShader != NULL) { pShader->Release(); pShader = NULL; }

	//get vsdata
	if (SUCCEEDED(Device->GetVertexShader(&vShader)))
		if (vShader != NULL)
			if (SUCCEEDED(vShader->GetFunction(NULL, &vsData)))
				if (vShader != NULL) { vShader->Release(); vShader = NULL; }

		

	//printf("Stride: %d, psData: %d, vsData: %d\n", Stride, psData, vsData);
	//get models
	



	//worldtoscreen
	if (MODELS)
	{
		AddModel(Device);
	}

	return OrigDrawIndexedPrimitive(Device, Type, BaseVertexIndex, MinIndex, nVertices, sIndex, pCount);
}


//Does Nothing ATM
bool INITIALIZED = false;

HRESULT __stdcall myEndScene(LPDIRECT3DDEVICE9 Device)
{
	//aimbot & esp
	//if (timeGetTime() - gametick0 > 1) //slow it down if you only have dip bypass, put code in AddModel
	//{
	if (!INITIALIZED)
	{
		//Device->CreateVertexBuffer(5000 * sizeof(CUSTOMVERTEX), 0 /*Usage*/, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL) ) )
	}

	if ((GetAsyncKeyState(enablekey)))
		enabled = !enabled;


	if (ModelInfo.size() != NULL)
	{
		UINT BestTarget = -1;
		DOUBLE fClosestPos = 99999;

		for (size_t i = 0; i < ModelInfo.size(); i += 1)
		{
			//drawpoint on targets (Esp)
			if(enabled)
				DrawPoint(Device, (int)ModelInfo[i]->Position2D.x, (int)ModelInfo[i]->Position2D.y, 8, 8, 0xFFFF0000);

			//get screen center
			float ScreenCenterX = viewport.Width / 2.0f;
			float ScreenCenterY = viewport.Height / 2.0f;
			//int ScreenCenterX = GetSystemMetrics(0) / 2 - 1;
			//int ScreenCenterY = GetSystemMetrics(1) / 2 - 1;

			//aimfov
			float radiusx = aimfov * (ScreenCenterX / 100);
			float radiusy = aimfov * (ScreenCenterY / 100);

			//get crosshairdistance
			ModelInfo[i]->CrosshairDistance = GetDistance(ModelInfo[i]->Position2D.x, ModelInfo[i]->Position2D.y, ScreenCenterX, ScreenCenterY);

			//if in fov
			if (ModelInfo[i]->Position2D.x >= ScreenCenterX - radiusx && ModelInfo[i]->Position2D.x <= ScreenCenterX + radiusx && ModelInfo[i]->Position2D.y >= ScreenCenterY - radiusy && ModelInfo[i]->Position2D.y <= ScreenCenterY + radiusy)

				//get closest/nearest target to crosshair
				if (ModelInfo[i]->CrosshairDistance < fClosestPos)
				{
					fClosestPos = ModelInfo[i]->CrosshairDistance;
					BestTarget = i;
				}
		}

		//if nearest target to crosshair
		if (BestTarget != -1)
		{
			double DistX = (double)ModelInfo[BestTarget]->Position2D.x - viewport.Width / 2.0f;
			double DistY = (double)ModelInfo[BestTarget]->Position2D.y - viewport.Height / 2.0f;

			//aimsmooth
			DistX /= aimsmooth;
			DistY /= aimsmooth;

			//if aimkey is pressed
			if ((GetAsyncKeyState(aimkey)))
				mouse_event(MOUSEEVENTF_MOVE, (DWORD)DistX, (DWORD)DistY, NULL, NULL); //doaim, move mouse to x & y
		}

		ModelInfo.clear();
	}
	//gametick0 = timeGetTime();
	//}

	return OrigEndScene(Device);
}

//==========================================================================================================================

#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
bool bCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return false;

	return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask)
{
	for (DWORD i = 0; i < dwLen; i++)
		if (bCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD)(dwAddress + i);

	return 0;
}

void Hook()
{
	DWORD *vtbl;

	// wait for the d3dx dll
	DWORD hD3D = 0;
	do {
		hD3D = (DWORD)GetModuleHandleA("d3d9.dll");
		Sleep(10);
	} while (!hD3D);
	DWORD adre = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");

	if (adre)
	{
		memcpy(&vtbl, (void*)(adre + 2), 4);

		//ms detours 1.5
		//OrigDrawIndexedPrimitive = (DrawIndexedPrimitive_t)DetourFunction((BYTE*)vtbl[82], (BYTE*)myDrawIndexedPrimitive);
		//OrigEndScene = (EndScene_t)DetourFunction((BYTE*)vtbl[42], (BYTE*)myEndScene);

		//ms detours 3.0
		OrigDrawIndexedPrimitive = (HRESULT(__stdcall*)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT))vtbl[82];
		OrigEndScene = (HRESULT(__stdcall*)(LPDIRECT3DDEVICE9))vtbl[42];

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)OrigDrawIndexedPrimitive, myDrawIndexedPrimitive);
		DetourAttach(&(PVOID&)OrigEndScene, myEndScene);
		DetourTransactionCommit();
	}
	//Define this
	FILE *pFile = nullptr;


	//Include these after process attach switch has been triggered.
	AllocConsole();
	freopen_s(&pFile, "CONOUT$", "w", stdout);
}

//==========================================================================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD Reason, LPVOID Reserved)
{
	DisableThreadLibraryCalls(hinstDll);

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		Hook();
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Hook, 0, 0, 0);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}