
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
//#include "precise.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PI 3.141592653

// CDemo_ClipView_VCDlg �Ի���

#define CANVAS_WIDTH	800	
#define CANVAS_HEIGHT	600
#define INFO_HEIGHT		50
#define TESTDATA_XML1  "TestData1.xml"
#define TESTDATA_XML2  "TestData2.xml"

//���Ϊ0����������Ҳ�������Է����������
#define TEST_LINES 1
#define TEST_CIRCLES 1

//�Ƿ�����ʼ���ݺͲü������ʹ��0���������
#define TEST_DRAW_INITIAL 1
#define TEST_DRAW_ANSWER 1

#define MAX_THREAD_NUMBER 16

//double��ȿɽ��ܾ���
#define DOUBLE_DEGREE 0.01

//�̼߳����
#define THREAD_CHECK_INTERVAL 5
//��С�Ļ�ͼ��С
#define MINIMUM_DRAW_SIZE 100
//����ڴ�й©
#define _CRTDBG_MAP_ALLOC


struct Vector
{
	double x,y;
};
struct XPoint{
	double x;
	double y;
};
struct ThreadTime
{
	int i;
	double time;
};
struct  _param   {   //����ṹ�����ڱ���ÿ��������Ҫ����Բ���ߡ�����ε�
	vector<Line> thread_lines;
	vector<Circle> thread_circles;
	Boundary boundary;
	int i;
};

struct  _arc2draw   {  //����ṹ�����ڱ���һ����Ҫ���Ļ���
	CRect rect;
	CPoint start_point;
	CPoint end_point;
};

struct IntersectPoint
{
public:
	bool isIntoPoly;
	int contexPoint;
	double t;
	CPoint point;
};


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
	double startTime;
	double endTime;
	BOOL hasOutCanvasData;
	CList<int> beginTIdList;

	void ClearTestCaseData();
	BOOL LoadTestCaseData(CString xmlPath, CString caseID);
	void DrawTestCase(CString xmlPath, CString caseID);
	void BeginMonitor();
	void EndMonitor();
	BOOL XmlNodeToPoint(pugi::xml_node node, CPoint& piont);
	int  SplitCStringToArray(CString str,CStringArray& strArray, CString split);
	BOOL IsPointOutCanvas(CPoint point);
	BOOL IsLineOutCanvas(Line line);
	BOOL IsCircleOutCanvas(Circle circle);
	BOOL IsBoundaryOutCanvas(Boundary boundary);
	BOOL GetThreadIdList(CList<int>& tIdList);

public:
	void DrawLine(Line line, COLORREF clr);
	void DrawCircle(Circle circle, COLORREF clr);
	void DrawBoundary(Boundary boundary, COLORREF clr);
	static DWORD WINAPI ThreadProc2(LPVOID lpParam);


	static const int INTIALIZE=0;
	//static const double ESP;


	static vector<ThreadTime> thread_use_time;						//��¼ÿ���߳�������ʱ��
	static double startclock;										//��ʼ�ü�ʱ��clockֵ




	static vector<Line> lines;                                     //���ڴ洢��ȡ����������
	static vector<Circle> circles;									//���ڴ洢��ȡ��������Բ
	static Boundary boundary;										//���ڴ洢��ȡ���Ķ���δ���

	static int circle_in_bound;									//Բ�ڶ�����ڵ�����
	static int circle_inter_bound;								//Բ�������ཻ������
	static int line_in_bound;
	static int line_out_bound;
	static int line_overlap_bound;

	//���ڴ洢��Ҫ�����ߵ�ȫ�ֱ���������
	static vector<Line> lines_to_draw[MAX_THREAD_NUMBER];			//���ڴ洢����Ҫ������
	static vector<_arc2draw> circles_to_draw[MAX_THREAD_NUMBER];    //���ڴ洢����Ҫ���Ļ���
	static vector<Line> lines_drawing;								//���ڴ洢���ڻ�����
	static vector<_arc2draw> circles_drawing;						//���ڴ洢���ڻ���Բ

	static CRITICAL_SECTION critical_sections[MAX_THREAD_NUMBER];	//Ϊÿ���̷߳���һ���ٽ���
	static CRITICAL_SECTION critical_circle_number[2];				//ΪԲ�ڶ�����ڲ����ཻ�������������ٽ���
	static CRITICAL_SECTION critical_line_number[3];				//Ϊ���ڶ�����ڲ����ཻ�������������ٽ���
	static CRITICAL_SECTION critical_thread_number[2];		//Ϊ���ڶ�����ڲ����ཻ�������������ٽ���


	static bool isConvexPoly;										//����εİ�͹�ԣ���ѡ��ͬ���㷨
	static vector<BOOL> convexPoint;								//����εĵ�İ�͹�ԣ�trueΪ͹��
	static vector<RECT> edgeRect;									//����εıߵ�����ο�
	static vector<Vector> normalVector;								//����εıߵ��ڷ�����
  

	//dqc methods begins
	static void preprocessJudgeConvexPoint(vector<BOOL>&);
	static void preprocessNormalVector(vector<Vector>&,vector<BOOL>&);
	static void preprocessEdgeRect(const Boundary);
	static inline bool isPointInLine(CPoint& p,CPoint& p1,CPoint& p2,int i);
	static bool isOverlap(Line l);
	static inline int crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2);
	static void dealConcave(vector<Line>&,Boundary&,int);
	static bool Compare(IntersectPoint,IntersectPoint);
	//dqc methods ends




	//gs methods begins	
	static void dealConvex(vector<Line>&,Boundary&,int);
	static bool InBox(Line&);
	static int Multinomial(int,int,int,int,int,int);
	static CPoint CrossPoint(Line&,Line&);
	static bool IsOnline(CPoint&,Line&);
	static bool Intersect(CPoint&,CPoint&,Line&);
	static Line result(Line&);

	//gs methods ends

	//xh methods begins
	static void getInterpointArray(vector<XPoint>&,int,vector<Circle>&);
	static struct XPoint getInterpoint(double,int,int,int,int);
	static bool isPointInBoundary(XPoint&);
	static bool isPointInBoundary(CPoint&);
	static XPoint getMiddlePoint(vector<XPoint>&,int,int,vector<Circle>&);
	static double getAngle(double,double,double,double,double);
	static void forCircleRun(vector<Circle>&,Boundary&,int);
	//xh methods ends

};
		
    

