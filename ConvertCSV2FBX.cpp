// ConvertCSV2FBX.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CSVFile.h"
#include <fbxsdk.h>
#include <set>
#include "CommonMath.h"

struct MeshVertex
{
	float u, v;
	float x, y, z;
	float nx, ny, nz;
	float tx, ty, tz,tw;
};

void ConvertCSV2FBX(const char* sourceCSVFile)
{
	FbxManager* sdkManager;
	FbxScene* scene;
	FbxIOSettings* m_ioSettings;
	 
	sdkManager = FbxManager::Create();

	m_ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);

	m_ioSettings->SetBoolProp(EXP_FBX_MATERIAL, true);
	m_ioSettings->SetBoolProp(EXP_FBX_TEXTURE, true);
	m_ioSettings->SetBoolProp(EXP_FBX_EMBEDDED, true);
	m_ioSettings->SetBoolProp(EXP_FBX_ANIMATION, true);
	m_ioSettings->SetBoolProp(EXP_FBX_SHAPE, true);

	m_ioSettings->SetIntProp(
		EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7400);

	sdkManager->SetIOSettings(m_ioSettings);
	scene = FbxScene::Create(sdkManager, "");

    CCSVFile* pSrcFile = new CCSVFile(sourceCSVFile);

	std::map<int, MeshVertex> mVerticsSet;
	int iVertexID = 0;
	//calc row count
	for (int iRow = 0; iRow < pSrcFile->GetRowNum(); iRow++)
	{ 
		pSrcFile->GetCellValue("IDX", iRow, iVertexID);
		if (mVerticsSet.find(iVertexID) == mVerticsSet.end())
		{
			pSrcFile->GetCellValue("in_POSITION0.x", iRow, mVerticsSet[iVertexID].x);
			pSrcFile->GetCellValue("in_POSITION0.y", iRow, mVerticsSet[iVertexID].y);
			pSrcFile->GetCellValue("in_POSITION0.z", iRow, mVerticsSet[iVertexID].z);

			pSrcFile->GetCellValue("in_NORMAL0.x", iRow, mVerticsSet[iVertexID].nx);
			pSrcFile->GetCellValue("in_NORMAL0.y", iRow, mVerticsSet[iVertexID].ny);
			pSrcFile->GetCellValue("in_NORMAL0.z", iRow, mVerticsSet[iVertexID].nz);


			pSrcFile->GetCellValue("in_TANGENT0.x", iRow, mVerticsSet[iVertexID].tx);
			pSrcFile->GetCellValue("in_TANGENT0.y", iRow, mVerticsSet[iVertexID].ty);
			pSrcFile->GetCellValue("in_TANGENT0.z", iRow, mVerticsSet[iVertexID].tz);
			pSrcFile->GetCellValue("in_TANGENT0.w", iRow, mVerticsSet[iVertexID].tw);

			pSrcFile->GetCellValue("in_TEXCOORD0.x", iRow, mVerticsSet[iVertexID].u);
			pSrcFile->GetCellValue("in_TEXCOORD0.y", iRow, mVerticsSet[iVertexID].v);
		}
	}
	int iTotalVerticsCount = mVerticsSet.size();

	std::string matName = sourceCSVFile;
	matName = matName.substr(0, matName.size() - 4);
	
	int pos = matName.find_last_of('\\');
	if (pos != std::string::npos)
	{
		matName = matName.substr(pos + 1, matName.size() - pos- 1);
	}

	FbxNode* rootNode = scene->GetRootNode();
	FbxSurfacePhong* materialFbx = FbxSurfacePhong::Create(sdkManager, matName.c_str());
	 
	FbxNode* subshapeNode = FbxNode::Create(rootNode, matName.c_str());
	subshapeNode->AddMaterial(materialFbx);
	//convert fbx mesh file 
	int                          index;
	FbxMesh* meshFbx = FbxMesh::Create(subshapeNode, subshapeNode->GetName());
	FbxGeometryElementUV* meshUVs = meshFbx->CreateElementUV("UV");
	FbxGeometryElementNormal* meshNormals = meshFbx->CreateElementNormal();
	FbxGeometryElementTangent* meshTangents = meshFbx->CreateElementTangent();
	FbxGeometryElementMaterial* meshMaterials = meshFbx->CreateElementMaterial();

	meshMaterials->SetMappingMode(FbxGeometryElement::eByPolygon);
	meshMaterials->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	meshMaterials->GetIndexArray().Add(0);

	meshFbx->InitControlPoints(iTotalVerticsCount);
	meshUVs->SetMappingMode(FbxGeometryElementUV::eByControlPoint);
	meshUVs->SetReferenceMode(FbxGeometryElementUV::eDirect);
	meshNormals->SetMappingMode(FbxGeometryElement::eByControlPoint);
	meshNormals->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxVector4* meshVectors = meshFbx->GetControlPoints();
 
	Matrix44 matRot;
	MatrixRotationZ(&matRot, -FLT_DTOR(90));
	for (index = 0; index < mVerticsSet.size(); index++)
	{
		Vector3 Vertex(mVerticsSet[index].x*100, mVerticsSet[index].y * 100, mVerticsSet[index].z * 100);
		Vec3TransformCoord(&Vertex, &Vertex, &matRot);
		meshVectors[index].Set(Vertex.x, Vertex.y, Vertex.z);
		  
		meshUVs->GetDirectArray().Add(FbxVector2(mVerticsSet[index].u,  mVerticsSet[index].v));

		Vector3 VertexNormal(mVerticsSet[index].nx, mVerticsSet[index].ny, mVerticsSet[index].nz); 
		Vec3TransformNormal(&VertexNormal, &VertexNormal, &matRot);

		meshNormals->GetDirectArray().Add(FbxVector4(VertexNormal.x,
			VertexNormal.y,
			VertexNormal.z, 0));

		Vector4 VertexTangent(mVerticsSet[index].tx, mVerticsSet[index].ty, mVerticsSet[index].tz, mVerticsSet[index].tw);
		VertexTangent = VertexTangent*matRot;

		meshTangents->GetDirectArray().Add(FbxVector4(VertexTangent.x,
			VertexTangent.y,
			VertexTangent.z, VertexTangent.w));
 
	}
	int iFaceID = 0;
	for (int iRow = 0; iRow < pSrcFile->GetRowNum(); iRow +=3)
	{
		meshFbx->BeginPolygon(0); 

		pSrcFile->GetCellValue("IDX", iRow, iFaceID);
		meshFbx->AddPolygon(iFaceID);
		pSrcFile->GetCellValue("IDX", iRow + 1, iFaceID);
		meshFbx->AddPolygon(iFaceID);
		pSrcFile->GetCellValue("IDX", iRow + 2, iFaceID);
		meshFbx->AddPolygon(iFaceID);
		meshFbx->EndPolygon();
	}

 
	subshapeNode->SetNodeAttribute(meshFbx);
	subshapeNode->SetShadingMode(FbxNode::eTextureShading);



	//save fbx
	FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
	std::string fbxFilePath = sourceCSVFile;
	fbxFilePath = fbxFilePath.substr(0, fbxFilePath.size() - 3);
	fbxFilePath += "fbx";
	if (!exporter->Initialize(fbxFilePath.c_str(), -1, m_ioSettings))
	{
		fprintf(stderr, "Failed to initialize FBX exporter\n");
		exporter->Destroy();
		return ;
	}
	exporter->SetFileExportVersion(FBX_2014_00_COMPATIBLE);

	if (!exporter->Export(scene))
	{
		fprintf(stderr, "Failed to produce FBX file\n");
		exporter->Destroy();
		return  ;
	}

	exporter->Destroy();

}

int main(int argc, char* argv[])
{
	ConvertCSV2FBX(argv[1]);
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 

