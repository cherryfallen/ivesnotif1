
// Demo_ClipView_VCDlg.cpp : ʵ���ļ�

#include "stdafx.h"
#include "Demo_ClipView_VC.h"
#include "Demo_ClipView_VCDlg.h"
#include "afxdialogex.h"
#include <TlHelp32.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <process.h>
#include <omp.h>
#include <stdio.h>
using namespace std;



/*---------------------------------------------- main part begins------------------------------------------------------------------*/

int CDemo_ClipView_VCDlg::circle_in_bound = 0;									//Բ�ڶ�����ڵ�����
int CDemo_ClipView_VCDlg::circle_inter_bound = 0;								//Բ�������ཻ������
int CDemo_ClipView_VCDlg::line_in_bound = 0;
int CDemo_ClipView_VCDlg::line_out_bound = 0;
int CDemo_ClipView_VCDlg::line_overlap_bound = 0;

vector<Line>CDemo_ClipView_VCDlg::lines_to_draw[MAX_THREAD_NUMBER];				//���ڴ洢����Ҫ������
vector<_arc2draw>CDemo_ClipView_VCDlg::circles_to_draw[MAX_THREAD_NUMBER];		//���ڴ洢����Ҫ���Ļ���
vector<Line>CDemo_ClipView_VCDlg::lines_drawing;								//���ڴ洢���ڻ�����
vector<_arc2draw>CDemo_ClipView_VCDlg::circles_drawing;							//���ڴ洢���ڻ���Բ
vector<Line>CDemo_ClipView_VCDlg::lines;										//���ڴ洢��ȡ����������
vector<Circle>CDemo_ClipView_VCDlg::circles;									//���ڴ洢��ȡ��������Բ
Boundary CDemo_ClipView_VCDlg::boundary;	

omp_lock_t critical_sections[MAX_THREAD_NUMBER];	//Ϊÿ���̷߳���һ���ٽ���
omp_lock_t critical_circle_number[2];				//ΪԲ�ڶ�����ڲ����ཻ�������������ٽ���
omp_lock_t critical_line_number[3];					//Ϊ���ڶ�����ڲ����ཻ�������������ٽ���

bool CDemo_ClipView_VCDlg::isConvexPoly;										//����εİ�͹�ԣ���ѡ��ͬ���㷨
vector<BOOL>CDemo_ClipView_VCDlg::convexPoint;									//����εĵ�İ�͹�ԣ�trueΪ͹��
vector<RECT>CDemo_ClipView_VCDlg::edgeRect;										//����εıߵ�����ο�
vector<Vector>CDemo_ClipView_VCDlg::normalVector;


/*--------------------------------------------- methods part begins------------------------------------------------------------------*/
CDemo_ClipView_VCDlg::CDemo_ClipView_VCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDemo_ClipView_VCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemo_ClipView_VCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLIP, m_btn_clip);
	DDX_Control(pDX, IDC_STATIC_DRAWING, m_stc_drawing);
	DDX_Control(pDX, IDC_STATIC_INFO_1, m_stc_info1);
	DDX_Control(pDX, IDC_STATIC_INFO_2, m_stc_info2);
}

BEGIN_MESSAGE_MAP(CDemo_ClipView_VCDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLIP, &CDemo_ClipView_VCDlg::OnBnClickedBtnClip)
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()


BOOL CDemo_ClipView_VCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��
	CRect clientRect;
	GetClientRect(&clientRect);
	CRect windowRect;
	GetWindowRect(&windowRect);
	int width = CANVAS_WIDTH + windowRect.Width() - clientRect.Width();
	int height = CANVAS_HEIGHT + INFO_HEIGHT + windowRect.Height() - clientRect.Height() + 10;
	SetWindowPos(&wndTopMost, 10, 10, width, height, SWP_NOZORDER);

	m_btn_clip.SetWindowPos(NULL, 50, CANVAS_HEIGHT + 15, 80, 30, SWP_NOZORDER);
	m_stc_drawing.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 20, 200, 20, SWP_NOZORDER);
	m_stc_info1.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 5, 450, 20, SWP_NOZORDER);
	m_stc_info2.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 25, 650, 40, SWP_NOZORDER);

	hasOutCanvasData = FALSE;
	GetModuleFileName(NULL,curPath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
	curPath.ReleaseBuffer();
	curPath = curPath.Left(curPath.ReverseFind('\\') + 1);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CDemo_ClipView_VCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CDemo_ClipView_VCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CDemo_ClipView_VCDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg=LOWORD(wParam);
	switch(uMsg)
	{
	case ID_TESTCASE1:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "1");
		}
		break;
	case ID_TESTCASE2:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "2");
		}
		break;
	case ID_TESTCASE3:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "3");
		}
		break;
	case ID_TESTCASE4:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "4");
		}
		break;
	case ID_TESTCASE5:
		{
			CString xmlPath = curPath + TESTDATA_XML2;
			DrawTestCase(xmlPath, "5");
		}
		break;
	default:
		break;
	}
	return CDialogEx::OnCommand(wParam, lParam);
}

void CDemo_ClipView_VCDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (HTCAPTION == nHitTest) {
		return;
	}
	CDialogEx::OnNcLButtonDown(nHitTest, point);
}

