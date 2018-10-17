#pragma once

#include <vector>
#include <QObject>
#include <common/interfaces.h>
#include <vcg/space/deprecated_point3.h>

#include <eigenlib/Eigen/Dense>

class EditLaplaceDeformationPlugin : public QObject, public MeshEditInterface
{
	Q_OBJECT
	Q_INTERFACES(MeshEditInterface)
		
public:
	Eigen::MatrixXf L, Ls, LsT, LsTLs, LsTbx, LsTby, LsTbz;	// L: ԭʼlaplace���󣨷��󣩣�LS: ������ê�㣨point_num + anchors, point_num����С�� T��ת��
	Eigen::VectorXf bx, by, bz;
	Eigen::VectorXf vx_new, vy_new, vz_new;

	std::vector<int> fixed_anchor_idx, move_anchor_idx;	// ê���Ϊ�̶�����ƶ���
	std::vector<vcg::Point3f> move_anchor_coord;		// �α���ƶ�ê��λ��
	std::vector<vcg::Point3f> Vertices;
	std::vector<std::vector<int>> Faces;
	std::vector<std::vector<int>> AdjacentVertices;

	EditLaplaceDeformationPlugin();
    virtual ~EditLaplaceDeformationPlugin() {}

    static const QString Info();

    bool StartEdit(MeshModel &/*m*/, GLArea * /*parent*/, MLSceneGLSharedDataContext* /*cont*/);
    void EndEdit(MeshModel &/*m*/, GLArea * /*parent*/, MLSceneGLSharedDataContext* /*cont*/);
	void Decorate(MeshModel &/*m*/, GLArea * /*parent*/, QPainter *p) {};
	void Decorate (MeshModel &/*m*/, GLArea * ){};
	void mousePressEvent(QMouseEvent *, MeshModel &, GLArea * ) {};
	void mouseMoveEvent(QMouseEvent *, MeshModel &, GLArea * ) {};
	void mouseReleaseEvent(QMouseEvent *event, MeshModel &/*m*/, GLArea *) {};
	void keyReleaseEvent(QKeyEvent *, MeshModel &, GLArea *) {};
	void drawFace(CMeshO::FacePointer fp, MeshModel &m, GLArea *gla, QPainter *p) {};
	void drawVert(CMeshO::VertexPointer vp, MeshModel &m, GLArea *gla, QPainter *p) {};


	// GY
	void LaplaceDeformation(MeshModel&);	// ���ε����±ߺ���

	void toCaculateAdjacentVertices(CMeshO* cm);
	void CalculateAdjacentVertices(CMeshO* cm);

	void get_LsTLs_Matrix();	// ��ȡlaplace�����Լ�LTLILT����
	void get_LsTb_Matrix();		// ��ȡ b ����x' = LTLILT * b

	void setNewCoord(MeshModel&);	// ����ģ�����꣬anchor�㵥�������α����

	void suggestedRenderingData(MeshModel &, MLRenderingData& dt);
	
	void ChimneyRotate(MeshModel &);

private:
    QPoint cur;
	QFont qFont;
    bool haveToPick;
	int pickmode;
	CMeshO::FacePointer   curFacePtr;
	CMeshO::VertexPointer curVertPtr;
	std::vector<CMeshO::FacePointer>   NewFaceSel;
	std::vector<CMeshO::VertexPointer> NewVertSel;
	int pIndex;
	GLArea * gla;
};
