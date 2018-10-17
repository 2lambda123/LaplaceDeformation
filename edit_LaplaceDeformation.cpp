#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <meshlab/glarea.h>
#include "edit_LaplaceDeformation.h"
#include <wrap/gl/pick.h>
#include <wrap/qt/gl_label.h>
#include <eigenlib/Eigen/Cholesky>
#include <fstream>

using namespace std;	// MatrixXd ������ std::cout << ��ֱ�� cout << m << endl;����
using namespace vcg;
using Eigen::MatrixXf;
using Eigen::VectorXf;

EditLaplaceDeformationPlugin::EditLaplaceDeformationPlugin()
{
	qFont.setFamily("Helvetica");
	qFont.setPixelSize(12);

	haveToPick = false;
	pickmode = 0; // 0 face 1 vertex
	curFacePtr = 0;
	curVertPtr = 0;
	pIndex = 0;
}

const QString EditLaplaceDeformationPlugin::Info()
{
	return tr("Laplace Deformation on a model!");
}

void EditLaplaceDeformationPlugin::ChimneyRotate(MeshModel &m)
{
	fixed_anchor_idx.clear();
	move_anchor_idx.clear();
	Vertices.clear();
	Faces.clear();
	AdjacentVertices.clear();
	tri::UpdateSelection<CMeshO>::VertexClear(m.cm);
	tri::UpdateSelection<CMeshO>::FaceClear(m.cm);

	int tmp;
	ifstream move_anchor;
	move_anchor.open("C:\\Users\\Administrator\\Desktop\\top_anchor.txt");
	while (move_anchor >> tmp)
		move_anchor_idx.push_back(tmp);
	move_anchor.close();

	ifstream fix_anchor;
	fix_anchor.open("C:\\Users\\Administrator\\Desktop\\bot_anchor.txt");
	while (fix_anchor >> tmp)
		fixed_anchor_idx.push_back(tmp);
	fix_anchor.close();

	// move_anchor��Ϊ��ɫ��move_anchor��Ϊ��ɫ
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	//	if (!vi->IsD())
	//	{
	//		int tmp = vi - m.cm.vert.begin();
	//		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), tmp) != move_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Red);
	//		else if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), tmp) != fixed_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Blue);
	//	}
	
//// test ������ת����д�öԲ���
	//float angle = (45 * M_PI) / 180;	// ÿ����ת10��
	//cout << "sin = " << sin(angle) << endl;
	//cout << "cos = " << cos(angle) << endl << endl;
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	//{
	//	int idx = vi - m.cm.vert.begin();
	//	if (find(move_anchor_idx.begin(), move_anchor_idx.end(), idx) != move_anchor_idx.end())
	//	{
	//		float tx = m.cm.vert[idx].P().X(), &ty = m.cm.vert[idx].P().Y();
	//		printf("Before Rotate, (%f, %f)\n", tx, ty);
	//		m.cm.vert[idx].P().X() = tx * cos(angle) + ty * sin(angle);
	//		m.cm.vert[idx].P().Y() = -tx * sin(angle) + ty * cos(angle);
	//		printf("After Rotate, (%f, %f)\n\n", m.cm.vert[idx].P().X(), m.cm.vert[idx].P().Y());
	//	}
	//}
//// test ������ת����д�öԲ���

//// Laplace Deformation - Rotate
	toCaculateAdjacentVertices(&m.cm);
	printf("\nVertices = %d\n", Vertices.size());
	printf("Faces = %d\n\n", Faces.size());

	get_LsTLs_Matrix();
	printf("LsTLs ����������\n");

	int fix_anchors = fixed_anchor_idx.size();
	int move_anchors = move_anchor_idx.size();
	int points_num = Vertices.size();
	VectorXf vx(points_num), vy(points_num);

	for (int i = 0; i < points_num; i++)
	{
		vx[i] = Vertices[i].X();
		vy[i] = Vertices[i].Y();
	}

	bx = L * vx;	// ����laplace�����������е�ĵ�laplace����
	by = L * vy;	// ����laplace�����������е�ĵ�laplace����

	bx.conservativeResize(points_num + fix_anchors + move_anchors);
	by.conservativeResize(points_num + fix_anchors + move_anchors);


	for (int i = 0; i < fix_anchors; i++)
	{
		bx[i + points_num] = Vertices[fixed_anchor_idx[i]].X();
		by[i + points_num] = Vertices[fixed_anchor_idx[i]].Y();
	}

	float angle = (15.0 * M_PI) / 180.0;	// ÿ����ת10�ȣ�����д 90 / 180 * pi��90/180 = 0.��

	// x_new = x * cos + y * sin
	// y_new = -x * sin + y * cos
	for (int i = 0; i < move_anchors; i++)
	{
		float tx = Vertices[move_anchor_idx[i]].X(), ty = Vertices[move_anchor_idx[i]].Y();
		bx[i + points_num + fix_anchors] = tx * cos(angle) + ty * sin(angle);
		by[i + points_num + fix_anchors] = -tx * sin(angle) + ty * cos(angle);
	}
	LsTbx = LsT * bx;
	LsTby = LsT * by;
	printf("LsTb ����������, ͨ��cholesky�ֽ⣬�����Է����� LsTLs * x = LsTb\n\n\n");

	vx_new = LsTLs.llt().solve(LsTbx);
	vy_new = LsTLs.llt().solve(LsTby);

	for (int i = 0; i < m.cm.vert.size(); i++)
	{
		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), i) != move_anchor_idx.end())
		{
			float tx = Vertices[i].X(), ty = Vertices[i].Y();
			m.cm.vert[i].P().X() = tx * cos(angle) + ty * sin(angle);
			m.cm.vert[i].P().Y() = -tx * sin(angle) + ty * cos(angle);
		}
		else
		{
			if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), i) == fixed_anchor_idx.end())	// �Ȳ���move�㣬�ֲ��ǹ̶���
			{
				m.cm.vert[i].P().X() = vx_new[i];
				m.cm.vert[i].P().Y() = vy_new[i];
			}
		}
	}
	//// Laplace Deformation - Rotate

	m.updateDataMask(MeshModel::MM_POLYGONAL);
	tri::UpdateBounding<CMeshO>::Box(m.cm);
	tri::UpdateNormal<CMeshO>::PerVertexNormalizedPerFaceNormalized(m.cm);

	////// ����update����
	gla->mvc()->sharedDataContext()->meshInserted(m.id());
	MLRenderingData dt;
	gla->mvc()->sharedDataContext()->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	suggestedRenderingData(m, dt);
	MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	gla->mvc()->sharedDataContext()->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	gla->update();
	////// ����update����
}