void CDemo_ClipView_VCDlg::OnBnClickedBtnClip()
{
	BeginMonitor();
	//TODO �ڴ˴���ɲü��㷨�Ͳü�����ʾ���򣬲��޸���Ҫ��ʾ��ͼ����Ϣ

	//��ʼ ��ʼ�������Ϣ
	CDemo_ClipView_VCDlg::circle_in_bound=0;
	CDemo_ClipView_VCDlg::circle_inter_bound=0;
	CDemo_ClipView_VCDlg::line_in_bound=0;
	CDemo_ClipView_VCDlg::line_out_bound=0;
	CDemo_ClipView_VCDlg::line_overlap_bound=0;
	//���� ��ʼ�������Ϣ

	//��ʼ ��¼ͼ����Ϣ
	int lines_num = lines.size();
	int circles_num = circles.size();
	int boundarys_num = 1;
	//���� ��¼ͼ����Ϣ

	//��ʼ ����boundary��������������ͬ���쳣������Ƚ��д���������쳣���
	vector<CPoint>::iterator iter;
	for (iter = CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1; iter != CDemo_ClipView_VCDlg::boundary.vertexs.end();)
	{
		if ((*iter).x ==(*(iter-1)).x&&(*iter).y ==(*(iter-1)).y)
			iter = CDemo_ClipView_VCDlg::boundary.vertexs.erase(iter);
		else iter++;
	}
	CDemo_ClipView_VCDlg::boundary.vertexs[0].x=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].x;
	CDemo_ClipView_VCDlg::boundary.vertexs[0].y=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].y;
	//���� ����boundary��������������ͬ���쳣������Ƚ��д���������쳣���

	//��ʼ ����boundary���������㹲�ߵ����
	for (iter = CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1; iter != CDemo_ClipView_VCDlg::boundary.vertexs.end();)
	{
		CPoint p1,p2,p3;
		if (iter==CDemo_ClipView_VCDlg::boundary.vertexs.end()-1)
		{
			p1=*(iter-1);
			p2=*(iter);
			p3=*(CDemo_ClipView_VCDlg::boundary.vertexs.begin()+1);
		}
		else
		{
			p1=*(iter-1);
			p2=*iter;
			p3=*(iter+1);
		}

		if ((p2.x-p1.x)*(p3.y-p2.y)==(p3.x-p2.x)*(p2.y-p1.y))//���㹲��
			iter = CDemo_ClipView_VCDlg::boundary.vertexs.erase(iter);
		else
			iter++;
	}
	CDemo_ClipView_VCDlg::boundary.vertexs[0].x=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].x;
	CDemo_ClipView_VCDlg::boundary.vertexs[0].y=CDemo_ClipView_VCDlg::boundary.vertexs[CDemo_ClipView_VCDlg::boundary.vertexs.size()-1].y;
	//���� ����boundary���������㹲�ߵ����


	//��ʼ ����CPU����ȷ�������߳���
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	int THREAD_NUMBER=info.dwNumberOfProcessors-1;
	if (THREAD_NUMBER>MAX_THREAD_NUMBER-1)
		THREAD_NUMBER=MAX_THREAD_NUMBER-1;
	if (THREAD_NUMBER<1)
		THREAD_NUMBER=1;
	//���� ����CPU����ȷ�������߳���

	//��ʼ �����㹻�Ŀռ䣬����֮��ռ䲻��ʱ�����ڴ濽��
	for (int i=0;i<THREAD_NUMBER;i++)
	{
		CDemo_ClipView_VCDlg::lines_to_draw[i].reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
		CDemo_ClipView_VCDlg::circles_to_draw[i].reserve(CDemo_ClipView_VCDlg::circles.size()*2/THREAD_NUMBER);
	}
	CDemo_ClipView_VCDlg::lines_drawing.reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
	CDemo_ClipView_VCDlg::circles_drawing.reserve(CDemo_ClipView_VCDlg::lines.size()*2/THREAD_NUMBER);
	//���� �����㹻�Ŀռ䣬����֮��ռ䲻��ʱ�����ڴ濽��

	//��ʼ ��ʼ���ٽ���Դ
	for (int i=0;i<THREAD_NUMBER;i++)
		omp_init_lock(&critical_sections[i]);
	for (int i=0;i<2;i++)
		omp_init_lock(&critical_circle_number[i]);
	for (int i=0;i<3;i++)
		omp_init_lock(&critical_line_number[i]);

	//���� ��ʼ���ٽ���Դ

	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));		//�����������Ϊ��ɫ

	COLORREF clrBoundary = RGB(255,0,0);								//�߽����ɫΪ��ɫ
	COLORREF clrLine = RGB(0,255,0);									//�ߵ���ɫΪ��ɫ
	COLORREF clrCircle = RGB(0,0,255);									//Բ����ɫΪ��ɫ
	
	//��ʼ Ϊ���ߺͻ�Բ�ֱ�����һ��Cpen
	CPen penLine,penCircle;												
	penLine.CreatePen(PS_SOLID, 1, clrLine);
	penCircle.CreatePen(PS_SOLID, 1, clrCircle);
	//���� Ϊ���ߺͻ�Բ�ֱ�����һ��Cpen

	DrawBoundary(boundary, clrBoundary);								//������α߽�


	HANDLE hThread[MAX_THREAD_NUMBER];								//���ڴ洢�߳̾��
	DWORD  dwThreadID[MAX_THREAD_NUMBER];								//���ڴ洢�̵߳�ID
	_param* Info = new _param[MAX_THREAD_NUMBER];						//���ݸ��̴߳������Ĳ���

	int nThreadLines=(int)(lines.size()/THREAD_NUMBER);					//ǰn-1���߳���Ҫ������ߵ��������߳���Ϊn��
	int nThreadCircles = (int)(circles.size()/THREAD_NUMBER);			//ǰn-1���߳���Ҫ�����Բ�ĸ������߳���Ϊn��

	//��ʼ Ϊÿ���̷߳�����Ҫ������ߺ�Բ
	for(int i=0;i<THREAD_NUMBER;i++){
		int upperNumber=(i+1)*nThreadLines;
		if (i==THREAD_NUMBER-1)
			upperNumber=CDemo_ClipView_VCDlg::lines.size();
		for (int j = i*nThreadLines; j < upperNumber; j++)
			Info[i].thread_lines.push_back(CDemo_ClipView_VCDlg::lines[j]);

		upperNumber=(i+1)*nThreadCircles;
		if (i==THREAD_NUMBER-1)
			upperNumber=CDemo_ClipView_VCDlg::circles.size();
		for (int j = i*nThreadCircles; j < upperNumber; j++)
			Info[i].thread_circles.push_back(CDemo_ClipView_VCDlg::circles[j]);
	
		Info[i].i = i;
		Info[i].boundary = CDemo_ClipView_VCDlg::boundary;
	}
	//���� Ϊÿ���̷߳�����Ҫ������ߺ�Բ


	//��ʼ �Զ���α߽����Ϣ����Ԥ����
	preprocessJudgeConvexPoint(convexPoint);
	preprocessNormalVector(normalVector,convexPoint);
	preprocessEdgeRect(boundary);
	//���� �Զ���α߽����Ϣ����Ԥ����


	//��ʼ �������߳̽��м���
	for (int i = 0;i<THREAD_NUMBER;i++)
		hThread[i] = CreateThread(NULL,0,ThreadProc2,&Info[i],0,&(dwThreadID[i]));
	//���� �������߳̽��м���

	//��ʼ ���߳�û��ȫ������ʱ���Ѿ���������ߺ�Բ���л�����ʾ
	while (WaitForMultipleObjects(THREAD_NUMBER,hThread,true,
		THREAD_CHECK_INTERVAL)!=WAIT_OBJECT_0)//���߳�û����ȫ���أ���ѭ������
	{
		if (!TEST_DRAW_ANSWER) continue;									//�����û�ͼ�����˳�����ѭ��

		//��ʼ ����		
		dc.SelectObject(&penLine);
		for (int i=0;i<THREAD_NUMBER;i++)
		{			
			if (lines_to_draw[i].size()>MINIMUM_DRAW_SIZE)					//����Ƿ�����˴������ݣ����㹻��Ļ��Ϳ�ʼ��ͼ���Խ�Լʱ��
			{
				omp_set_lock(&critical_sections[i]);				//������ٽ���Դlines_to_draw[i]���з���
				lines_drawing.swap(lines_to_draw[i]);						//������ͼ����������ʱΪ�������������ͼ����
				omp_unset_lock(&critical_sections[i]);				//�뿪���ٽ���Դlines_to_draw[i]���з���

				//��ʼ �Ե�k�߳̽����������߽��л���
				for (vector<Line>::iterator iter=lines_drawing.begin();
					iter!=lines_drawing.end(); iter++)
				{
					dc.MoveTo((*iter).startpoint);
					dc.LineTo((*iter).endpoint);
				}
				//���� �Ե�k�߳̽����������߽��л���
				lines_drawing.clear();										//��ջ�ͼ����
			}
		}
		//���� ����

		//��ʼ ��Բ
		dc.SelectObject(&penCircle);
		for (int i=0;i<THREAD_NUMBER;i++)
		{
			if (circles_to_draw[i].size()>MINIMUM_DRAW_SIZE)				//����Ƿ�����˴������ݣ����㹻��Ļ��Ϳ�ʼ��ͼ���Խ�Լʱ��
			{
				omp_set_lock(&critical_sections[i]);				//������ٽ���Դcritical_sections[i]���з���
				circles_drawing.swap(circles_to_draw[i]);					//������ͼ����������ʱΪ�������������ͼ����
				omp_unset_lock(&critical_sections[i]);				//�뿪���ٽ���Դcritical_sections[i]���з���

				//��ʼ �Ե�k�߳̽���������Բ���л���
				for (vector<_arc2draw>::iterator iter=circles_drawing.begin();iter!= circles_drawing.end();iter++)
					dc.Arc(&((*iter).rect),(*iter).start_point,(*iter).end_point);
				//���� �Ե�k�߳̽���������Բ���л���
				circles_drawing.clear();									//��ջ�ͼ����
			}
		}
		//���� ��Բ

	}
	//���� ���߳�û��ȫ������ʱ���Ѿ���������ߺ�Բ���л�����ʾ


	//��ʼ ��ʣ�µ��ߺ�Բ��
	if (TEST_DRAW_ANSWER)
	{
		dc.SelectObject(&penLine);
		for (int i=0;i<THREAD_NUMBER;i++)
			for (vector<Line>::iterator iter=lines_to_draw[i].begin();iter!=lines_to_draw[i].end(); iter++)
			{
				dc.MoveTo((*iter).startpoint);
				dc.LineTo((*iter).endpoint);
			}

		dc.SelectObject(&penCircle);
		for (int i=0;i<THREAD_NUMBER;i++)
			for (vector<_arc2draw>::iterator iter= circles_to_draw[i].begin();iter!= circles_to_draw[i].end(); iter++)
				dc.Arc(&((*iter).rect),(*iter).start_point,(*iter).end_point);
	}
	//���� ��ʣ�µ��ߺ�Բ��


	//��ʼ ����һЩ����
	int circle_out_bound = circles_num-circle_inter_bound-circle_in_bound;
	int line_inter_bound = lines_num-line_in_bound-line_out_bound-line_overlap_bound;
	//���� ����һЩ����
	

	//��ʼ ���ͼ�θ��ӳ̶�
	CString text;
	text.Format("����%d���ߣ�%d��Բ��%d���߽磬����%d��ͼ���ڶ�����ڲ���%d��ͼ���ڶ�����ⲿ������%d��ͼ�κͶ�����ཻ��%d��ͼ�κͶ�����غ�",lines_num,circles_num,boundarys_num,line_in_bound+circle_in_bound,line_out_bound+circle_out_bound,line_inter_bound+circle_inter_bound,line_overlap_bound);
	m_stc_info2.SetWindowText(text);
	//���� ���ͼ�θ��ӳ̶�


	//��ʼ �ͷ��ٽ���Դ�Ͷѿռ�
	for (int i=0;i<THREAD_NUMBER;i++)
	{
		omp_destroy_lock(&critical_sections[i]);
		vector<Line>().swap(lines_to_draw[i]);
		vector<_arc2draw>().swap(circles_to_draw[i]);
	}
	
	for (int i=0;i<3;i++)
		omp_destroy_lock(&critical_line_number[i]);
	for (int i=0;i<2;i++)
		omp_destroy_lock(&critical_circle_number[i]);

	vector<Line>().swap(lines_drawing);
	vector<_arc2draw>().swap(circles_drawing);
	vector<Line>().swap(lines);
	vector<Circle>().swap(circles);
	vector<BOOL>().swap(convexPoint);
	vector<RECT>().swap(edgeRect);
	vector<Vector>().swap(normalVector);


	for(int i=0;i<THREAD_NUMBER;i++){
		vector<Line>().swap(Info[i].thread_lines);
		vector<Circle>().swap(Info[i].thread_circles);
	}
	delete [] Info;
    Info = NULL;

	penLine.DeleteObject();
	penCircle.DeleteObject();

	//���� �ͷ��ٽ���Դ�Ͷѿռ�
	
	_CrtDumpMemoryLeaks();										//���й©��Ϣ
	
	
	EndMonitor();
}

