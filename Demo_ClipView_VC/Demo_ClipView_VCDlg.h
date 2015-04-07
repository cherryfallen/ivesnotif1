
// Demo_ClipView_VCDlg.h : ͷ�ļ�
//
#pragma once
#include <vector>
using std::vector;
#include "pugiconfig.hpp"
#include "pugixml.hpp"
#include "GraphicEntity.h"
#include <time.h>
#include <Windows.h>
#include <Psapi.h>
#include "afxwin.h"
#include <algorithm>
#pragma comment(lib,"psapi.lib")


struct r_lineNum
{
	CPoint point;
	int num_line;
};
struct Vector
{
	double x,y;
};

const int THREAD_NUMBER=8;

// CDemo_ClipView_VCDlg �Ի���
class CDemo_ClipView_VCDlg : public CDialogEx
{
// ����
public:
	CDemo_ClipView_VCDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DEMO_CLIPVIEW_VC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CButton m_btn_clip;
	CStatic m_stc_drawing;
	CStatic m_stc_info1;
	CStatic m_stc_info2;
	afx_msg void OnBnClickedBtnClip();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CString curPath;
	CStringArray complexArray;
	double startTime;
	double endTime;
	double startMemory;
	double endMemory;
	BOOL hasOutCanvasData;

	void ClearTestCaseData();
	BOOL LoadTestCaseData(CString xmlPath, CString caseID);
	void DrawTestCase(CString xmlPath, CString caseID);
	void BeginTimeAndMemoryMonitor();
	void EndTimeAndMemoryMonitor();
	BOOL XmlNodeToPoint(pugi::xml_node node, CPoint& piont);
	int  SplitCStringToArray(CString str,CStringArray& strArray, CString split);
	BOOL IsPointOutCanvas(CPoint point);
	BOOL IsLineOutCanvas(Line line);
	BOOL IsCircleOutCanvas(Circle circle);
	BOOL IsBoundaryOutCanvas(Boundary boundary);

public:
	vector<Line> lines;
	vector<Circle> circles;
	Boundary boundary;
	void DrawLine(Line line, COLORREF clr);
	void DrawCircle(Circle circle, COLORREF clr);
	void DrawBoundary(Boundary boundary, COLORREF clr);
	static DWORD WINAPI ThreadProc(LPVOID lpParam);

public:
	//dqc methods begins
	void JudgeConvexPoint(vector<BOOL>&);
	void ComputeNormalVector(vector<Vector>&,vector<BOOL>&);
	//vector<BOOL> convexPoint;//����εĵ�İ�͹�ԣ�trueΪ͹��
	//vector<RECT> edgeRect;//����εıߵ�����ο�
	//vector<IntersectPoint> intersectPoint;//�߶ε����н��㣨Ҳ�����߶ε�������յ㣩
	int CrossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2);
	static void dealConcave(vector<Line>&,Boundary&,CDemo_ClipView_VCDlg*);
	//dqc methods ends

	//gs methods begins	
	static void dealConvex(vector<Line>&,Boundary&,CDemo_ClipView_VCDlg*);
	bool InBox(Line&);
	int Multinomial(int,int,int,int,int,int);
	CPoint CrossPoint(Line&,Line&);
	bool IsOnline(CPoint&,Line&);
	bool Intersect(CPoint&,CPoint&,Line&);
	Line result(Line&);

	//gs methods ends

	//xh methods begins
	void getInterpointArray(vector<r_lineNum>&,int,vector<Circle>&);
	struct r_lineNum getInterpoint(double,int,int,int,int,int);
	bool isPointInBoundary(CPoint&);
	CPoint getMiddlePoint(vector<r_lineNum>&,int,int,vector<Circle>&);
	double getAngle(long,long,long,long,double);
	static void forCircleRun(vector<Circle>&,Boundary&,CDemo_ClipView_VCDlg*);
	//xh methods ends
};