// ����ǰѡ��ģ��
bool EditLaplaceDeformationPlugin::StartEdit(MeshModel & m, GLArea * _gla, MLSceneGLSharedDataContext* ctx)
{
	gla = _gla;

//// ���̴���תʵ�� 2018.8.31�ɹ������ǲ��������������ת������Ƭ�������ཻ
	//ChimneyRotate(m);
	//return true;
//// ���̴���תʵ��

	// Ϊʲôģ����������ѡ�еĵ㣿��������ClearS
	/*for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	if (!vi->IsD() && vi->IsS())
	{
	(*vi).C() = vcg::Color4b(vcg::Color4b::Red);
	cout << vi - m.cm.vert.begin() << endl;
	}*/

//// Iniatial ����
	fixed_anchor_idx.clear();
	move_anchor_idx.clear();
	tri::UpdateSelection<CMeshO>::VertexClear(m.cm);
	tri::UpdateSelection<CMeshO>::FaceClear(m.cm);
	//tri::UpdateFlags<CMeshO>::FaceClearS(m.cm);		// ����
	//tri::UpdateFlags<CMeshO>::VertexClearS(m.cm);		// ����

	move_anchor_coord.resize(m.cm.vert.size());
	for (int i = 0; i < move_anchor_coord.size(); i++)
	{
		move_anchor_coord[i].X() = -1;
		move_anchor_coord[i].Y() = -1;
		move_anchor_coord[i].Z() = -1;
	}
//// Iniatial ����

	m.updateDataMask(MeshModel::MM_ALL);

	// ���� fixed_anchor.txt �ļ�����Ϊ�̶�ê��
	int tmp;
	ifstream fix_a;
	fix_a.open("C:\\Users\\Administrator\\Desktop\\fixed_anchor.txt");
	while (fix_a >> tmp)
		fixed_anchor_idx.push_back(tmp);
	fix_a.close();

	//// ���� move_anchor.txt �ļ�����Ϊ�ƶ�ê��
	//ifstream move_a;
	//move_a.open("C:\\Users\\Administrator\\Desktop\\move_anchor.txt");
	//while (move_a >> tmp)
	//	move_anchor_idx.push_back(tmp);
	//move_a.close();

	// ���� move_anchor_coord.txt �ļ�����Ϊ�ƶ�ê��������� idx x y z

	ifstream move_coord;
	move_coord.open("C:\\Users\\Administrator\\Desktop\\move_anchor_coord.txt");
	while (move_coord >> tmp)
	{
		move_anchor_idx.push_back(tmp);
		move_coord >> move_anchor_coord[tmp].X();
		move_coord >> move_anchor_coord[tmp].Y();
		move_coord >> move_anchor_coord[tmp].Z();
		//if (fabs(m.cm.vert[tmp].P().X() - move_anchor_coord[tmp].X()) > 0.001
		//	&& fabs(m.cm.vert[tmp].P().Y() - move_anchor_coord[tmp].Y()) > 0.001
		//	&& fabs(m.cm.vert[tmp].P().Z() - move_anchor_coord[tmp].Z()) > 0.001)
		//{
		//	printf("m.cm.vert[%d].P() = (%f, %f, %f)\n", tmp, m.cm.vert[tmp].P().X(), m.cm.vert[tmp].P().Y(), m.cm.vert[tmp].P().Z());
		//	printf("move_anchor_coord[%d] = (%f, %f, %f)\n", tmp, move_anchor_coord[tmp].X(), move_anchor_coord[tmp].Y(), move_anchor_coord[tmp].Z());
		//	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
		//}
	}
	move_coord.close();
	
	//int cnt_d = 0;
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	//{
	//	if (vi->IsD())
	//	{
	//		++cnt_d;
	//		printf("���Ѿ���ɾ��\n");
	//	}
	//	else
	//	{
	//		int t = vi - m.cm.vert.begin();
	//		if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), t) != fixed_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Blue);
	//		else if (find(move_anchor_idx.begin(), move_anchor_idx.end(), t) != move_anchor_idx.end())
	//		{
	//			//cout << t << endl;
	//			//cout << m.cm.vert[t].P().Y() << endl;
	//			//cout << move_anchor_coord[t].Y() << endl << endl;

	//			vi->C() = vcg::Color4b(vcg::Color4b::Red);
	//		}
	//	}
	//}
	//printf("\n\n�� %d ���㱻ɾ��!\n", cnt_d);
	printf("�� %d ���㱻ѡΪ�̶�ê�㣨��ɫ��ʾ��!\n", fixed_anchor_idx.size());
	printf("�� %d ���㱻ѡΪ�ƶ�ê�㣨��ɫ��ʾ��!\n", move_anchor_idx.size());

	// ����Laplace�α�
	LaplaceDeformation(m);

	// �����û�� nan
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); vi++)
	//	printf("%d - (%f, %f, %f)\n", vi - m.cm.vert.begin(), vi->P().X(), vi->P().Y(), vi->P().Z());