/*
*���ܣ��Բ����ߺ�Բ�����󽻼�����̺߳���
*���������ݸ�ÿ���̵߳Ĵ�������ߺ�Բ������
*˼·���Դ��ݹ�����Բ���ȷֱ������δ��ڽ����󽻣�
*	   �����Ҫ���Ƶ��߶κͻ��ߣ�������һ�������Ƶ�������
*/
DWORD WINAPI CDemo_ClipView_VCDlg::ThreadProc2(LPVOID lpParam)  
{  
	_param  * Info = ( _param *)lpParam; 

	//��ʼ �����ߵĲü�
	if (TEST_LINES)                                                       
	{
		if (isConvexPoly)
			dealConvex(Info->thread_lines,Info->boundary,Info->i);
		else
			dealConcave(Info->thread_lines,Info->boundary,Info->i);

	}
	//���� �����ߵĲü�

	//��ʼ ����Բ�Ĳü�
	if (TEST_CIRCLES)
		forCircleRun(Info->thread_circles,Info->boundary,Info->i);
	//���� ����Բ�Ĳü�

	return 0;                                                             //�̷߳���
}






/*----------------------------------------------- ͹������߲ü��㷨 starts--------------------------------------------------------*/

/*
*���ܣ��ж��߶��Ƿ��ڰ�Χ������ڰ�Χ�����򷵻�true�����򷵻�false
*������boundaryΪ�ü��߽�  lineΪ���жϵ��߶�
*˼·����������εĶ��㣬����С��x,y������x,yֵ�ֱ�Ϊ��Χ�еĶԽ��߶˵�
*/
bool CDemo_ClipView_VCDlg::InBox(Line& line){
	bool isVisible=true;
	int xl=INTIALIZE,xr=-INTIALIZE,yt=-INTIALIZE,yb=INTIALIZE;					//�԰�Χ�еı߽�ֵ���г�ʼ��
	int n=boundary.vertexs.size();
	for(int i=0;i<n-1;i++){
		if(boundary.vertexs[i].x<=xl) xl=boundary.vertexs[i].x;
		if(boundary.vertexs[i].x>=xr) xr=boundary.vertexs[i].x;
		if(boundary.vertexs[i].y>=yt) yt=boundary.vertexs[i].y;
		if(boundary.vertexs[i].y<=yb) yb=boundary.vertexs[i].y;
	}
	if(line.startpoint.x<xl&&line.endpoint.x<xl) return false;
	else if((line.startpoint.x>xr)&&(line.endpoint.x>xr)) return false;
	else if(line.startpoint.y>yt&&line.endpoint.y>yt) return false;
	else if(line.startpoint.y<yb&&line.endpoint.y<yb) return false;
	else return true;
}

/*
*���ܣ�����(x1,y1)(x2,y2)һ��ʽֱ�߷���Ϊ(y2-y1)x+(x1-x2)y+x2y1-x1y2=0��
*      ������������ʽ�ж�(x,y)��(x1,y1)(x,2,y2)�߶ε�λ��,���ض���ʽֵ
*������(x1,y1),(x2,y2)Ϊ���߶����˶˵������ֵ  (x,y��Ϊ���жϵĵ������ֵ
*˼·�������жϵĵ������������ʽ�У���֪��������
*/
int CDemo_ClipView_VCDlg::Multinomial(int x1,int y1,int x2,int y2,int x,int y){
	int result=(y2-y1)*x+(x1-x2)*y+x2*y1-x1*y2;
	return result;
}


/*
*���ܣ�����line1,line2����ֱ�ߵĽ���
*˼·������ֱ�ߵ�һ��ʽ���̣���ֱ��ֱ�ӵĽ���
*/
CPoint CDemo_ClipView_VCDlg::CrossPoint(Line& line1,Line& line2){
	CPoint CrossP;
	int a1 = line1.endpoint.y-line1.startpoint.y;
	int b1 = line1.startpoint.x-line1.endpoint.x;
	int c1 = line1.endpoint.x*line1.startpoint.y-line1.startpoint.x*line1.endpoint.y;

	int a2 = line2.endpoint.y-line2.startpoint.y;
	int b2 = line2.startpoint.x-line2.endpoint.x;
	int c2 = line2.endpoint.x*line2.startpoint.y-line2.startpoint.x*line2.endpoint.y;

	if((b1*a2-a1*b2)!=0){
		double tmp_y = (double)(a1*c2-c1*a2)/(double)(b1*a2-a1*b2);
		CrossP.y = (long) tmp_y;

		if(a1 != 0) 
		{
			double tmp_x = (-c1-b1* tmp_y )/a1;
			CrossP.x = (long) tmp_x;
		}
		else if(a2 !=0)
		{
			double tmp_x = (-c2-b2* tmp_y )/a2;
			CrossP.x = (long) tmp_x;
		}
	}

	return CrossP;	
}


/*
*���ܣ��жϵ�pt�Ƿ�����line�ϣ����������򷵻�true�����򷵻�false
*˼·������ֱ�ߵ������˵�ͱ��жϵĶ˵㴫��Multinomial()�����У�
*      ���˶���ʽֵΪ0��˵�������߶���
*/
bool CDemo_ClipView_VCDlg::IsOnline(CPoint& pt,Line& line){
	if(Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt.x,pt.y)==0)
		return 1;
	else 
		return 0;
}


/*
*���ܣ��ж���pt1,pt2�����˵������߶����߶�line�Ƿ��ཻ��
*      ���ཻ�򷵻�true�����򷵻�false
*˼·���ֱ�pt1,line�������˵��pt2��line�������˵㴫��Multinomial()�����У�
*      �����ص���������ʽֵ�����˵��pt1,pt2�����߶κ�line�ཻ
*/
bool CDemo_ClipView_VCDlg::Intersect(CPoint& pt1,CPoint& pt2,Line& line){
	int a1=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt1.x,pt1.y);
	int a2=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt2.x,pt2.y);
	if((a1<0&&a2>0)||(a1>0&&a2<0))
		return 1;
	else
		return 0;
}


/*
*���ܣ�����ü��߽�boundary�ͱ��ü��߶�line.���زü�������ʾ�߶�
*˼·�����ݵ������ε�λ�ã��������Ҫ��ʾ�߶εĶ˵㣬�ó���ʾ�߶�
*/
Line CDemo_ClipView_VCDlg::result(Line& line){
	Line line_result;
	line_result.startpoint.x=line_result.startpoint.y=INTIALIZE;
	line_result.endpoint.x=line_result.endpoint.y=INTIALIZE;

	bool bspt=isPointInBoundary(line.startpoint);								//startpoint�Ƿ��ڶ�����ڣ����Ƿ���ֵΪtrue

	bool bept=isPointInBoundary(line.endpoint);									//endpoint�Ƿ��ڶ�����ڣ����Ƿ���ֵΪtrue
	
	if(bspt&&bept)																//�����㶼�ڶ����������Ƹ�ֱ��
		return line; 
	
	else if((bspt&&(!bept))||((!bspt)&&bept)){									//��startpoint�ڶ�����ڣ���endpoint�ڶ������
		if(bspt&&(!bept))
			line_result.startpoint=line.startpoint;
		else if((!bspt)&&bept)
			line_result.startpoint=line.endpoint;

		//��ʼ �󽻵�
		int n=boundary.vertexs.size();
		int i=0;
		while(i<n-1){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			
			if(Intersect(side.startpoint,side.endpoint,line)){					//����(x1,y1)(x2,y2)һ��ʽֱ�߷��� 
																				//(y2-y1)x+(x1-x2)y+x2y1-x1y2=0
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=CrossPoint(line,side);
						break;
				}
				else i++;
			}
			else i++;
		}
		//���� �󽻵�

		return line_result;														//���ƽ��㵽startpoint��ֱ��
	}

	else if((!bspt)&&(!bept)){												    //��startpoint��endpoint���ڶ������
		int n=boundary.vertexs.size();
		int i=0,nCrossPoint=0;
		while(i<(n-1)&&nCrossPoint!=2){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			bool isIntersect=Intersect(side.startpoint,side.endpoint,line);
			if(isIntersect&&line_result.startpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.startpoint=p;
						nCrossPoint++;
						i++;
				}
				else i++;
			}
			else if(isIntersect&&line_result.endpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&
					p.x>=min(line.startpoint.x,line.endpoint.x)&&
					p.y<=max(line.startpoint.y,line.endpoint.y)&&
					p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=p;
						nCrossPoint++;
						i++;
				}
				else i++;
			}
			else i++;
		}
		return line_result;
	}
}


void CDemo_ClipView_VCDlg::dealConvex(vector<Line>& lines,Boundary& boundary,int threadNumber)
{
	//��ʼ ����͹����δ���
	for(int i=0;i<lines.size();i++){
		bool overlap=isOverlap(lines[i]);
		if (overlap)
		{
				omp_set_lock(&critical_line_number[2]); 
				line_overlap_bound++;
				omp_unset_lock(&critical_line_number[2]); 
		}
		int size=boundary.vertexs.size()-1;
		bool haveIntersect=false;
		for (int ii=0;ii<size;ii++)
		{
			CPoint p1=boundary.vertexs[ii];
		    CPoint p2=boundary.vertexs[ii+1];
			if(Intersect(p1,p2,lines[i])){
				haveIntersect=true;
				break;
			}
		}
		
		//��ʼ û�н�������
		if (!haveIntersect)
		{
			if (isPointInBoundary(lines[i].startpoint))							//����ڶ������
			{
				Line l=lines[i];
				omp_set_lock(&critical_sections[threadNumber]); 
				lines_to_draw[threadNumber].push_back(lines[i]);
				omp_unset_lock(&critical_sections[threadNumber]);				
				
				if (!overlap)
				{
					omp_set_lock(&critical_line_number[0]); 
					line_in_bound++;
					omp_unset_lock(&critical_line_number[0]); 
				}
			}
			else if (!overlap)

			{
				omp_set_lock(&critical_line_number[1]); 
				line_out_bound++;
				omp_unset_lock(&critical_line_number[1]); 
			}
			continue;
		}
		//���� û�н�������

		if(InBox(lines[i])){													//���ж��߶��Ƿ��ڰ�Χ���ڣ������Բ�������ʾ���߶Σ�
																				//����������²���
			Line line=result(lines[i]);
			if(line.startpoint.x!=INTIALIZE &&
				line.startpoint.y!=INTIALIZE &&
				line.endpoint.x!=INTIALIZE &&
				line.endpoint.y!=INTIALIZE)
				lines_to_draw[threadNumber].push_back(line);
		}
	}
	//���� ����͹����δ���
}

/*----------------------------------------------- ͹������߲ü��㷨 ends----------------------------------------------------------*/





/*----------------------------------------------- ���������߲ü��㷨 starts------------------------------------------------------*/

/*
*���ܣ��������������Ĳ��
*������a1,a2Ϊ��һ��������������յ㣬b1,b2Ϊ�ڶ���������������յ�
*/
inline int CDemo_ClipView_VCDlg::crossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2)
{
	int ux=a2.x-a1.x;
	int uy=a2.y-a1.y;
	int vx=b2.x-b1.x;
	int vy=b2.y-b1.y;
	return ux*vy-uy*vx;
}


/*
*���ܣ��ж�һ�����Ƿ���һ���߶��ϣ��������ӳ���
*������pΪ�õ㣬p1��p2Ϊ���߶ε������˵㣬iΪ���߶ζ�Ӧ�ıߵ����
*˼·�����жϵ��Ƿ����߶εľ��ο��ڣ����ǵĻ�ֱ�ӷ���false�����ж�(P-P1)X(P2-P1)�Ƿ����0������0��ʾ���߶��ϡ�
*/
inline bool CDemo_ClipView_VCDlg::isPointInLine(CPoint& p,CPoint& p1,CPoint& p2,int i)
{
	if (p.x<edgeRect[i].left || p.x>edgeRect[i].right || 
		p.y<edgeRect[i].bottom || p.y>edgeRect[i].top)
		return false;
	if (!crossMulti(p1,p,p1,p2)==0)							//�жϲ���Ƿ�Ϊ0
		return false;
	return true;
}

/*
*���ܣ��ж�ĳһ���߶��Ƿ������εı߽��ص�����Ϊ������Ҫ�������ʾ�ص����߶�����
*˼·�����ж��Ƿ�ƽ�У���ƽ�о����������ж��Ƿ���һ���˵�����һ���߶��ϣ��еĻ���˵���ص�
*/
bool CDemo_ClipView_VCDlg::isOverlap(Line l)
{
	int size=boundary.vertexs.size()-1;
	for (int i=0;i<size;i++)
	{
		CPoint p1=boundary.vertexs[i];
		CPoint p2=boundary.vertexs[i+1];

		if (!((p2.x-p1.x)*(l.endpoint.y-l.startpoint.y)==(p2.y-p1.y)*(l.endpoint.x-l.startpoint.x)))
			continue;

		bool isP1InLine=false,isP2InLine=false,isP3InLine=true;
		isP1InLine=isPointInLine(l.startpoint,p1,p2,i);
		if (!isP1InLine)
			isP2InLine=isPointInLine(l.endpoint,p1,p2,i);

		if (!isP1InLine && !isP2InLine)
		{
			if (p1.x<min(l.startpoint.x,l.endpoint.x) || p1.x>max(l.startpoint.x,l.endpoint.x) || 
				p1.y<min(l.startpoint.y,l.endpoint.y) || p1.y>max(l.startpoint.y,l.endpoint.y))
				isP3InLine=false;
			if (isP3InLine && !crossMulti(l.startpoint,p1,l.startpoint,l.endpoint)==0)
				isP3InLine=false;
		}


		if (isP1InLine || isP2InLine || isP3InLine)
			return true;
	}
	return false;
}


/*
*���ܣ����ڽ�������ıȽϺ�������tֵ�ô�С��������
*������a,b�ֱ�Ϊ��������
*/
bool CDemo_ClipView_VCDlg::Compare(IntersectPoint a,IntersectPoint b)
{
	if (a.t<b.t)
		return true;
	else
		return false;
}

/*
*���ܣ��ж϶����ÿ�����ǰ��㻹��͹��
*˼·�����������ߵ������Ĳ������֪͹���ֵ�밼���ֵ�����Բ�ͬ��
*      ����x�������ĵ�һ����͹�㡣�ȼ����������ߵ������Ĳ�������棬
*	   ͬʱ�ҳ�x���ĵ㡣��ͨ����x���ĵ�Ƚ������Եó���İ�͹�ԡ�
*/
void CDemo_ClipView_VCDlg::preprocessJudgeConvexPoint(vector<BOOL>& convexPoint)
{
	int dotnum=boundary.vertexs.size()-1;										//����ζ�����
	vector<int> z;
	int max=-1,maxnum=-1;
	for (int i=0;i<dotnum;i++)
	{																				
		CPoint* p1;																//��ǰ��Ϊp2,��ǰһ��Ϊp1,��һ��Ϊp3
		if (i==0)
			p1=&boundary.vertexs[dotnum-1];
		else
			p1=&boundary.vertexs[i-1];
		CPoint* p2=&boundary.vertexs[i];
		CPoint* p3=&boundary.vertexs[i+1];
		int crossmulti=crossMulti(*p1,*p2,*p2,*p3);								//�������������ߵĲ������p1p2��p2p3��

		z.push_back(crossmulti);
		if (p2->x>max)															//�ҳ�x���ĵ㣬��¼
		{
			max=p2->x;
			maxnum=i;
		}
	}

	isConvexPoly=true;
	//��ʼ ͨ����x���ĵ�Ƚ������Եó���İ�͹��
	for (int i=0;i<dotnum;i++)  
		if ((z[maxnum]>0 && z[i]>0)||(z[maxnum]<0 && z[i]<0))
			convexPoint.push_back(true);
		else
		{
			convexPoint.push_back(false);
			isConvexPoly=false;
		}
	//���� ͨ����x���ĵ�Ƚ������Եó���İ�͹��
	convexPoint.push_back(convexPoint[0]);

}

/*
*���ܣ��������εıߵ��ڷ�����
*˼·���ƶ���εı߽����μ��㡣����ĳһ���ߣ������⹹���һ���뵱ǰ�ߴ�ֱ�ķ�������
*	   ���÷�������ǰһ���ߵ������ĵ��Ϊ�����򽫷���������
*	   ����������ߵĹ�������Ϊ���㣬�򽫸÷������ٷ���
*/
void CDemo_ClipView_VCDlg::preprocessNormalVector(vector<Vector>& normalVector,vector<BOOL>& convexPoint)
{

	int edgenum=boundary.vertexs.size()-1;											//����α���

	for (int i=0;i<edgenum;i++)
	{
		CPoint p1=boundary.vertexs[i];
		CPoint p2=boundary.vertexs[i+1];											//p1p2Ϊ����εı�

		//��ʼ ���ڷ�����
		CPoint p0;																	//p0Ϊp1ǰ��һ������
		if (i==0)
			p0=boundary.vertexs[edgenum-1];
		else
			p0=boundary.vertexs[i-1];

		double n1,n2;		
		if (p2.y-p1.y==0)															//���������һ��������
		{
			n2=1;
			n1=0;
		}
		else
		{
			n1=1;
			n2=-(double)(p2.x-p1.x)/(p2.y-p1.y);
		}
		if ((p1.x-p0.x)*n1+(p1.y-p0.y)*n2>0)										//��ǰһ���ߵ�ˣ����������ˣ�ʹ����������
		{
			n1=-n1;
			n2=-n2;
		}
		if (!convexPoint[i])														//����ǰ��㣬�������ٷ���
		{
			n1=-n1;
			n2=-n2;
		}

		Vector nv;
		nv.x=n1;
		nv.y=n2;
		normalVector.push_back(nv);
		//���� ���ڷ�����
	}
}