////// ����update����
	gla->mvc()->sharedDataContext()->meshInserted(m.id());
	MLRenderingData dt;
	gla->mvc()->sharedDataContext()->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	suggestedRenderingData(m, dt);
	MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	gla->mvc()->sharedDataContext()->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	gla->update();
////// ����update����

///////////// ��������ָ�벻��ʹ����������// 2018.8.17���ϱߵĴ�����Ը���ģ�ͣ������±ߵ���β���
	//ctx->meshInserted(m.id());
	//MLRenderingData dt;
	//ctx->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	//// ��Ҫ�Լ�ʵ�ֵĴ��麯��
	//suggestedRenderingData(m, dt);
	//MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	//ctx->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	//gla->update();
///////////// ��������ָ�벻��ʹ����������

	return true;
}

void EditLaplaceDeformationPlugin::EndEdit(MeshModel &/*m*/, GLArea * /*parent*/, MLSceneGLSharedDataContext* /*cont*/)
{
	haveToPick = false;
	pickmode = 0; // 0 face 1 vertex
	curFacePtr = 0;
	curVertPtr = 0;
	pIndex = 0;
}

void EditLaplaceDeformationPlugin::LaplaceDeformation(MeshModel& m)
{
	long t1 = GetTickCount();

	toCaculateAdjacentVertices(&m.cm);
	printf("\nVertices = %d\n", Vertices.size());
	printf("Faces = %d\n\n", Faces.size());

	get_LsTLs_Matrix();
	printf("LsTLs ����������\n");
	get_LsTb_Matrix();
	printf("LsTb ����������, ͨ��cholesky�ֽ⣬�����Է����� LsTLs * x = LsTb\n");

	// ͨ��cholesky�ֽ⣬�����Է����� Ax = b���� LsTLs * x = LsTb
	// Cholesky�ֽ��ǰ�һ���Գ�����(symmetric, positive definite)�ľ����ʾ��һ�������Ǿ��� L ����ת�� LT �ĳ˻��ķֽ�
	// ��Ҫͷ�ļ� #include <Eigen/Cholesky>
	vx_new = LsTLs.ldlt().solve(LsTbx);	// ��֪��llt().solve()Ϊʲô�����nan
	//vy_new = LsTLs.ldlt().solve(LsTby);
	//vz_new = LsTLs.ldlt().solve(LsTbz);

	printf("�µ�����������\n");

	setNewCoord(m);
	printf("ģ������������\n");
	tri::UpdateBounding<CMeshO>::Box(m.cm);
	tri::UpdateNormal<CMeshO>::PerVertexNormalizedPerFaceNormalized(m.cm);

	long t2 = GetTickCount();
	long time_used = (t2 - t1) * 1.0 / 1000;
	int minutes = time_used / 60;
	int seconds = time_used % 60;
	printf("\nLaplace�α���ɣ���ʱ: %dm %ds\n\n", minutes, seconds);
}