/*
*���ܣ��������εıߵ�����ο�
*˼·��ĳ�߶ε�����ο����Ը��߶�Ϊ�Խ��ߵľ��Σ�
*	   ���ڿ����ų��߶���߲��ཻ����������ߵ�����ο����߶ε�����ο��ཻʱ����   
*/
void CDemo_ClipView_VCDlg::preprocessEdgeRect(const Boundary boundary)
{
	int edgenum=boundary.vertexs.size()-1;
	for (int i=0;i<edgenum;i++)
	{
		RECT r;
		r.bottom=min(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.top=max(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.left=min(boundary.vertexs[i].x,boundary.vertexs[i+1].x);
		r.right=max(boundary.vertexs[i].x,boundary.vertexs[i+1].x);

		edgeRect.push_back(r);
	}
}



/*
*���ܣ���������εĲü�
*˼·����Ԥ�����жϳ�ÿ����İ�͹�ԣ�������ж���εıߵ�����ο�
*	   Ȼ��ö��ÿ���߶���ÿ������εıߣ���ͨ�������ų���
*      �ų����Բ������ཻ�ıߣ���ͨ�����������һ���ж��Ƿ��н��㡣
*	   �����øĽ���cyrus-beck�㷨��������ֵ���Լ����������
*     �����㻹����㣩�������߶ε������Ϊ��㣬�յ���Ϊ���㡣
*	   Ȼ�����߶������εĶ����ཻ�����������
*	   ���Ѱ�����з��ϡ����-���㡱���߶���Ϊ��ʾ���߶Ρ�
*/
void CDemo_ClipView_VCDlg::dealConcave(vector<Line>& lines,Boundary& boundary,int threadNumber)
{
	vector<IntersectPoint> intersectPoint;										//�߶ε����н��㣨Ҳ�����߶ε�������յ㣩

	int edgenum=boundary.vertexs.size()-1;
	int linenum=lines.size();
	RECT r;
	for (int i =0;i<linenum;i++)												//ö��ÿ���߶�
	{
		r.bottom=min(lines[i].startpoint.y,lines[i].endpoint.y);
		r.top=max(lines[i].startpoint.y,lines[i].endpoint.y);
		r.left=min(lines[i].startpoint.x,lines[i].endpoint.x);
		r.right=max(lines[i].startpoint.x,lines[i].endpoint.x);



		intersectPoint.clear();													//���߶���㵱����������λ
		IntersectPoint ip1;
		ip1.isIntoPoly=true;
		ip1.t=0;
		ip1.point=lines[i].startpoint;
		intersectPoint.push_back(ip1);

		BOOL haveIntersect=false;
		for (int j=0;j<edgenum;j++)												//ö��ÿ������εı�
		{
			//��ʼ �����ų���
			if (r.left>edgeRect[j].right || r.right<edgeRect[j].left ||
				r.top<edgeRect[j].bottom || r.bottom>edgeRect[j].top)
				continue;
			//���� �����ų���


			//��ʼ ��������
			CPoint p1=boundary.vertexs[j];
			CPoint p2=boundary.vertexs[j+1];									//p1p2Ϊ����εı�
			CPoint q1=lines[i].startpoint;
			CPoint q2=lines[i].endpoint;										//q1q2Ϊ�߶�
			long long a= ((long long)crossMulti(p1,q1,p1,p2))*(crossMulti(p1,p2,p1,q2));
			long long b= ((long long)crossMulti(q1,p1,q1,q2))*(crossMulti(q1,q2,q1,p2));
			if (!(a>=0 && b>=0))												//���ཻ������������
				continue;
			//���� ��������

			//��ʼ ȡ���ڷ�����
			double n1=normalVector[j].x;
			double n2=normalVector[j].y;
			//���� ȡ���ڷ�����

			//��ʼ cyrus-beck�㷨 �������t
			double t1=n1*(q2.x-q1.x)+n2*(q2.y-q1.y);
			double t2=n1*(q1.x-p1.x)+n2*(q1.y-p1.y);
			double t=-t2/t1;
			CPoint pt;
			pt.x=(LONG)((q2.x-q1.x)*t+q1.x);
			pt.y=(LONG)((q2.y-q1.y)*t+q1.y);
			if (t>=-0.000000001 && t<=1.000000001)								//�������Ĳ���t���� t>=0 && t<=1, ˵���������߶��ϣ��������뽻�㼯
			{
				IntersectPoint ip;
				if (t1>0)
					ip.isIntoPoly=true;
				else
					ip.isIntoPoly=false;
				ip.t=t;
				ip.point=pt;
				ip.contexPoint=j;
				intersectPoint.push_back(ip);

				haveIntersect=true;
			}
			//���� cyrus-beck�㷨 �������t

		}


		bool overlap=isOverlap(lines[i]);
		if (overlap)
		{
				omp_set_lock(&critical_line_number[2]); 
				line_overlap_bound++;
				omp_unset_lock(&critical_line_number[2]); 
		}

		//��ʼ û�н�������
		if (!haveIntersect)
		{
			if (isPointInBoundary(lines[i].startpoint))							//����ڶ������
			{
				Line l=lines[i];
				

				omp_set_lock(&critical_sections[threadNumber]); 
				lines_to_draw[threadNumber].push_back(lines[i]);
				omp_unset_lock(&critical_sections[threadNumber]);				
				
				if (!overlap)
				{
					omp_set_lock(&critical_line_number[0]); 
					line_in_bound++;
					omp_unset_lock(&critical_line_number[0]); 
				}
			}
			else if (!overlap)

			{
				omp_set_lock(&critical_line_number[1]); 
				line_out_bound++;
				omp_unset_lock(&critical_line_number[1]); 
			}
			continue;
		}
		//���� û�н�������

		//��ʼ ���߶��յ㵱���������ĩλ
		IntersectPoint ip2;
		ip2.isIntoPoly=false;
		ip2.t=1;
		ip2.point=lines[i].endpoint;
		intersectPoint.push_back(ip2);
		//���� ���߶��յ㵱���������ĩλ

		std::sort(intersectPoint.begin()+1,intersectPoint.end()-1,Compare);		//��t��С��������


		/*��ʼ ����
		���������tֵ��ͬ�����߶������εĶ����ཻ�������������
		����һ������㣬һ���ǳ��㣬����ݶ���İ�͹�����°����롢��
		�����ܶ���һ��������ĵ㣬���߶εĶ˵㣩*/
		vector<IntersectPoint>::iterator iter = intersectPoint.begin();
		iter++;
		for (;iter!= intersectPoint.end()&&(iter+1)!= intersectPoint.end()
			&&(iter+2)!= intersectPoint.end();iter++ )
			if ( ((*iter).isIntoPoly ^ (*(iter+1)).isIntoPoly )
				&&abs((*iter).t-(*(iter+1)).t)<1e-9)  
			{
				//��ʼ �õ��ö������
				int contexPointNumber=max((*iter).contexPoint,(*(iter+1)).contexPoint);
				if ( ((*iter).contexPoint==edgenum-1 && (*(iter+1)).contexPoint==0)
					|| ((*(iter+1)).contexPoint==edgenum-1&& (*iter).contexPoint==0) )
					contexPointNumber=0;
				//���� �õ��ö������

				if (convexPoint[contexPointNumber]&& !(*iter).isIntoPoly)		//�����͹�㣬����Ϊ�Ƚ����
				{
					(*iter).isIntoPoly=true;
					(*(iter+1)).isIntoPoly=false;
				}
				else if (!convexPoint[contexPointNumber]
						&& (*iter).isIntoPoly)									//����ǰ��㣬����Ϊ�ȳ����
				{
					(*iter).isIntoPoly=false;
					(*(iter+1)).isIntoPoly=true;
				}

			}


			//��ʼ ���潻��
			int size=(int)intersectPoint.size()-1;
			for (int j=0;j<size;j++)
			{
				if (intersectPoint[j].isIntoPoly && !intersectPoint[j+1].isIntoPoly)
				{
					Line l;
					l.startpoint=intersectPoint[j].point;
					l.endpoint=intersectPoint[j+1].point;
					omp_set_lock(&critical_sections[threadNumber]); 
					lines_to_draw[threadNumber].push_back(l);
					omp_unset_lock(&critical_sections[threadNumber]); 
					j++;
				}
			}
			//���� ���潻��

		/*���� ����
		���������tֵ��ͬ�����߶������εĶ����ཻ�������������
		����һ������㣬һ���ǳ��㣬����ݶ���İ�͹�����°����롢��
		�����ܶ���һ��������ĵ㣬���߶εĶ˵㣩*/
	}

}


/*----------------------------------------------- ���������߲ü��㷨 ends--------------------------------------------------------*/





/*----------------------------------------------- ��������Բ�ü��㷨 starts------------------------------------------------------*/

/*
*���ܣ�����һ��������Բ�����δ����ཻ��Ҫ���ƵĻ��Σ����洢��circles_to_draw������
*˼·��ͨ������Բ�����εĽ��㣬��ͨ�����ڽ����˳ʱ���Բ�ϵĽ����Ƿ��ڶ�����ڣ�
*	   �жϻ����Ƿ���Ҫ����   
*/
void  CDemo_ClipView_VCDlg::forCircleRun(vector<Circle>& circles,Boundary& boundary,int threadNumber)
{

	for(unsigned int i = 0;i<circles.size();i++){									//�ٴο�ʼ����ÿһ��Բ
		vector<XPoint> point_Array;													//���ڴ洢ÿ��Բ�����δ��ڵĽ���
		getInterpointArray(point_Array,i,circles);									//��ô洢���������

		//��ʼ ����û�н����Լ����е����
		if(point_Array.size()==0||point_Array.size()==1)							//����ȫ������β��ཻ�Լ����е������Բ�ų���
		{
			if (point_Array.size()==1)
			{
				omp_set_lock(&critical_circle_number[1]);  
				circle_inter_bound++;
				omp_unset_lock(&critical_circle_number[1]);
			}
			XPoint mm;
			mm.x = circles[i].center.x;
			mm.y = circles[i].center.y;
			bool inBoundary = isPointInBoundary(mm);								//Բ�����ڶ������
			/*if (inBoundary==false&&point_Array.size()==0)
			{
				omp_set_lock(&critical_circle_number[1]);  
				circle_inter_bound++;
				omp_unset_lock(&critical_circle_number[1]);
			}*/
			if (inBoundary==true)
			{
				long _x = boundary.vertexs[0].x-circles[i].center.x;
				long _y = boundary.vertexs[0].y-circles[i].center.y;
				bool t = (_x*_x+_y*_y)>(circles[i].radius*circles[i].radius);
				/*if (t == false && point_Array.size()==0)
				{
					omp_set_lock(&critical_circle_number[1]);  
					circle_inter_bound++;
				omp_unset_lock(&critical_circle_number[1]);
				}*/
				
				if (t==true)														//��Բ�İ뾶�Ƚ�С
				{
					if (point_Array.size()==0)
					{
						omp_set_lock(&critical_circle_number[0]);  
						circle_in_bound++;
						omp_unset_lock(&critical_circle_number[0]);
					}
					//��ʼ ������Բ
					CRect rect(circles[i].center.x - circles[i].radius,circles[i].center.y - 
						       circles[i].radius,circles[i].center.x + circles[i].radius,
							   circles[i].center.y + circles[i].radius);
					CPoint start_point,end_point;
					start_point.x = 0; start_point.y = 0;
					end_point.x = 0; end_point.y = 0;
					/*CClientDC dc(dlg);
					dc.Arc(&rect, start_point, end_point);*/
					struct _arc2draw arc2draw ;
					arc2draw.rect = rect;
					arc2draw.start_point = start_point;
					arc2draw.end_point = end_point;
					omp_set_lock(&critical_sections[threadNumber]);  
					circles_to_draw[threadNumber].push_back(arc2draw);
					omp_unset_lock(&critical_sections[threadNumber]); 
					//���� ������Բ
				}
			}
		}
		//���� ����û�н����Լ����е����


		//��ʼ ����һ������
		else if (point_Array.size()>1)
		{
			omp_set_lock(&critical_circle_number[1]);  
			circle_inter_bound++;
			omp_unset_lock(&critical_circle_number[1]);

			vector<XPoint>::iterator pos;
			point_Array.push_back(point_Array[0]);									//Ϊ�˱������ۺͼ��㣬�����������������������һ������
			
			/*��ʼ ����Բ�պù�����ε�ĳһ��ʱ��
			  ����ָõ㱻��¼���ε����������Ҫ�����ų�*/
			for (pos = point_Array.begin()+1; pos != point_Array.end(); pos++)
			{
				if (abs((*pos).x -(*(pos-1)).x)<0.1 
					&& abs((*pos).y -(*(pos-1)).y)<0.1)
				{
					pos = point_Array.erase(pos);
					pos--;
				}
			}
			/*���� ����Բ�պù�����ε�ĳһ��ʱ��
			  ����ָõ㱻��¼���ε����������Ҫ�����ų�*/

			//��ʼ �Խ��������е�ÿһ��������б���
			for (unsigned int j = 0; j < point_Array.size()-1; j++)
			{
				XPoint mid_point = getMiddlePoint(point_Array,i,j,circles);				//������������Բ�ϵ��м��
				bool mid_in_bound = isPointInBoundary(mid_point);						//�ж��м���Ƿ��ڶ���δ�����
				if(mid_in_bound == true)												//�ڶ���δ����ڣ�������Բ�� 
				{				
					int start_x = circles[i].center.x-circles[i].radius;
					int start_y = circles[i].center.y-circles[i].radius;
					int end_x =  circles[i].center.x+circles[i].radius;
					int end_y = circles[i].center.y+circles[i].radius;

					CRect rect(start_x,start_y,end_x,end_y);
					CPoint tmp,tmp2;
					long tmp_xy =(long)(point_Array[j].x);
					if (point_Array[j].x-tmp_xy>0.5)
					{
						tmp.x = tmp_xy+1;
					}
					else
						tmp.x = tmp_xy;

					tmp_xy =(long)(point_Array[j].y);
					if (point_Array[j].y-tmp_xy>0.5)
					{
						tmp.y = tmp_xy+1;
					}
					else
						tmp.y = tmp_xy;
					tmp_xy =(long)(point_Array[j+1].x);
					if (point_Array[j+1].x-tmp_xy>0.5)
					{
						tmp2.x = tmp_xy+1;
					}
					else
						tmp2.x = tmp_xy;

					tmp_xy =(long)(point_Array[j+1].y);
					if (point_Array[j+1].y-tmp_xy>0.5)
					{
						tmp2.y = tmp_xy+1;
					}
					else
						tmp2.y = tmp_xy;
					if (!(tmp.x==tmp2.x&&tmp.y==tmp2.y))
					{
						struct _arc2draw arc2draw ;
						arc2draw.rect = rect;
						arc2draw.start_point = tmp;
						arc2draw.end_point = tmp2;
						omp_set_lock(&critical_sections[threadNumber]);  
						circles_to_draw[threadNumber].push_back(arc2draw);
						omp_unset_lock(&critical_sections[threadNumber]); 
					}


				}
			}
			//���� �Խ��������е�ÿһ��������б���

		}
		//���� ����һ������

	}
}


/*
*���ܣ�����Բ��Բ��������Բ��ĳ��������Լ��뾶�ĳ��ȣ�
*      ͨ��Բ�Ĳ������̣���øõ�ĽǶ�
*˼·��ͨ�����Ǻ���������á�
*/
double CDemo_ClipView_VCDlg::getAngle(double x1,double x2,double y1,double y2,double r)
{
	double cos = (double)(x1-x2)/(double)r;
	double sin = (double)(y1-y2)/(double)r;
	if (sin>0)
	{
		double a = acos(cos);
		return a;
	}
	else
	{
		double a = PI+PI- acos(cos);
		return a;
	}
}


/*
*���ܣ���ö���δ�����Բ�����ڵ�����������Բ�ϵĵ��м�㡣
*˼·��ͨ��Բ�Ĳ������̣����������������뷽���У������������ԵĽǶȣ���ʹ��getAngle������
*	   Ȼ��ͨ�����Ǽ��㣬����м��ĽǶȣ����˽Ƕȴ���Բ�Ĳ������̣��Ϳ�������м�������
*/
XPoint CDemo_ClipView_VCDlg::getMiddlePoint(vector<XPoint>& point_Array,int i,int j,vector<Circle>& circles2)
{
	double a1 = getAngle(point_Array[j].x,circles2[i].center.x,point_Array[j].y,circles2[i].center.y,circles2[i].radius);
	double a2 = getAngle(point_Array[j+1].x,circles2[i].center.x,point_Array[j+1].y,circles2[i].center.y,circles2[i].radius);
	double mid_a = 0;

	if(a1<PI&&a2<PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}

	}
	else if (a1<PI&&a2>PI)
	{
		if(a1+a2>2*PI)
		{
			mid_a = (a1+a2)/2-PI;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}

	}
	else if (a2<PI&&a1>PI)
	{

		mid_a = (a1+a2)/2;
	}
	else if (a2>PI&&a1>PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2-PI;
		}

	}
	double x = circles2[i].center.x+circles2[i].radius*cos(mid_a);
	double y = circles2[i].center.y+circles2[i].radius*sin(mid_a);
	XPoint point ;
	point.x = x;
	point.y = y;
	return point;
}


/*
*��������
*���ܣ��жϵ��Ƿ��ڶ���δ�����
*˼·�����ݽ��㷨���ĳ���Ƿ��ڶ���εĴ����ڲ���
*      ͨ���õ�ȡƽ����y���ֱ��l��
*	   ���ж�ֱ���Ƿ񴩹�����δ��ڵĵ㡣
*      ���������ж���õ����ڵ������Ƿ���l�����ߣ�
*	   ���ڣ��򽻵�����һ������ͬһ�ߣ��򽻵����Ӷ���
*	   Ȼ���ж϶���δ��ڵ�ÿһ�����Ƿ���ֱ��l�н��㣬
*	   ���У��򽻵�����һ�������������Ϊż����
*      ��õ��ڶ���δ����ⲿ����Ϊ��������õ��ڶ���δ����ڲ���
*/
bool CDemo_ClipView_VCDlg::isPointInBoundary(CPoint& point)
{
	XPoint xp;
	xp.x=point.x;
	xp.y=point.y;
	bool ans=isPointInBoundary(xp);
	return ans;
}


/*
*���ܣ��жϵ��Ƿ��ڶ���δ�����
*˼·�����ݽ��㷨���ĳ���Ƿ��ڶ���εĴ����ڲ���ͨ���õ�ȡƽ����y���ֱ��L��
*	   ���ж�ֱ���Ƿ񴩹�����δ��ڵĵ㡣���������ж���õ����ڵ�����p1��p2�Ƿ���L�����ߣ�
*	   ���ڣ��򽻵�����һ������ͬһ�ߣ��򽻵����Ӷ������������ض������һ��p1Ҳ��ֱ��L�ϣ���ôȡ�÷������һ��p3��
*      ��p3��p2��ֱ��L�����ߣ��򽻵�����һ�����򲻼ӣ������������ȷ�������û���������㹲�ߣ���
*	   Ȼ���ж϶���δ��ڵ�ÿһ�����Ƿ���ֱ��l�н��㣨�����Ƕ���εĶ˵㣩�����У��򽻵�����һ��
*	   �����������Ϊż������õ��ڶ���δ����ⲿ����Ϊ��������õ��ڶ���δ����ڲ��� 
*/
bool CDemo_ClipView_VCDlg::isPointInBoundary(XPoint& point)
{

	int interNum = 0;												//������
	long x1 = point.x;												//��x1��y1�����жϵĵ㣬��x2��y2���ǹ���x1��y1��ƽ����y�������С�ĵ�
	long y1 = point.y;
	long x2 = x1;
	long y2 = 0;
	for (unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		//��ʼ ����ƽ����y���ֱ��L������εĶ˵�����
		if ((abs(boundary.vertexs[i].x-point.x)<
			DOUBLE_DEGREE)&&(boundary.vertexs[i].y<point.y))		//�Ա߽�������Ϻ��������߽�������ϵ������������
		{
			if (i==(boundary.vertexs.size()-1))						//���õ��Ƕ���ε����һ�����ʱ��
			{
				double between1 =boundary.vertexs[i-1].x-point.x;	//������һ����
				double between2 =boundary.vertexs[1].x-point.x;		//������һ����
				if (abs(between2)<DOUBLE_DEGREE)					//��������һ����Ҳ����ֱ��L�ϣ�����
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)						//��������һ����Ҳ����ֱ��L�ϣ�����÷����ϵ���һ����
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))					//p3��p2��ֱ��L�����ߣ��򽻵�����һ
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))						//�ж���õ����ڵ�����p1��p2�Ƿ���L�����ߣ����ڣ��򽻵�����һ
				{
					interNum++;
				}

			}
			else if (i==(boundary.vertexs.size()-2))				//���õ��Ƕ���εĵ����ڶ������ʱ��
			{
				double between1 =boundary.vertexs[i-1].x-point.x;
				double between2 =boundary.vertexs[i+1].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))
				{
					interNum++;
				}

			}
			else if (i==1)											//���õ��Ƕ���εĵڶ������ʱ��
			{
				double between1 =boundary.vertexs[0].x-point.x;
				double between2 =boundary.vertexs[2].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[boundary.vertexs.size()-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||
					(between1<0&&between2>0))
				{
					interNum++;
				}

			}
			else													//���õ��ڶ���εĵ���м�λ��ʱ
			{
				double between1 =boundary.vertexs[i-1].x-point.x;
				double between2 =boundary.vertexs[i+1].x-point.x;
				if (abs(between2)<DOUBLE_DEGREE)
				{
					continue;
				}
				if(abs(between1)<DOUBLE_DEGREE)
				{
					double between3 =boundary.vertexs[i-2].x-point.x;
					if ((between3>0&&between2<0)||
						(between3<0&&between2>0))
					{
						interNum++;
					}
					continue;
				}

				if ((between1>0&&between2<0)||(between1<0&&between2>0))
				{
					interNum++;
				}
			}
		}
	}
	//���� ����ƽ����y���ֱ��L������εĶ˵�����

	//��ʼ ����ƽ����y���ֱ��L�������ཻ�����
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		double between1 =boundary.vertexs[i].x-point.x;
		double between2 =boundary.vertexs[i-1].x-point.x;
		if (abs(between1)<DOUBLE_DEGREE||abs(between2)<DOUBLE_DEGREE)
		{
			continue;
		}
		if((between1>0&&between2<0)||(between1<0&&between2>0))
		{
			long x3 = boundary.vertexs[i].x;
		    long x4 = boundary.vertexs[i-1].x;
			long y3 = boundary.vertexs[i].y;
			long y4 = boundary.vertexs[i-1].y;
			long a = y4-y3;
			long b = x3-x4;
			long c = x4*y3-x3*y4;
			double isInter1 = a*point.x+b*point.y+c;
			double isInter2 = a*point.x+b*y2+c;
			if((isInter1<0&&isInter2>0)||(isInter1>0&&isInter2<0))
			{
				interNum++;
			}

		}
	}
	//���� ����ƽ����y���ֱ��L�������ཻ�����

	
	if(interNum%2!=0)
	{
		return true;												//����Ϊ����������ڶ������
	}
	else
	{
		return false;												//����Ϊż��������ڶ������
	}

}