void EditLaplaceDeformationPlugin::toCaculateAdjacentVertices(CMeshO* cm)
{
	CMeshO::VertexIterator vi;
	CMeshO::FaceIterator fi;

	for (vi = cm->vert.begin(); vi != cm->vert.end(); ++vi)
		Vertices.push_back(vi->P());

	for (fi = cm->face.begin(); fi != cm->face.end(); ++fi)
		if (!(*fi).IsD())
		{
			std::vector<int> tri;
			for (int k = 0; k < (*fi).VN(); k++)
			{
				int vInd = (int)(fi->V(k) - &*(cm->vert.begin()));
				//if (cm->vert[vInd].IsD())
				//	printf("���Ѿ���ɾ��");
				tri.push_back(vInd);
			}
			Faces.push_back(tri);
		}

	CalculateAdjacentVertices(cm);

	// ������е�Ľ�������
	/*for (int i = 0; i < AdjacentVertices.size(); i++)
		printf("�� %d ������ %d ������\n", i, AdjacentVertices[i].size());*/

	// ������е���Χ�������
	//for (int i = 0; i < AdjacentVertices.size(); i++)
	//	for (int j = 0; j < AdjacentVertices[i].size(); j++)
	//		printf("AdjacentVertices[%d][%d] = %d\n", i, j, AdjacentVertices[i][j]);
}

void EditLaplaceDeformationPlugin::CalculateAdjacentVertices(CMeshO* cm)
{
	AdjacentVertices.resize(Vertices.size());
	for (size_t i = 0; i < Faces.size(); i++)
	{
		std::vector<int>& t = Faces[i];
		//printf("t0 = %d, t[1] = %, t[2] = %d\n", t[0], t[1], t[2]);
		std::vector<int>& p0list = AdjacentVertices[t[0]];
		std::vector<int>& p1list = AdjacentVertices[t[1]];
		std::vector<int>& p2list = AdjacentVertices[t[2]];

		CMeshO::VertexIterator vi0 = cm->vert.begin() + t[0];
		CMeshO::VertexIterator vi1 = cm->vert.begin() + t[1];
		CMeshO::VertexIterator vi2 = cm->vert.begin() + t[2];
		if (!vi1->IsD() && std::find(p0list.begin(), p0list.end(), t[1]) == p0list.end())
			p0list.push_back(t[1]);
		if (!vi2->IsD() && std::find(p0list.begin(), p0list.end(), t[2]) == p0list.end())
			p0list.push_back(t[2]);
		if (!vi0->IsD() && std::find(p1list.begin(), p1list.end(), t[0]) == p1list.end())
			p1list.push_back(t[0]);
		if (!vi2->IsD() && std::find(p1list.begin(), p1list.end(), t[2]) == p1list.end())
			p1list.push_back(t[2]);
		if (!vi0->IsD() && std::find(p2list.begin(), p2list.end(), t[0]) == p2list.end())
			p2list.push_back(t[0]);
		if (!vi1->IsD() && std::find(p2list.begin(), p2list.end(), t[1]) == p2list.end())
			p2list.push_back(t[1]);
	}
}

void EditLaplaceDeformationPlugin::get_LsTLs_Matrix()
{
	int fix_anchors = fixed_anchor_idx.size();
	int move_anchors = move_anchor_idx.size();
	int points_num = Vertices.size();
	L = MatrixXf(points_num, points_num);
	Ls = MatrixXf(points_num + fix_anchors + move_anchors, points_num);

	for (int i = 0; i < points_num; i++)
	{
		for (int j = 0; j < points_num; j++)
		{
			if (i == j)
			{
				//printf("�� %d ������ %d ������\n", i, AdjacentVertices[i].size());
				L(i, j) = AdjacentVertices[i].size();
				Ls(i, j) = AdjacentVertices[i].size();
				continue;
			}

			if (std::find(AdjacentVertices[i].begin(), AdjacentVertices[i].end(), j) != AdjacentVertices[i].end())
			{
				L(i, j) = -1;
				Ls(i, j) = -1;
			}
			else
			{
				L(i, j) = 0;
				Ls(i, j) = 0;
			}
		}
	}

	// �̶�ê��
	for (int i = 0; i < fix_anchors; i++)
		for (int j = 0; j < points_num; j++)
		{
			if (j == fixed_anchor_idx[i])
				Ls(points_num + i, j) = 1;
			else
				Ls(points_num + i, j) = 0;
		}

	// �ƶ�ê��
	for (int i = 0; i < move_anchor_idx.size(); i++)
		for (int j = 0; j < points_num; j++)
		{
			if (j == move_anchor_idx[i])
				Ls(points_num + fix_anchors + i, j) = 1;
			else
				Ls(points_num + fix_anchors + i, j) = 0;
		}

	//printf("Laplace���� - L\n");
	//cout << L << endl << endl;

	//printf("�����ê����Ϣ��Laplace���� - Ls\n");
	//cout << Ls << endl << endl;

	printf("Laplace ����������\n");
	// ����̫���˴�ӡ������������ʲô����
	// ���һ��
	//for (int i = 0; i < points_num; i++)
	//{
	//	int cnt = 0;
	//	int tmp = 0;
	//	for (int j = 0; j < points_num; j++)
	//	{
	//		if (Ls(i, j) == -1)
	//			++cnt;
	//		else if (Ls(i, j) != 0)
	//			tmp = Ls(i, j);
	//	}
	//	if (cnt != tmp)
	//		printf("Error!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	//}


	LsT = Ls.transpose();
	LsTLs = LsT * Ls;

	// �鿴������Ϣ
	//printf("\nLsTLs\n");
	//cout << LsTLs.rows() << "  " << LsTLs.cols() << endl;
	//cout << LsTLs << endl;
	//printf("\n");
}