/*
*���ܣ�ͨ��ֱ�߲������̵Ĳ�����ֱ�ߵ�����������꣬��ø�ֱ���ϵ�һ���㡣
*˼·��ͨ��ֱ�ߵĲ������̿�����á�
*/
struct XPoint CDemo_ClipView_VCDlg::getInterpoint(double t,int x1,int x2,int y1,int y2)
{
	double x = x1+(x2-x1)*t;
	double y = y1+(y2-y1)*t;
	struct XPoint pit;
	pit.x = x;
	pit.y = y;

	return pit;
}


/*
*���ܣ����Բ�����εĽ���ļ��ϣ��ҽ���˳������ʱ��ķ������point_Array��������
*˼·����ʱ���������ε�ÿһ���ߣ�����ֱ�ߵ�Բ�ĵľ����ж��Ƿ��н��㣬���н��㣬��ֱ��
*	   �ϵĵ�Ĳ������̵���ʽ����Բ�ı��ʽ������õ�A*t^2+B*t+C=0����ʽ��t��ֱ�߲��������еĲ���������0-1��
*	   ���A,B,C��ֵ���� B^2-4*A*C=0,��ֻ��һ���⣬�ҽ�Ϊ(-B)/(2*A)��
*	   ��B^2-4*A*C>0��������,һ����Ϊ((-B)+sqrt(B^2-4*A*C))/(2*A),��һ����Ϊ((-B)-sqrt(B^2-4*A*C))/(2*A)
*	   Ȼ�󣬶Խ��ֵ�����жϣ�������ڵ���0����С�ڵ���1�����ǽ��㣬������ȥ������ֵ����������̼�����ý��㡣
*	   ��󣬶Խ����˳������жϣ���ֵС��ֵ�ȴ���point_Array������
*/
void CDemo_ClipView_VCDlg::getInterpointArray(vector<XPoint>& point_Array,int circle_num,vector<Circle>& circle2)
{
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		long x1 = boundary.vertexs[i].x;
		long x2 = boundary.vertexs[i-1].x;
		long y1 = boundary.vertexs[i].y;
		long y2 = boundary.vertexs[i-1].y;
		long x0 = circle2[circle_num].center.x;
		long y0 = circle2[circle_num].center.y;
		long a = y2-y1;
		long b = x1-x2;
		long c = x2*y1-x1*y2;
		double _abs = abs((double)(a*x0+b*y0+c));
		double _sqrt = sqrt((double)(a*a+b*b));
		double d = _abs/_sqrt;
		if (d>circle2[circle_num].radius)
		{
			continue;
		}
		long long A = x1*x1+x2*x2+y1*y1+y2*y2-2*x1*x2-2*y1*y2;
		long long B = 2*(x0*x1+x1*x2+y0*y1+y1*y2-x0*x2-y0*y2-x1*x1-y1*y1);
		long long C = x0*x0+x1*x1+y0*y0+y1*y1-2*x0*x1-2*y0*y1-circle2[circle_num].radius*circle2[circle_num].radius;
		long long key = B*B-4*A*C;
		if(key == 0)
		{
			double t = ((double)(-B)/(double)(2*A));
			if (t>=0&&t<=1)
			{
				struct XPoint pit= getInterpoint(t,x1,x2,y1,y2);
				point_Array.push_back(pit);
			}
			else
			{
				continue;
			}
		}
		else if(key > 0)
		{
			double t1 = (double)((-B+sqrt(key))/(2*A));
			double t2 = (double)((-B-sqrt(key))/(2*A));
			if (t1>t2)
			{
				double tmp = t2;
				t2 = t1;
				t1 =tmp;
			}
			if (t1>=0&&t1<=1)
			{				
				struct XPoint pit = getInterpoint(t1,x1,x2,y1,y2);
				point_Array.push_back(pit);
				if (t2>=0&&t2<=1)
				{
					struct XPoint pit = getInterpoint(t2,x1,x2,y1,y2);
					point_Array.push_back(pit);
				}

			}
			else
			{
				if (t2>=0&&t2<=1)
				{
					struct XPoint pit = getInterpoint(t2,x1,x2,y1,y2);
					point_Array.push_back(pit);
				}
				else
				{
					continue;
				}
			}
		}
	}
	if (point_Array.size()>2)
	{

		//��ʼ �����еĽ��㰴��˳ʱ������
		vector<XPoint> y_up_0;
		vector<XPoint> y_down_0;
		for (int i = 0; i < point_Array.size(); i++)
		{
			if (point_Array[i].y <= circle2[circle_num].center.y)
			{
				y_down_0.push_back(point_Array[i]);
			}
			else
			{
				y_up_0.push_back(point_Array[i]);
			}
		}
		if (y_down_0.size()>1)
		{
			for (int i = 0; i < y_down_0.size()-1; i++)
			{
				for (int j = 0; j < y_down_0.size()-1-i; j++)
				{
					if (y_down_0[j].x < y_down_0[j+1].x)
					{
						XPoint tmp;
						tmp.x = y_down_0[j].x;
						tmp.y = y_down_0[j].y;
						y_down_0[j] = y_down_0[j+1];
						y_down_0[j+1] = tmp;
					}

				}
			}
		}
		if (y_up_0.size()>1)
		{
			for (int i = 0; i < y_up_0.size()-1; i++)
			{
				for (int j = 0; j < y_up_0.size()-1-i; j++)
				{
					if (y_up_0[j].x > y_up_0[j+1].x)
					{
						XPoint tmp;
						tmp.x = y_up_0[j].x;
						tmp.y = y_up_0[j].y;
						y_up_0[j] = y_up_0[j+1];
						y_up_0[j+1] = tmp;
					}
				}
			}
		}
		if (y_down_0.size()>0)
		{
			for (int i = 0; i < y_down_0.size(); i++)
			{
				point_Array[i] = y_down_0[i];
			}
		}
		if (y_up_0.size()>0)
		{
			for (int i = 0; i < y_up_0.size(); i++)
			{
				point_Array[y_down_0.size()+i] = y_up_0[i];
			}
		}
		//���� �����еĽ��㰴��˳ʱ������

	}

}