void EditLaplaceDeformationPlugin::get_LsTb_Matrix()
{
	int fix_anchors = fixed_anchor_idx.size();
	int move_anchors = move_anchor_idx.size();
	int points_num = Vertices.size();
	VectorXf vx(points_num), vy(points_num), vz(points_num);

	for (int i = 0; i < points_num; i++)
	{
		vx[i] = Vertices[i].X();
		vy[i] = Vertices[i].Y();
		vz[i] = Vertices[i].Z();
	}

	// ����laplace�����������е�ĵ�laplace����
	bx = L * vx;	// �˼����Ǽ���
	by = L * vy;
	bz = L * vz;

	bx.conservativeResize(points_num + fix_anchors + move_anchors);
	by.conservativeResize(points_num + fix_anchors + move_anchors);
	bz.conservativeResize(points_num + fix_anchors + move_anchors);

	// ���α�ǰ����Թ̶�ê��������и�ֵ
	for (int i = 0; i < fix_anchors; i++)
	{
		bx[i + points_num] = Vertices[fixed_anchor_idx[i]].X();
		by[i + points_num] = Vertices[fixed_anchor_idx[i]].Y();
		bz[i + points_num] = Vertices[fixed_anchor_idx[i]].Z();
	}

	// ���α��������ƶ�ê��������и�ֵ
	for (int i = 0; i < move_anchors; i++)
	{
		bx[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].X();
		//cout << Vertices[move_anchor_idx[i]].Y() << endl;
		//cout << move_anchor_coord[move_anchor_idx[i]].Y() << endl << endl;
		by[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].Y();
		//cout << Vertices[move_anchor_idx[i]].Y() << endl;
		//cout << move_anchor_coord[move_anchor_idx[i]].Y() << endl << endl;
		bz[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].Z();
	}

	//cout << bx << endl << endl;
	//cout << by << endl << endl;

	// �����������ϵ� LsTb ����
	LsTbx = LsT * bx;
	LsTby = LsT * by;
	LsTbz = LsT * bz;

	//cout << LsTbx << endl << endl;
}

void EditLaplaceDeformationPlugin::setNewCoord(MeshModel& m)
{
	for (int i = 0; i < m.cm.vert.size(); i++)
	{
		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), i) != move_anchor_idx.end())
		{
			m.cm.vert[i].P().X() = move_anchor_coord[i].X();
			//m.cm.vert[i].P().Y() = move_anchor_coord[i].Y();
			//m.cm.vert[i].P().Z() = move_anchor_coord[i].Z();
		}
		else
		{
			if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), i) == fixed_anchor_idx.end())
			{
				m.cm.vert[i].P().X() = vx_new[i];
				//m.cm.vert[i].P().Y() = vy_new[i];
				//m.cm.vert[i].P().Z() = vz_new[i];
			}
		}
	}
}

void EditLaplaceDeformationPlugin::suggestedRenderingData(MeshModel &m, MLRenderingData& dt)
{
	if (m.cm.VN() == 0)
		return;
	MLRenderingData::PRIMITIVE_MODALITY pr = MLRenderingData::PR_SOLID;
	if (m.cm.FN() > 0)
		pr = MLRenderingData::PR_SOLID;

	MLRenderingData::RendAtts atts;
	atts[MLRenderingData::ATT_NAMES::ATT_VERTPOSITION] = true;
	atts[MLRenderingData::ATT_NAMES::ATT_VERTCOLOR] = true;
	atts[MLRenderingData::ATT_NAMES::ATT_VERTNORMAL] = true;
	atts[MLRenderingData::ATT_NAMES::ATT_FACENORMAL] = true;
	atts[MLRenderingData::ATT_NAMES::ATT_FACECOLOR] = true;

	MLPerViewGLOptions opts;
	dt.get(opts);
	opts._sel_enabled = true;
	opts._face_sel = true;
	opts._vertex_sel = true;
	dt.set(opts);

	dt.set(pr, atts);
}