/*----------------------------------------------- ��������Բ�ü��㷨 ends----------------------------------------------------*/


BOOL CDemo_ClipView_VCDlg::XmlNodeToPoint(pugi::xml_node node, CPoint& piont)
{
	pugi::xml_node pointNode = node.first_child();
	CString strValue(pointNode.value());
	int curPos = 0;
	int count = 0;
	while(TRUE)
	{
		CString resToken = strValue.Tokenize(",", curPos);
		if (resToken.IsEmpty())
			break;
		int coord = _ttoi(resToken);
		count++;
		if (count == 1)
		{
			piont.x = coord;
		}
		else if (count == 2)
		{
			piont.y = CANVAS_HEIGHT - coord; //����CAD�봰����ʾ������ϵһ��
		}
	}
	if (count != 2)
	{
		return FALSE;
	}
	return TRUE;
}
int  CDemo_ClipView_VCDlg::SplitCStringToArray(CString str,CStringArray& strArray, CString split)
{
	int curPos = 0;
	while(TRUE)
	{
		CString resToken = str.Tokenize(split, curPos);
		if (resToken.IsEmpty())
			break;
		strArray.Add(resToken);
	}
	return strArray.GetSize();
}
BOOL CDemo_ClipView_VCDlg::IsPointOutCanvas(CPoint point)
{
	if (point.x < 0 || point.y < 0 || point.x > CANVAS_WIDTH || point.y > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsLineOutCanvas(Line line)
{
	if (IsPointOutCanvas(line.startpoint) || IsPointOutCanvas(line.endpoint))
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsCircleOutCanvas(Circle circle)
{
	if (IsPointOutCanvas(circle.center))
	{
		return TRUE;
	}
	if ((circle.center.x - circle.radius) < 0 || (circle.center.x + circle.radius) > CANVAS_WIDTH
		|| (circle.center.y - circle.radius) < 0 || (circle.center.y + circle.radius) > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsBoundaryOutCanvas(Boundary boundary)
{
	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	for (;iter != boundary.vertexs.end(); iter++)
	{
		if (IsPointOutCanvas(*iter))
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CDemo_ClipView_VCDlg::LoadTestCaseData(CString xmlPath, CString caseID)
{
	pugi::xml_document doc;
	if (!doc.load_file(xmlPath)) return FALSE;
	pugi::xml_node root = doc.child("TestRoot");
	pugi::xml_node testCase = root.find_child_by_attribute("ID",caseID);
	if (!testCase) return FALSE;
	for (pugi::xml_node entity = testCase.first_child(); entity; entity = entity.next_sibling())
	{
		pugi::xml_attribute type = entity.attribute("Type");
		CString typeValue(type.value());
		if (typeValue.CompareNoCase("Line") == 0)
		{
			Line line;
			pugi::xml_node startNode = entity.child("StartPoint");
			if(!XmlNodeToPoint(startNode, line.startpoint)) return FALSE;
			pugi::xml_node endNode = entity.child("EndPoint");
			if(!XmlNodeToPoint(endNode, line.endpoint)) return FALSE;
			if (IsLineOutCanvas(line))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			lines.push_back(line);
		} 
		else if(typeValue.CompareNoCase("Circle") == 0)
		{
			Circle circle;
			pugi::xml_node centerNode = entity.child("CenterPoint");
			if(!XmlNodeToPoint(centerNode, circle.center)) return FALSE;
			pugi::xml_node radiusNode = entity.child("Radius");
			pugi::xml_node pointNode = radiusNode.first_child();
			circle.radius = _ttoi(pointNode.value());
			if (IsCircleOutCanvas(circle))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			circles.push_back(circle);
		}
		else if (typeValue.CompareNoCase("Polygon") == 0)
		{
			for (pugi::xml_node vertex = entity.first_child(); vertex; vertex = vertex.next_sibling())
			{
				CPoint point;
				if(!XmlNodeToPoint(vertex, point)) return FALSE;
				boundary.vertexs.push_back(point);
			}
			if (IsBoundaryOutCanvas(boundary))
			{
				hasOutCanvasData = TRUE;
				boundary.vertexs.clear();
			}
		}
	}
	return TRUE;
}
void CDemo_ClipView_VCDlg::ClearTestCaseData()
{
	hasOutCanvasData = FALSE;
	boundary.vertexs.clear();
	circles.clear();
	lines.clear();
	if (beginTIdList.GetSize() > 0)
	{
		beginTIdList.RemoveAll();
	}
	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}

void CDemo_ClipView_VCDlg::BeginMonitor()
{
	m_btn_clip.EnableWindow(FALSE);
	m_stc_drawing.SetWindowText("�ü���...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	GetThreadIdList(beginTIdList);
	startTime = clock();
}
void CDemo_ClipView_VCDlg::EndMonitor()
{
	endTime = clock();
	double useTimeS = (endTime - startTime)/1000;
	m_stc_drawing.ShowWindow(SW_HIDE);
	m_stc_info1.ShowWindow(SW_SHOW);
	CString strTime;
	strTime.Format("ͼ�βü���ϣ���ʱ��%.3lf �룡", useTimeS);
	m_stc_info1.SetWindowText(strTime);
	m_stc_info2.ShowWindow(SW_SHOW);
	CList<int> endTIdList;
	GetThreadIdList(endTIdList);
	POSITION pos = endTIdList.GetHeadPosition();
	while(pos)
	{
		int tId = endTIdList.GetAt(pos);
		CString strId;
		strId.Format("%d", tId);
		if (!beginTIdList.Find(tId))
		{
			HANDLE th = OpenThread(THREAD_QUERY_INFORMATION, FALSE, tId);
			if (th)
			{
				DWORD exitCode = 0;
				GetExitCodeThread(th, &exitCode);
				CloseHandle(th);
				if (exitCode == STILL_ACTIVE)
				{
					MessageBox(_T("�½��߳���������:") + strId, _T("�߳���Ϣ"), MB_OK);
					break;
				}
			}
			else
			{
				MessageBox(_T("�½��߳��޷�����:") + strId, _T("�߳���Ϣ"), MB_OK);
				break;
			}
		}
		endTIdList.GetNext(pos);
	}
}
void CDemo_ClipView_VCDlg::DrawTestCase(CString xmlPath, CString caseID)
{
	m_stc_drawing.ShowWindow(SW_SHOW);
	m_stc_drawing.SetWindowText("ͼ�λ�����...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	ClearTestCaseData();
	if (!LoadTestCaseData(xmlPath, caseID))
	{
		MessageBox("��ȡ����ʧ��!","Demo_CliView_VC",MB_OK);
		m_stc_drawing.SetWindowText("");
		return;
	}

	if (TEST_DRAW_INITIAL)
	{

		COLORREF clrLine = RGB(0,255,0);
		vector<Line>::iterator iterLine = lines.begin();
		for (;iterLine != lines.end(); iterLine++)
		{
			DrawLine(*iterLine, clrLine);
		}

		COLORREF clrCircle = RGB(0,0,255);
		vector<Circle>::iterator iterCircle = circles.begin();
		for (;iterCircle != circles.end(); iterCircle++)
		{
			DrawCircle(*iterCircle, clrCircle);
		}

		COLORREF clrBoundary = RGB(255,0,0);
		DrawBoundary(boundary, clrBoundary);

	}

	if (hasOutCanvasData)
	{
		m_stc_drawing.SetWindowText("���ڳ����߽����ݣ�����");
	}
	else
	{
		m_btn_clip.EnableWindow(TRUE);
		m_stc_drawing.SetWindowText("ͼ�λ�����ϣ�");
	}
}


void CDemo_ClipView_VCDlg::DrawLine(Line line, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.MoveTo(line.startpoint);
	dc.LineTo(line.endpoint);

	dc.SelectObject(penOld);
	penUse.DeleteObject();
}
void CDemo_ClipView_VCDlg::DrawCircle(Circle circle, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.Arc(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.center.x + circle.radius, circle.center.y + circle.radius, 0, 0, 0, 0);

	dc.SelectObject(penOld);
	penUse.DeleteObject();
}
void CDemo_ClipView_VCDlg::DrawBoundary(Boundary boundary, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);


	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	if (iter == boundary.vertexs.end()) return;
	CPoint startPoint = *iter;
	dc.MoveTo(startPoint);
	for (;iter != boundary.vertexs.end(); iter++)
	{
		dc.LineTo(*iter);
	}
	dc.LineTo(startPoint);

	dc.SelectObject(penOld);
	penUse.DeleteObject();
}

BOOL CDemo_ClipView_VCDlg::GetThreadIdList(CList<int>& tIdList)
{
	int pID = _getpid();
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pID);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	te32.dwSize = sizeof(THREADENTRY32);
	BOOL bGetThread = Thread32First(hThreadSnap, &te32);
	while(bGetThread)
	{
		if (te32.th32OwnerProcessID == pID)
		{
			tIdList.AddTail(te32.th32ThreadID);
		}
		bGetThread = Thread32Next(hThreadSnap, &te32);
	}
	CloseHandle(hThreadSnap);
	return TRUE;
}